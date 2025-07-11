/*
 * /tmp is simple
 * TODO: tmpfs doesn't need its own tmpfs_entry. use vfs's instead!!!
 *       the precondition is that we complete reference count.
 */

#include <textos/fs.h>
#include <textos/file.h>
#include <textos/errno.h>
#include <textos/mm/vmm.h>
#include <textos/mm/heap.h>
#include <textos/klib/rbtree.h>
#include <textos/assert.h>

typedef struct tmpfs_page
{
    size_t idx;
    void *vpage;
    rbnode_t node;
} tmpfs_page_t;

typedef struct tmpfs_entry
{
    char *name;
    u64 ino;
    int mode;
    dev_t rdev;
    superblk_t *super;
    struct tmpfs_entry *parent;
    struct tmpfs_entry *subdir;
    struct tmpfs_entry *next;
    size_t filsz;
    rbtree_t pages;
} tmpfs_entry_t;

#include <textos/klib/string.h>
#include <textos/fs/inter.h>

static u64 __tmpfs_ino = 1;

_UTIL_CMP();

#define tmpfs_foreach(ent) \
    for (tmpfs_entry_t *ptr = ent ; ptr ; ptr = ptr->next)

static tmpfs_entry_t *tmp_ent_find(tmpfs_entry_t *dir, char *name)
{
    tmpfs_foreach(dir->subdir) {
        if (_cmp(ptr->name, name))
            return ptr;
    }
    return NULL;
}

static int tmpfs_ent_count(tmpfs_entry_t *dir)
{
    int count = 0;
    tmpfs_foreach(dir->subdir) {
        count++;
    }
    return count;
}

static void tmpfs_ent_regst(tmpfs_entry_t *ent, tmpfs_entry_t *prt)
{
    ent->next = prt->subdir;
    prt->subdir = ent;
    ent->parent = prt;
}

static void tmpfs_ent_unreg(tmpfs_entry_t *ent)
{
    tmpfs_entry_t **pp = &ent->parent->subdir;
    while (*pp && *pp != ent)
        pp = &(*pp)->next;
    if (*pp == ent)
        *pp = ent->next;
}

fs_opts_t __tmpfs_op;

static node_t *tmpfs_nodeget(tmpfs_entry_t *ent)
{
    unsigned ma = ent->super->dev->major;
    unsigned mi = ent->super->dev->minor;

    node_t *node = malloc(sizeof(node_t));
    node->name = strdup(ent->name);
    node->mode = ent->mode;
    node->siz = ent->filsz;
    node->ino = ent->ino;
    node->atime = arch_time_now();
    node->mtime = arch_time_now();
    node->ctime = arch_time_now();
    node->parent = NULL;
    node->child = NULL;
    node->next = NULL;
    node->dev = makedev(ma, mi);
    node->rdev = ent->rdev;
    node->pdata = ent;
    node->sb = ent->super;
    node->mount = NULL;
    node->opts = &__tmpfs_op;
    return node;
}

/*
 * FIXME : name may include '/' or other characters, strip them later!!!
 *         all physical file system interfaces of vrtfs has this bug!!!
 */
static int tmpfs_open(node_t *parent, char *name, u64 args, int mode, node_t **result)
{
    node_t *node = NULL;
    tmpfs_entry_t *dir = parent->pdata;
    tmpfs_entry_t *ent = tmp_ent_find(dir, name);
    if (ent != NULL)
        goto end;

    if (~args & O_CREAT) {
        *result = NULL;
        return -ENOENT;
    }

    if (args & O_DIRECTORY)
        mode |= S_IFDIR;
    else
        mode |= S_IFREG;

    ent = malloc(sizeof(tmpfs_entry_t));
    ent->name = strdup(name);
    ent->ino = __tmpfs_ino++;
    ent->mode = mode;
    ent->super = dir->super;
    ent->subdir = NULL;
    ent->filsz = 0;
    ent->pages.root = NULL;
    tmpfs_ent_regst(ent, dir);

end:
    node = tmpfs_nodeget(ent);
    vfs_regst(node, parent);
    *result = node;
    return 0;
}

int tmpfs_mknod(node_t *parent, char *name, dev_t rdev, int mode, node_t **result)
{
    tmpfs_entry_t *dir = parent->pdata;
    tmpfs_entry_t *ent = malloc(sizeof(tmpfs_entry_t));
    ent->name = strdup(name);
    ent->ino = __tmpfs_ino++;
    ent->mode = mode;
    ent->rdev = rdev;
    ent->super = dir->super;
    ent->subdir = NULL;
    ent->filsz = 0;
    ent->pages.root = NULL;
    tmpfs_ent_regst(ent, dir);
    
    node_t *node = tmpfs_nodeget(ent);
    vfs_regst(node, parent);
    *result = node;
    return 0;
}

/**
 * @brief get a page which stores a part of a file from rbtree
 */
static tmpfs_page_t *getblk(tmpfs_entry_t *ent, size_t idx)
{
    ASSERTK(S_ISREG(ent->mode));

    rbtree_t *t = &ent->pages;
    rbnode_t **pp = &t->root;
    while (*pp) {
        tmpfs_page_t *pg = CR(*pp, tmpfs_page_t, node);
        if (pg->idx < idx)
            pp = &(*pp)->left;
        else if (pg->idx > idx)
            pp = &(*pp)->right;
        else
            return pg;
    }
    return NULL;
}

static void insert(tmpfs_entry_t *ent, tmpfs_page_t *ipg)
{
    rbtree_t *t = &ent->pages;
    rbnode_t *p = NULL;
    rbnode_t **pp = &t->root;
    while (*pp) {
        p = *pp;
        tmpfs_page_t *pg = CR(*pp, tmpfs_page_t, node);
        if (pg->idx < ipg->idx)
            pp = &(*pp)->left;
        else if (pg->idx > ipg->idx)
            pp = &(*pp)->right;
        else
            __builtin_unreachable();
    }

    rbtree_link(&ipg->node, pp, p);
    rbtree_fixup(t, &ipg->node);
}

/**
 * @brief extends a file. i.e. allocate free pages. used only when req >= has!!!
 * @retval size_t the size extended to
 */
static size_t extend(tmpfs_entry_t *ent, size_t siz)
{
    size_t req = DIV_ROUND_UP(siz, PAGE_SIZ);
    size_t has = DIV_ROUND_UP(ent->filsz, PAGE_SIZ);
    ASSERTK(req >= has);
    if (has == req)
        return ent->filsz = siz;
    
    // TODO: extending limitlessly is not allowed!
    size_t rem = req - has;
    void *vpgs = vmm_allocpages(rem, PE_P | PE_RW);
    while (rem)
    {
        tmpfs_page_t *pg = malloc(sizeof(tmpfs_page_t));
        pg->idx = has++;
        pg->vpage = vpgs;
        insert(ent, pg);
        vpgs += PAGE_SIZ;
        rem -= 1;
    }
    return ent->filsz = siz;
}

// TODO
static int tmpfs_ioctl(node_t *this, int req, void *argp)
{
    return 0;
}

static int tmpfs_chown(node_t *this, uid_t owner, gid_t group, bool ap)
{
    return vfs_m_chown(this, owner, group, ap);
}

static int tmpfs_chmod(node_t *this, mode_t mode, bool clrsgid)
{
    return vfs_m_chmod(this, mode, clrsgid);
}

static int tmpfs_close(node_t *this)
{
    return 0;
}

static int tmpfs_remove(node_t *this)
{
    tmpfs_entry_t *ent = this->pdata;
    if (ent->subdir)
        return -ENOTEMPTY;
    tmpfs_ent_unreg(ent);
    free(ent);
    return 0;
}

static int tmpfs_read(node_t *this, void *buf, size_t siz, size_t offset)
{
    if (siz == 0)
        return 0;
    if (offset >= this->siz)
        return 0;
    
    // adjust to real size
    siz = MIN(this->siz, offset + siz) - offset;
    tmpfs_entry_t *ent = this->pdata;
    size_t rem = siz;
    size_t pidx = offset / PAGE_SIZ; // page index
    size_t boff = offset % PAGE_SIZ; // byte offset
    while (rem)
    {
        tmpfs_page_t *pg = getblk(ent, pidx);
        size_t cpysiz = MIN(rem, boff != 0 ? PAGE_SIZ - boff : MIN(rem, PAGE_SIZ));
        memcpy(buf, pg->vpage + boff, cpysiz);
        boff = 0;
        rem -= cpysiz;
        buf += cpysiz;
        pidx += 1;
    }
    this->atime = arch_time_now();
    return siz - rem;
}

static int tmpfs_write(node_t *this, void *buf, size_t siz, size_t offset)
{
    tmpfs_entry_t *ent = this->pdata;
    size_t maxpos = siz + offset;
    if (maxpos > this->siz)
        this->siz = extend(ent, maxpos);
    
    // because of extending, real size is not used yet not
    size_t rem = siz;
    size_t pidx = offset / PAGE_SIZ; // page index
    size_t boff = offset % PAGE_SIZ; // byte offset
    while (rem)
    {
        tmpfs_page_t *pg = getblk(ent, pidx);
        size_t cpysiz = MIN(rem, boff != 0 ? PAGE_SIZ - boff : MIN(rem, PAGE_SIZ));
        memcpy(pg->vpage + boff, buf, cpysiz);
        boff = 0;
        rem -= cpysiz;
        buf += cpysiz;
        pidx += 1;
    }
    this->atime = arch_time_now();
    return siz - rem;
}

static int tmpfs_truncate(node_t *this, size_t len)
{
    if (len == this->siz)
        return 0;

    tmpfs_entry_t *ent = this->pdata;
    if (len > this->siz) {
        this->siz = extend(ent, len);
    } else {
        size_t pidx = DIV_ROUND_UP(len, PAGE_SIZ);
        size_t end = DIV_ROUND_UP(this->siz, PAGE_SIZ);
        while (pidx <= end)
        {
            tmpfs_page_t *pg = getblk(ent, pidx);
            rbtree_delete(&ent->pages, &pg->node);
            pidx++;
        }
        this->siz = len;
    }
    return 0;
}

static inline void init_ctx(dirctx_t *ctx, node_t *dir)
{
    ctx->sb = dir->sb;
    ctx->node = dir;
    ctx->pos = 0;
    ctx->stat = ctx_pre;
}

static int tmpfs_readdir(node_t *node, dirctx_t *ctx)
{
    if (ctx->stat == ctx_inv)
        init_ctx(ctx, node);
    if (ctx->stat == ctx_end)
        return EOF;

    if (ctx->pos == 0)
    {
        if (!dir_emit_dot(ctx))
            goto end;
        ctx->pos++;
    }
    if (ctx->pos == 1)
    {
        if (!dir_emit_dotdot(ctx))
            goto end;
        ctx->pos++;
    }
    
    tmpfs_entry_t *dir = node->pdata;
    tmpfs_foreach(dir->subdir) {
        if (!dir_emit(ctx, ptr->name, strlen(ptr->name), ptr->ino, dir_get_type(ptr->mode)))
            goto end;
        ctx->pos++;
    }
    ctx->stat = ctx_end;

end:
    node->atime = arch_time_now();

    return 0;
}

static int tmpfs_seekdir(node_t *this, dirctx_t *ctx, size_t *pos)
{
    tmpfs_entry_t *dir = this->pdata;
    size_t subsz = tmpfs_ent_count(dir);
    if (subsz + 1 >= *pos)
    {
        ctx->stat = ctx_pre;
        ctx->pos = *pos;
        return 0;
    }

    ctx->stat = ctx_end;
    ctx->pos = subsz + 1;
    return EOF;
}

/*
 * TODO: may be the easiest mmap...
 */
static void *tmpfs_mmap(node_t *this, vm_region_t *vm)
{
    return (void *)((addr_t)-EINVAL);
}

/*
 * tmpfs initialization
 */
node_t *__fs_init_tmpfs()
{
    devst_t *anony = dev_new();
    dev_register_anony(anony);
    superblk_t *sb = malloc(sizeof(superblk_t));
    sb->blksz = PAGE_SIZ;
    sb->dev = anony;
    sb->root = NULL;
    sb->op = &__tmpfs_op;
    sb->sbi = NULL;

    tmpfs_entry_t *root = malloc(sizeof(tmpfs_entry_t));
    root->name = "";
    root->ino = __tmpfs_ino++;
    root->mode = S_IFDIR | 0555;
    root->super = sb;
    root->parent = NULL;
    root->subdir = NULL;
    root->next = NULL;

    return sb->root = tmpfs_nodeget(root);
}

fs_opts_t __tmpfs_op = {
    tmpfs_open,
    tmpfs_mknod,
    tmpfs_chown,
    tmpfs_chmod,
    tmpfs_remove,
    tmpfs_readdir,
    tmpfs_seekdir,
    tmpfs_read,
    tmpfs_write,
    tmpfs_truncate,
    tmpfs_mmap,
    tmpfs_ioctl,
    tmpfs_close,
};