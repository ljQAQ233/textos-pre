#include <textos/fs.h>
#include <textos/mm.h>
#include <textos/dev.h>
#include <textos/klib/stack.h>
#include <textos/dev/buffer.h>


#include <string.h>

fs_opts_t __dev_opts;

// mount overlay
typedef struct
{
    stack_t chd;
} mount_t;

int vfs_mount(node_t *dir, node_t *root)
{
    if (!dir->mount)
        dir->mount = malloc(sizeof(mount_t));

    mount_t *mnt = dir->mount;
    stack_push(&mnt->chd, dir->child);
    dir->child = root;
    dir->attr |= NA_MNT;
    root->parent = dir->parent;
    return 0;
}

int vfs_umount(node_t *dir)
{
    mount_t *mnt = dir->mount;
    node_t *root = dir->child;
    root->parent = root;
    dir->child = stack_top(&mnt->chd);
    stack_pop(&mnt->chd);
    if (stack_siz(&mnt->chd)  == 0)
        dir->attr &= ~NA_MNT;

    return 0;
}

static int dev_read(node_t *this, void *buf, size_t siz, size_t offset)
{
    dev_t *dev = this->pdata;
    if (dev->type == DEV_CHAR)
        return dev->read(dev, buf, siz);

    int blksiz = 512;
    int blk = offset / blksiz;
    int off = offset % blksiz;

    // 在 fat32 中写过这样的代码
    // off 是 src 在块中的偏移, 只有第一个读取的块可能 off != 0
    // 每一次最大读取的字节数就是块的大小
    // 对于最后一个块, 可能不会完全读取, 所以取最小 siz
    buffer_t *b;
    for (int rem = siz ; rem ; blk++) {
        b = bread(dev, blk);

        int cpy = MIN(off != 0 ? blksiz - off : blksiz, rem);
        memcpy(buf, b->blk + off, cpy);
        rem -= cpy;
        off = 0;

        brelse(b);
    }
    return siz;
}

static int dev_write(node_t *this, void *buf, size_t siz, size_t offset)
{
    dev_t *dev = this->pdata;
    if (dev->type == DEV_CHAR)
        return dev->write(dev, buf, siz);

    int blksiz = 512;
    int blk = offset / blksiz;
    int off = offset % blksiz;

    buffer_t *b;
    for (int rem = siz ; rem ; blk++) {
        b = bread(dev, blk);

        int cpy = MIN(off != 0 ? blksiz - off : blksiz, rem);
        memcpy(b->blk + off, buf, cpy);
        rem -= cpy;
        off = 0;

        bdirty(b, true);
        brelse(b);
    }
    return siz;
}

void __vrtdev_init()
{
    vfs_initops(&__dev_opts);
    __dev_opts.read = (void *)dev_read;
    __dev_opts.write = (void *)dev_write;
}
