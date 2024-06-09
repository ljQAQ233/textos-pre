void _start ()
{
    // do syscall
    __asm__ volatile ("movq $0, %rax");
    __asm__ volatile ("int $0x80");
    while (1);
}
