#include <textos/file.h>

int sys_open(char *path, int flgs)
{
    return open(path, flgs);
}

ssize_t sys_write(int fd, void *buf, size_t cnt)
{
    return write(fd, buf, cnt);
}

ssize_t sys_read(int fd, void *buf, size_t cnt)
{
    return read(fd, buf, cnt);
}

int sys_close(int fd)
{
    return close(fd);
}

int sys_dup(int fd)
{
    return dup(fd);
}

int sys_dup2(int old, int new)
{
    return dup2(old, new);
}
