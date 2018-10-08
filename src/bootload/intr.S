#include "intr.h"
  .h8300h
  .section .text

  .global _intr_softerr
# .type _intr_softerr,@function
_intr_softerr:
  mov.l er6,@-er7 // push er6
  mov.l er5,@-er7 // push er5
  mov.l er4,@-er7 // push er4
  mov.l er3,@-er7 // push er3
  mov.l er2,@-er7 // push er2
  mov.l er1,@-er7 // push er1
  mov.l er0,@-er7 // push er0
  mov.l er7,er1   // er1 == user stack
  mov.l #_intrstack,sp // change to kernel stack
  mov.l er1,@-er7  // save user stack to kernel stack
  mov.w #SOFTVEC_TYPE_SOFTERR,r0
  jsr   @_interrupt
  mov.l @er7+,er1 // er1 == user stack
  mov.l er1,er7   // restore user stack
  mov.l @er7+,er0 // pop er0
  mov.l @er7+,er1 // pop er1
  mov.l @er7+,er2 // pop er2
  mov.l @er7+,er3 // pop er3
  mov.l @er7+,er4 // pop er4
  mov.l @er7+,er5 // pop er5
  mov.l @er7+,er6 // pop er6
  rte

  .global _intr_syscall
# .type _intr_syscall,@function
_intr_syscall:
  mov.l er6,@-er7 // push er6
  mov.l er5,@-er7 // push er5
  mov.l er4,@-er7 // push er4
  mov.l er3,@-er7 // push er3
  mov.l er2,@-er7 // push er2
  mov.l er1,@-er7 // push er1
  mov.l er0,@-er7 // push er0
  mov.l er7,er1   // er1 == user stack
  mov.l #_intrstack,sp // change to kernel stack
  mov.l er1,@-er7  // save user stack to kernel stack
  mov.w #SOFTVEC_TYPE_SYSCALL,r0
  jsr   @_interrupt
  mov.l @er7+,er1 // er1 == user stack
  mov.l er1,er7   // restore user stack
  mov.l @er7+,er0 // pop er0
  mov.l @er7+,er1 // pop er1
  mov.l @er7+,er2 // pop er2
  mov.l @er7+,er3 // pop er3
  mov.l @er7+,er4 // pop er4
  mov.l @er7+,er5 // pop er5
  mov.l @er7+,er6 // pop er6
  rte

  .global _intr_serintr
# .type _intr_serintr,@function
_intr_serintr:
  mov.l er6,@-er7 // push er6
  mov.l er5,@-er7 // push er5
  mov.l er4,@-er7 // push er4
  mov.l er3,@-er7 // push er3
  mov.l er2,@-er7 // push er2
  mov.l er1,@-er7 // push er1
  mov.l er0,@-er7 // push er0
  mov.l er7,er1   // er1 == user stack
  mov.l #_intrstack,sp // change to kernel stack
  mov.l er1,@-er7  // save user stack to kernel stack
  mov.w #SOFTVEC_TYPE_SERINTR,r0
  jsr   @_interrupt
  mov.l @er7+,er1 // er1 == user stack
  mov.l er1,er7   // restore user stack
  mov.l @er7+,er0 // pop er0
  mov.l @er7+,er1 // pop er1
  mov.l @er7+,er2 // pop er2
  mov.l @er7+,er3 // pop er3
  mov.l @er7+,er4 // pop er4
  mov.l @er7+,er5 // pop er5
  mov.l @er7+,er6 // pop er6
  rte
