#include <textos/dev.h>
#include <textos/mm.h>
#include <textos/panic.h>
#include <textos/klib/list.h>

#include <string.h>

/*
  The flow to register:
    1. dev_new() to create new buffer
    2. Set it by device initializer itself
    3. dev_register() to insert into the root list
*/

static void inv_handle () { PANIC ("this opts is not supported!"); };

static dev_t devroot = {
    .name = "dev root"
};

static dev_pri_t devs = {
    .dev = &devroot,
    .list = LIST_INIT(devs.list),
};

extern void __dev_initmem();

void dev_init ()
{
    __dev_initmem();

    dev_list();
}

#include <textos/fs.h>
#include <textos/args.h>
#include <textos/klib/vsprintf.h>

#define FOREACH_DEV() \
        for (list_t *i = devs.list.back ; i != &devs.list ; i = i->back)

// called recursively
static void initnod(dev_t *dev)
{
    char path[128];
    sprintf(path, "/dev/%s", dev->name);
    vfs_mknod(path, dev);

    for (list_t *p = &dev->subdev ; p != &dev->subdev ; p = p->forward)
        initnod(CR(p, dev_t, subdev));
}

void dev_initnod()
{
    node_t *dir;
    vfs_open(NULL, &dir, "/dev", VFS_CREATE | VFS_DIR);
    FOREACH_DEV() {
        initnod(CR(i, dev_pri_t, list)->dev);
    }
}

void __dev_register (dev_pri_t *pri)
{
    if (pri->dev->read == NULL)
        pri->dev->read = (void *)inv_handle;
    if (pri->dev->write == NULL)
        pri->dev->write = (void *)inv_handle;
    
    list_insert_tail (&devs.list, &pri->list);
}

static int applyid(dev_t *prt)
{
    static int total;
    if (!prt)
        return total++;
    return CR(&prt->subdev.back, dev_t, subdev)->minor + 1;
}

void dev_register (dev_t *prt, dev_t *dev)
{
    if (prt != NULL)
    {
        int minor = applyid(prt);
        dev->major = prt->major;
        dev->minor = prt->minor;
        list_insert(&prt->subdev, &dev->subdev);
        return;
    }

    int major = applyid(NULL);
    dev->major = major;
    dev->minor = 0;
    list_init(&dev->subdev);

    dev_pri_t *pri = malloc(sizeof(dev_pri_t));
    pri->dev = dev;

    __dev_register (pri);
}

dev_t *dev_new ()
{
    dev_t *d = malloc(sizeof(dev_t));
    
    d->read = (void *)inv_handle;
    d->write = (void *)inv_handle;
    d->bread = (void *)inv_handle;
    d->bwrite = (void *)inv_handle;
    
    return d;
}

dev_t *dev_lookup_type (int subtype, int idx)
{
    FOREACH_DEV() {
        dev_pri_t *pri = CR(i, dev_pri_t, list);
        if (pri->dev->subtype == subtype)
            if (idx-- == 0)
                return pri->dev;
    }

    return NULL;
}

dev_t *dev_lookup_name (const char *name)
{
    FOREACH_DEV() {
        dev_pri_t *Pri = CR(i, dev_pri_t, list);
        if (strcmp (Pri->dev->name, name) == 0)
            return Pri->dev;
    }

    return NULL;
}

dev_t *dev_lookup_nr (int major, int minor)
{
    for (list_t *lm = &devs.list ; lm != &devs.list ; lm = lm->back) {
        dev_pri_t *mp = CR(lm, dev_pri_t, list);
        if (mp->dev->major != major)
            continue;
        if (minor == 0)
            return mp->dev;
        for (list_t *ls = &mp->dev->subdev ; ls != &mp->dev->subdev ; ls = ls->back) {
            dev_t *sd = CR(ls, dev_t, subdev);
            if (sd->minor == minor)
                return sd;
        }
    }

    return NULL;
}

#include <textos/printk.h>

static char *dev_typestr (int type)
{
    if (type == DEV_CHAR)
        return "character device";
    if (type == DEV_BLK)
        return "block device";

    return "unknown device";
}

void dev_list ()
{
    int idx = 0;

    FOREACH_DEV() {
        dev_pri_t *pri = CR (i, dev_pri_t, list);
        printk ("dev index - %04d -> %s\n"  , idx, pri->dev->name);
        printk ("            type -> %s\n"  , dev_typestr(pri->dev->type));
        printk ("            opts -> %d%d\n",
                pri->dev->read == (void *)inv_handle ? 0 : 1,
                pri->dev->write == (void *)inv_handle ? 0 : 1);

        idx++;
    }
}

