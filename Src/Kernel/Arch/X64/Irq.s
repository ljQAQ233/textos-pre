[bits 64]

; bool IntrStateGet ()
global IntrStateGet
IntrStateGet:
  pushf
  pop rax
  shr rax, 9
  and rax, 1
  ret

; void IntrStateEnable ()
global IntrStateEnable
IntrStateEnable:
  pushf
  pop  rax
  or   rax, (1 << 9)
  push rax
  popf
  ret

; void IntrStateDisable ()
global IntrStateDisable
IntrStateDisable:
  pushf
  pop  rax
  xor  rax, (1 << 9)
  push rax
  popf
  ret

