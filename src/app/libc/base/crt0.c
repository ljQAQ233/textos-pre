extern void _exit(int stat);

__attribute__((weak))
int main(int argc, const char *argv[], const char *envp[])
{
    return 0;
}

extern const char **__environ;
extern void __init_stdio();

void start0(long *args)
{
    int argc = args[0];
    const char **argv = (void *)&args[1];
    const char **envp = (void *)&args[1+argc+1];

    __environ = envp;
    __init_stdio();

    _exit(main(argc, argv, envp));

    asm("ud2");
}

__attribute__((weak))
__attribute__((naked))
void _start(long args)
{
    asm volatile(
        "movq %rsp, %rdi\n"
        "xorq %rbp, %rbp\n"
        "call start0"
        );
}

