#ifndef __ERRNO_H__
#define __ERRNO_H__

#define EPERM        1   // Operation not permitted
#define ENOENT       2   // No that file or dir
#define ESRCH        3   // No such process
#define EBADF        9   // Bad file descriptor
#define ENOEXEC      8   // Exec format error
#define ENOMEM       12  // No memory to allocate
#define EACCES       13  // Permission denied
#define EFAULT       14  // Bad address
#define ENOBLK       15  // File is not block device
#define EEXIST       17  // File already exists
#define	ENOTDIR      20  // Not a directory
#define	EISDIR       21  // Is a directory
#define	EINVAL       22  // Invalid argument
#define ENFILE       23  // 
#define EMFILE       24  // Number of fd of this process was out of range
#define ENOSPC       28  // No space left on device
#define ERANGE       34  // Result not representable
#define ENAMETOOLONG 36  // Filename too long
#define ENOSYS       38  // Function not implemented
#define ENOTEMPTY    39  // Directory not empty
#define EDESTADDRREQ 89  // Destination address required
#define EADDRINUSE   98  // Address in use
#define ECONNRESET   104 // Connection reset by peer
#define EISCONN      106
#define ETIMEDOUT    110
#define ECONNREFUSED 111

#endif
