#include <textos/task.h>
#include <textos/file.h>
#include <textos/errno.h>
#include <textos/assert.h>
#include <textos/fs/pipe.h>

#include <textos/mm.h>

static int get_fd()
{
    file_t **ft = task_current()->files;
    for (int i = 0 ; i < MAX_FILE ; i++)
    {
        if (!ft[i])
            return i;
    }
    return -1;
}

static int get_free(int *new, file_t **file)
{
    file_t **ft = task_current()->files;
    int fd = get_fd();
    if (!(ft[fd] = malloc(sizeof(file_t))))
        return -1;

    *file = ft[fd];
    return *new = fd;
}

// TODO: flgs
static inline u64 parse_args(int x)
{
    int v = 0;
    if (x & O_CREAT)
        v |= VFS_CREATE;

    return v;
}

int open(char *path, int flgs)
{
    node_t *node;
    file_t *file;
    u64 opargs = parse_args(flgs);
    
    int ret;
    if ((ret = vfs_open(task_current()->pwd, &node, path, opargs)) < 0)
        return ret;

    int fd;
    if (get_free(&fd, &file) < 0)
        return -EMFILE;

    file->refer = 1;
    file->offset = 0;
    file->node = node;
    file->flgs = flgs;

fail:
    return fd;
}

// todo: max size limited
ssize_t write(int fd, void *buf, size_t cnt)
{
    file_t *file = task_current()->files[fd];
    if (!file)
        return -EBADF;

    int accm = file->flgs & O_ACCMODE;
    if (accm == O_RDONLY)
        return -EBADF;

    int ret = file->node->opts->write(file->node, buf, cnt, file->offset);
    if (ret < 0)
        return ret;
    
    file->offset += cnt;
    return cnt;
}

ssize_t read(int fd, void *buf, size_t cnt)
{
    file_t *file = task_current()->files[fd];
    if (!file)
        return -EBADF;

    int accm = file->flgs & O_ACCMODE;
    if (accm == O_WRONLY)
        return -EBADF;

    int ret = file->node->opts->read(file->node, buf, cnt, file->offset);
    if (ret < 0)
        return -1;
    
    file->offset += cnt;
    return cnt;
}

int close(int fd)
{
    file_t *file = task_current()->files[fd];
    if (!file)
        return -EBADF;

    if (--file->refer > 0) {
        task_current()->files[fd] = NULL;
        return 0;
    }

    int ret = file->node->opts->close(file->node);
    
    free(file);
    task_current()->files[fd] = NULL;
    return ret;
}

int dup2(int old, int new)
{
    file_t **ft = task_current()->files;
    file_t *file = ft[old];
    if (!file)
        return -EBADF;

    if (old == new)
        return -EINVAL;

    // if newfd exists, close it first
    if (ft[new])
        close(new);

    file->refer++;
    ft[new] = file;
    return new;
}

int dup(int fd)
{
    int new = get_fd();
    if (new < 0)
        return -EMFILE;

    return dup2(fd, new);
}

int pipe(int fds[2])
{
    int fd0, fd1;
    file_t *f0, *f1;
    ASSERTK(get_free(&fd0, &f0) >= 0);
    ASSERTK(get_free(&fd1, &f1) >= 0);

    node_t *n = malloc(sizeof(*n));
    ASSERTK(n != NULL);

    pipe_init(n);

    f0->node = n;
    f1->refer = 1;
    f0->spec = S_PIPE_R;
    f0->flgs = O_RDONLY;

    f1->node = n;
    f0->refer = 1;
    f1->flgs = O_WRONLY;
    f1->spec = S_PIPE_W;

    fds[0] = fd0;
    fds[1] = fd1;
}
