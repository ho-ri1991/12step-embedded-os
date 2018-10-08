  .h8300h
  .section .text
  .global _start
# .type _start,@function
_start:
  mov.l #_bootstack,sp
  jsr @_main

1:
  bra 1b

  .global _dispatch
# .type _dispatch,@function
_dispatch:
  mov.l @er0,er7  // restore stack pointer
  mov.l @er7+,er0 // pop er0
  mov.l @er7+,er1 // pop er1
  mov.l @er7+,er2 // pop er2
  mov.l @er7+,er3 // pop er3
  mov.l @er7+,er4 // pop er4
  mov.l @er7+,er5 // pop er5
  mov.l @er7+,er6 // pop er6
  rte
