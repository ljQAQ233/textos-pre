#include <textos/video.h>

static u32 hor, ver;
static u32 *fb;
static u64 fb_siz;

void pixel_put (
        u32 x,u32 y,
        u32 color
        )
{
    if (x > hor || y > hor) {
        return;
    }

    u32 *pixel = fb + x + hor * y;
    *pixel = color;
}

void block_put (
        u32 x,u32 y,
        u32 xe,u32 ye,
        u32 color
        )
{
    if (x > xe) {
        u32 tmp = x;
        x = xe;
        xe = tmp;
    }

    if (y > ye) {
        u32 tmp = y;
        y = ye;
        ye = tmp;
    }

    for (u32 i = x ; i < xe && i < hor ; i++)
        for (u32 j = y ; j < ye && j < ver ; j++)
            pixel_put (i, j, color);
}

void screen_clear ()
{
    u32 i = fb_siz / 4;
    u32 *p = fb;

    while (i-- && p)
        *p++ = 0;
}

#include <boot.h>

void __video_pre (vconfig_t *v)
{
    hor = v->hor;
    ver = v->ver;

    fb = (void *)v->fb;
    fb_siz = v->fb_siz;
}
