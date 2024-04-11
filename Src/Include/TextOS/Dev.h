#ifndef __DEV_H__
#define __DEV_H__

enum DevType {
    DEV_CHAR,
    DEV_BLK,
};

enum SubType {
    DEV_KBD,    /* PS/2 Keyboard  */
    DEV_SERIAL, /* Serial port    */
    DEV_KNCON,  /* Kernel console */

    /* The next devices are block devices */

    DEV_IDE,
};

struct _Dev;
typedef struct _Dev Dev_t;

struct _Dev {
    char  *Name;
    int    Type;
    int    SubType;
    union {
        struct {
            int   (*Write)(Dev_t *This, void *Buffer, size_t Count);
            int   (*Read)(Dev_t *This, void *Buffer, size_t Count);
        };
        struct {
            int   (*BlkWrite)(Dev_t *This, u64 Addr, void *Buffer, size_t Count);
            int   (*BlkRead)(Dev_t *This, u64 Addr, void *Buffer, size_t Count);
        };
    };

    /* TODO : device isolation 设备隔离 */

    void *Private; /* 在调用 操作函数 时传入, 以支持同种类型的多种设备 */
};

enum Lkup {
    LKUP_TYPE,
    LKUP_NAME,
    LKUP_ID,
};

Dev_t *DevNew ();

void DevRegister (Dev_t *Dev);

Dev_t *DevLookupByType (int Type, int SubType);

Dev_t *DevLookupByName (const char *Name);

Dev_t *DevLookupByID (int Ident);

#endif
