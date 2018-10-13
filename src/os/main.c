#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

// initial thread
static int start_thread(int argc, char* argv[])
{
  kz_run(test11_1_main, "test10_1", 1, 0x100, 0, NULL);
  kz_run(test11_2_main, "test10_1", 2, 0x100, 0, NULL);

  kz_chpri(15);
  INTR_ENABLE;
  while (1)
  {
    asm volatile ("sleep");
  }

  return 0;
}

int main(void)
{
  INTR_DISABLE;

  puts("kozos boot succeed!\n");

  kz_start(start_thread, "idle", 0, 0x100, 0, NULL);
  // never return

  return 0;
}

