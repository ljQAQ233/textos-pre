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

void dev_init ()
{
    dev_list();
}

void __dev_register (dev_pri_t *pri)
{
    if (pri->dev->read == NULL)
        pri->dev->read = (void *)inv_handle;
    if (pri->dev->write == NULL)
        pri->dev->write = (void *)inv_handle;
    
    list_insert_tail (&devs.list, &pri->list);
}

void dev_register (dev_t *dev)
{
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

#define FOREACH_DEV() \
        for (list_t *i = devs.list.back ; i != &devs.list ; i = i->back)

dev_t *dev_lookup_type (int type, int subtype)
{
    FOREACH_DEV() {
        dev_pri_t *pri = CR(i, dev_pri_t, list);
        if (pri->dev->type != type)
            continue;
        if (pri->dev->subtype == subtype)
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

dev_t *dev_lookup_id (int Ident)
{
    PANIC ("Look up by LKUP_ID has not been implented yet\n");

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

