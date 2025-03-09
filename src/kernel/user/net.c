#include <textos/syscall.h>
#include <textos/net/socket.h>

RETVAL(int) sys_socket(int domain, int type, int proto)
{
    return socket(domain, type, proto);
}

RETVAL(ssize_t) sys_sendmsg(int fd, msghdr_t *msg, int flags)
{
    return sendmsg(fd, msg, flags);
}

RETVAL(ssize_t) sys_recvmsg(int fd, msghdr_t *msg, int flags)
{
    return recvmsg(fd, msg, flags);
}

RETVAL(ssize_t) sys_sendto(int fd, void *buf, size_t len, int flags, sockaddr_t *dst, size_t dlen)
{
    return sendto(fd, buf, len, flags, dst, dlen);
}

RETVAL(ssize_t) sys_recvfrom(int fd, void *buf, size_t len, int flags, sockaddr_t *src, size_t slen)
{
    return recvfrom(fd, buf, len, flags, src,slen);
}
