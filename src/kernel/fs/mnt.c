#include <textos/fs.h>
#include <textos/mm.h>
#include <textos/klib/stack.h>

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
    dir->child = stack_top(&mnt->chd),
                 stack_pop(&mnt->chd);
    if (stack_siz(&mnt->chd) == 0)
        dir->attr &= ~NA_MNT;

    return 0;
}