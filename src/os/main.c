#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

// initial thread
static int start_thread(int argc, char* argv[])
{
  kz_run(test08_1_main, "command", 0x100, 0, NULL);
  return 0;
}

int main(void)
{
  INTR_DISABLE;

  puts("kozos boot succeed!\n");

  kz_start(start_thread, "start", 0x100, 0, NULL);
  // never return

  return 0;
}

