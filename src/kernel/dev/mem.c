#include <textos/dev.h>

#include <string.h>

// EOF
static int null_read(devst_t *dev, void *buf, size_t cnt)
{
    memset(buf, EOF, cnt);
    return 0;
}

// drop!!!
static int null_write(devst_t *dev, void *buf, size_t cnt)
{
    return 0;
}

// 0
static int zero_read(devst_t *dev, void *buf, size_t cnt)
{
    memset(buf, 0, cnt);
    return cnt;
}

// drop
static int zero_write(devst_t *dev, void *buf, size_t cnt)
{
    return 0;
}

// todo : full

static devst_t null = {
    .name = "null",
    .read = null_read,
    .write = null_write,
    .type = DEV_CHAR,
    .subtype = DEV_NULL,
    .minor = 3,
};

static devst_t zero = {
    .name = "zero",
    .read = zero_read,
    .write = zero_write,
    .type = DEV_CHAR,
    .subtype = DEV_ZERO,
    .minor = 5
};

static devst_t mem = {
    .name = "mem",
    .type = DEV_CHAR,
    .subtype = DEV_MEMORY,
};

void __dev_initmem()
{
    // todo : major nr
    // null / zero
    dev_register(NULL, &mem);
    dev_register(&mem, &null);
    dev_register(&mem, &zero);
}
