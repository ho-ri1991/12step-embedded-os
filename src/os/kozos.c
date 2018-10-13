#include "defines.h"
#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "lib.h"
#include "memory.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

typedef struct _kz_context {
  uint32 sp; // stack pointer
} kz_context;

typedef struct _kz_thread {
  struct _kz_thread* next;
  char name[THREAD_NAME_SIZE + 1];
  int priority;
  char* stack;
  uint32 flags;
#define KZ_THREAD_FLAG_READY (1 << 0)

  struct {
    kz_func_t func;
    int argc;
    char** argv;
  } init;

  struct {
    kz_syscall_type_t type;
    kz_syscall_param_t* param;
  } syscall;

  kz_context context;
} kz_thread;

static struct {
  kz_thread* head;
  kz_thread* tail;
} readyque[PRIORITY_NUM];

static kz_thread* current;
static kz_thread threads[THREAD_NUM];
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];

void dispatch(kz_context* context);

// pop_front
static int getcurrent(void)
{
  if (current == NULL)
    return -1;

  if (!(current->flags & KZ_THREAD_FLAG_READY)) {
    return 1;
  }

  readyque[current->priority].head = current->next;
  if (readyque[current->priority].head == NULL)
    readyque[current->priority].tail = NULL;

  current->flags &= ~KZ_THREAD_FLAG_READY;
  current->next = NULL;

  return 0;
}

// push current into readyque
static int putcurrent(void)
{
  if (current == NULL)
    return -1;
  if (current->flags & KZ_THREAD_FLAG_READY)
    return 0;

  if (readyque[current->priority].tail)
    readyque[current->priority].tail->next = current;
  else
    readyque[current->priority].head = current;

  readyque[current->priority].tail = current;
  current->flags |= KZ_THREAD_FLAG_READY;

  return 0;
}

static void thread_end(void)
{
  kz_exit();
}

static void thread_init(kz_thread* thp)
{
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end();
}

static kz_thread_id_t thread_run(kz_func_t func, char* name, int priority, int stacksize, int argc, char* argv[])
{
  int i;
  kz_thread* thp;
  uint32* sp;
  extern char userstack; // defined in linker script
  static char* thread_stack = &userstack;

  // find empty thread
  for (i = 0; i < THREAD_NUM; i++)
  {
    thp = &threads[i];
    if (!thp->init.func)
      break;
  }
  if (i == THREAD_NUM)
    return -1;

  // clear thread data
  memset(thp, 0, sizeof(*thp));

  // set thread data
  strcpy(thp->name, name);
  thp->next = NULL;
  thp->priority = priority;
  thp->flags = 0;
  thp->init.func = func;
  thp->init.argc = argc;
  thp->init.argv = argv;

  memset(thread_stack, 0, stacksize);
  thread_stack += stacksize;

  thp->stack = thread_stack;

  sp = (uint32*)thp->stack;
  *(--sp) = (uint32)thread_end; // return address from thread_init, not required because thread_init calls it

  // return address from dispatch, and set ccr, see p.247
  // When priority is zero, disable interupt
  *(--sp) = (uint32)thread_init | ((uint32)(priority ? 0 : 0xc0) << 24);

  *(--sp) = 0;           // ER6
  *(--sp) = 0;           // ER5
  *(--sp) = 0;           // ER4
  *(--sp) = 0;           // ER3
  *(--sp) = 0;           // ER2
  *(--sp) = 0;           // ER1
  *(--sp) = (uint32)thp; // ER0 (argument for thread_init)

  thp->context.sp = (uint32)sp;

  putcurrent(); // push cuurent thread which called syscall to readyque

  current = thp;
  putcurrent(); // push new thread to readyque

  return (kz_thread_id_t)current;
}

static int thread_exit(void)
{
  puts(current->name);
  puts(" EXIT.\n");
  memset(current, 0, sizeof(*current));
  return 0;
}

static int thread_wait(void)
{
  putcurrent();
  return 0;
}

static int thread_sleep(void)
{
  // do nothing because the thread is removed from readyque when it calls system call
  return 0;
}

static int thread_wakeup(kz_thread_id_t id)
{
  putcurrent();

  current = (kz_thread*)id;
  putcurrent();

  return 0;
}

static kz_thread_id_t thread_getid(void)
{
  putcurrent();
  return (kz_thread_id_t)current;
}

static int thread_chpri(int priority)
{
  int old = current->priority;
  if (priority >= 0)
    current->priority = priority;
  putcurrent();
  return old;
}

static void* thread_kmalloc(int size)
{
  putcurrent();
  return kzmem_alloc(size);
}

static int thread_kmfree(char* p)
{
  kzmem_free(p);
  putcurrent();
  return 0;
}

static int setintr(softvec_type_t type, kz_handler_t handler)
{
  static void thread_intr(softvec_type_t type, unsigned long sp);

  softvec_setintr(type, thread_intr);
  handlers[type] = handler;

  return 0;
}

static void call_functions(kz_syscall_type_t type, kz_syscall_param_t* p)
{
  switch (type)
  {
  case KZ_SYSCALL_TYPE_RUN: // kz_run
    p->un.run.ret = thread_run(p->un.run.func,
                               p->un.run.name, 
                               p->un.run.priority,
                               p->un.run.stacksize,
                               p->un.run.argc,
                               p->un.run.argv);
    break;
  case KZ_SYSCALL_TYPE_EXIT: // kz_exit
    thread_exit();
    break;
  case KZ_SYSCALL_TYPE_WAIT:
    p->un.wait.ret = thread_wait();
    break;
  case KZ_SYSCALL_TYPE_SLEEP:
    p->un.sleep.ret = thread_sleep();
    break;
  case KZ_SYSCALL_TYPE_WAKEUP:
    p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
    break;
  case KZ_SYSCALL_TYPE_GETID:
    p->un.getid.ret = thread_getid();
    break;
  case KZ_SYSCALL_TYPE_CHPRI:
    p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
    break;
  case KZ_SYSCALL_TYPE_KMALLOC:
    p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
    break;
  case KZ_SYSCALL_TYPE_KMFREE:
    p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
    break;
  default:
    break;
  }
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t* p)
{
  getcurrent();
  call_functions(type, p);
}

static void schedule(void)
{
  int i;

  for (i = 0; i < PRIORITY_NUM; i++)
  {
    if (readyque[i].head)
      break;
  }
  if (i == PRIORITY_NUM) // no threads found
    kz_sysdown();

  current = readyque[i].head;
}

static void syscall_intr(void)
{
  syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void)
{
  puts(current->name);
  puts(" DOWN.\n");
  getcurrent(); // remove from readyque
  thread_exit();
}

static void thread_intr(softvec_type_t type, unsigned long sp)
{
  current->context.sp = sp; // update sp since interrupt handler _intr_syscall (defined in bootload/intr.S) pushed er* to the user stack

  if (handlers[type])
    handlers[type]();

  schedule();
  dispatch(&current->context);
}

void kz_start(kz_func_t func, char* name, int priority, int stacksize, int argc, char* argv[])
{ // initlization of OS, create initial thread which runs func
  kzmem_init();

  current = NULL;

  memset(readyque, 0, sizeof(readyque));
  memset(threads, 0, sizeof(threads));
  memset(handlers, 0, sizeof(handlers));

  setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
  setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

  current = (kz_thread*)thread_run(func, name, priority, stacksize, argc, argv); // create initial thread

  dispatch(&current->context);
  // never return
}

void kz_sysdown(void)
{
  puts("system error!\n");
  while (1)
    ;
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t* param)
{
  current->syscall.type = type;
  current->syscall.param = param;
  asm volatile ("trapa #0"); // execute trap interrupt (call intr_syscall in bootloader)
}

