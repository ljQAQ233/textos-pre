#include <irq.h>
#include <cpu.h>
#include <gdt.h>
#include <intr.h>
#include <textos/syscall.h>

extern void sys_open();
extern void sys_fcntl();
extern void sys_read();
extern void sys_readv();
extern void sys_write();
extern void sys_writev();
extern void sys_readdir();
extern void sys_seekdir();
extern void sys_close();
extern void sys_lseek();
extern void sys_mmap();
extern void sys_mprotect();
extern void sys_munmap();
extern void sys_stat();
extern void sys_ioctl();
extern void sys_chown();
extern void sys_fchown();
extern void sys_chmod();
extern void sys_fchmod();
extern void sys_dup();
extern void sys_dup2();
extern void sys_pipe();
extern void sys_chdir();
extern void sys_mkdir();
extern void sys_rmdir();
extern void sys_getcwd();
extern void sys_mknod();
extern void sys_mount();
extern void sys_umount2();
extern void sys_execve();
extern void sys_fork();
extern void sys_exit();
extern void sys_wait4();
extern void sys_sigaction();
extern void sys_sigprocmask();
extern void sys_sigreturn();
extern void sys_kill();
extern void sys_uname();
extern void sys_test();
extern void sys_poweroff();
extern void sys_yield();
extern void sys_getuid();
extern void sys_getgid();
extern void sys_geteuid();
extern void sys_getegid();
extern void sys_setuid();
extern void sys_setgid();
extern void sys_setreuid();
extern void sys_setregid();
extern void sys_getgroups();
extern void sys_setgroups();
extern void sys_getpid();
extern void sys_getppid();

extern void sys_socket();
extern void sys_bind();
extern void sys_listen();
extern void sys_accept();
extern void sys_connect();
extern void sys_sendmsg();
extern void sys_recvmsg();
extern void sys_sendto();
extern void sys_recvfrom();

typedef long (*sys_func)(long, long, long, long, long, long);

void *sys_handlers[] = {
    [SYS_open] = sys_open,
    [SYS_fcntl] = sys_fcntl,
    [SYS_read] = sys_read,
    [SYS_readv] = sys_readv,
    [SYS_write] = sys_write,
    [SYS_writev] = sys_writev,
    [SYS_readdir] = sys_readdir,
    [SYS_seekdir] = sys_seekdir,
    [SYS_close] = sys_close,
    [SYS_lseek] = sys_lseek,
    [SYS_mmap] = sys_mmap,
    [SYS_mprotect] = sys_mprotect,
    [SYS_munmap] = sys_munmap,
    [SYS_stat] = sys_stat,
    [SYS_ioctl] = sys_ioctl,
    [SYS_chown] = sys_chown,
    [SYS_fchown] = sys_fchown,
    [SYS_chmod] = sys_chmod,
    [SYS_fchmod] = sys_fchmod,
    [SYS_dup] = sys_dup,
    [SYS_dup2] = sys_dup2,
    [SYS_pipe] = sys_pipe,
    [SYS_getcwd] = sys_getcwd,
    [SYS_chdir] = sys_chdir,
    [SYS_mkdir] = sys_mkdir,
    [SYS_rmdir] = sys_rmdir,
    [SYS_mknod] = sys_mknod,
    [SYS_mount] = sys_mount,
    [SYS_umount2] = sys_umount2,
    [SYS_execve] = sys_execve,
    [SYS_fork] = sys_fork,
    [SYS_exit] = sys_exit,
    [SYS_wait4] = sys_wait4,
    [SYS_sigaction] = sys_sigaction,
    [SYS_sigprocmask] = sys_sigprocmask,
    [SYS_sigreturn] = sys_sigreturn,
    [SYS_uname] = sys_uname,
    [SYS_kill] = sys_kill,
    [SYS_test] = sys_test,
    [SYS_poweroff] = sys_poweroff,
    [SYS_yield] = sys_yield,
    [SYS_getuid] = sys_getuid,
    [SYS_getgid] = sys_getgid,
    [SYS_geteuid] = sys_geteuid,
    [SYS_getegid] = sys_getegid,
    [SYS_setuid] = sys_setuid,
    [SYS_setgid] = sys_setgid,
    [SYS_setreuid] = sys_setreuid,
    [SYS_setregid] = sys_setregid,
    [SYS_getgroups] = sys_getgroups,
    [SYS_setgroups] = sys_setgroups,
    [SYS_getpid] = sys_getpid,
    [SYS_getppid] = sys_getppid,
    
    [SYS_socket] = sys_socket,
    [SYS_bind] = sys_bind,
    [SYS_listen] = sys_listen,
    [SYS_accept] = sys_accept,
    [SYS_connect] = sys_connect,
    [SYS_sendmsg] = sys_sendmsg,
    [SYS_recvmsg] = sys_recvmsg,
    [SYS_sendto] = sys_sendto,
    [SYS_recvfrom] = sys_recvfrom,
};

#include <textos/panic.h>
#include <textos/task.h>

// todo : optimize

__INTR_HANDLER(syscall_handler)
{
    int nr = frame->rax;
    if (nr >= SYS_maxium)
        PANIC("syscall number was out of range!\n");

    task_current()->sframe = frame;

    DEBUGK(K_INIT, "syscall %d\n", nr);

    sys_func func = sys_handlers[nr];
    frame->rax = func(
        frame->rdi, frame->rsi,
        frame->rdx, frame->r10,
        frame->r8, frame->r9);
}

extern void msyscall_handler();

#define EFER_SCE (1 << 0)
#define SYS_STAR (0 | ((u64)(KERN_CODE_SEG << 3) << 32) | ((u64)(USER_CODE32_SEG << 3) << 48))

void syscall_init ()
{
    intr_register (INT_SYSCALL, syscall_handler); // 注册中断函数
    intr_setiattr (INT_SYSCALL, true);            // 用户态可用

    write_msr(MSR_STAR, SYS_STAR);
    write_msr(MSR_LSTAR, (u64)msyscall_handler);
    write_msr(MSR_FMASK, (1 << 9)); // disable irq
    write_msr(MSR_EFER, EFER_SCE);
}

