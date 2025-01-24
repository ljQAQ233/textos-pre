#include <textos/fs.h>
#include <textos/dev.h>
#include <textos/args.h>
#include <textos/klib/vsprintf.h>

#include <string.h>

int part_read(dev_t *dev, u32 addr, void *buf, u8 cnt)
{
    dev_t *prt = dev_lookup_nr(dev->major, 0);
    cnt = MIN(cnt, dev->ptend - addr);
    return prt->bread(prt, addr + dev->ptoff, buf, cnt);
}

int part_write(dev_t *dev, u32 addr, void *buf, u8 cnt)
{
    dev_t *prt = dev_lookup_nr(dev->major, 0);
    cnt = MIN(cnt, dev->ptend - addr);
    return prt->bwrite(prt, addr + dev->ptoff, buf, cnt);
}

dev_t *register_part(dev_t *disk, int nr, addr_t ptoff, size_t ptsiz)
{
    char name[128];
    char path[128];
    disk->mkname(disk, name, nr);
    sprintf(path, "/dev/%s", name);
    
    dev_t *part = dev_new();
    strcpy(part->name, name);
    part->type = DEV_BLK;
    part->subtype = DEV_PART;
    part->bread = (void *)part_read;
    part->bwrite = (void *)part_write;
    part->ptoff = ptoff;
    part->ptend = ptoff + ptsiz;
    dev_register(disk, part);

    return part;
}
