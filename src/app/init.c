#include <app/sys.h>
#include <app/api.h>

#include <stdio.h>

char buf[64];

void _start()
{
    char *argv[] = {
        NULL,
    };
    char *envp[] = {
        "PWD=/",
        NULL,
    };

    int fd = open("/config", O_RDONLY);
    perror(NULL);

    /*
    int pid = fork();
    if (pid == 0) {
        execve("/cat.elf", argv, envp);
    } else {
        int stat;
        int pchd = wait(&stat);
        printf("chd %d exited : %d\n", pchd, stat);
        while(1);
    }
    */
    execve("/sh.elf", argv, envp);

    write(1, "execve failed!\n", 17);
    while(1);
}
