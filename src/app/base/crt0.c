__attribute__((weak))
int main(int argc, const char *argv[], const char *envp[])
{
    return 0;
}

const char **__envp;

void start0(long *args)
{
    int argc = args[0];
    const char **argv = (void *)&args[1];
    const char **envp = (void *)&args[1+argc+1];

    __envp = envp;

    main(argc, argv, envp);

    while(1);
}

__attribute__((weak))
__attribute__((naked))
void _start(long args)
{
    asm volatile(
        "movq $0, %rbp\n" // rbp = 0
        "push %rbp\n"     // rip = 0
        "push %rbp\n"     // rbp = 0
        "movq %rdi, %rsp\n"
        "movq %rsp, %rbp\n"
        "call start0"
        );
}
