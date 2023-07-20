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
  sti
  ret

; void IntrStateDisable ()
global IntrStateDisable
IntrStateDisable:
  cli
  ret

