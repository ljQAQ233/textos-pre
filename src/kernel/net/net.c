/*
 * network uses big-endian byte order
 */

#include <textos/net.h>
#include <string.h>

#ifndef CONFIG_BYTE_BE

u16 htons(u16 h)
{
    return ((h & 0xFF00) >> 8) | ((h & 0x00FF) << 8);
}

u16 ntohs(u16 h)
{
    return ((h & 0xFF00) >> 8) | ((h & 0x00FF) << 8);
}

u32 htonl(u32 h)
{
    return ((h & 0xFF000000) >> 24) |
           ((h & 0x00FF0000) >> 8)  |
           ((h & 0x0000FF00) << 8)  |
           ((h & 0x000000FF) << 24);
}

u32 ntohl(u32 h)
{
    return ((h & 0xFF000000) >> 24) |
           ((h & 0x00FF0000) >> 8)  |
           ((h & 0x0000FF00) << 8)  |
           ((h & 0x000000FF) << 24);
}

#else

u16 htons(u16 h)
{
    return h;
}

u16 ntohs(u16 h)
{
    return h;
}

u32 htonl(u32 h)
{
    return h;
}

u32 ntohl(u32 h)
{
    return h;
}

#endif

void eth_addr_copy(mac_t addr1, mac_t addr2)
{
    memcpy(addr1, addr2, ETH_HLEN);
}

bool eth_addr_cmp(mac_t addr1, mac_t addr2)
{
    return memcmp(addr1, addr2, ETH_HLEN) == 0;
}

bool eth_addr_isany(mac_t addr)
{
    return memcmp(addr, ETH_ANY, ETH_HLEN) == 0;
}

void ip_addr_copy(ipv4_t addr1, ipv4_t addr2)
{
    *(u32 *)addr1 = *(u32 *)addr2;
}

bool ip_addr_cmp(ipv4_t addr1, ipv4_t addr2)
{
    u32 a1 = *(u32 *)addr1;
    u32 a2 = *(u32 *)addr2;
    return a1 == a2;
}

bool ip_addr_maskcmp(ipv4_t addr1, ipv4_t addr2, ipv4_t mask)
{
    u32 a1 = *(u32 *)addr1;
    u32 a2 = *(u32 *)addr2;
    u32 m = *(u32 *)mask;
    return (a1 & m) == (a2 & m);
}

bool ip_addr_isbroadcast(ipv4_t addr, ipv4_t mask)
{
    u32 a = *(u32 *)addr;
    u32 m = *(u32 *)mask;
    return (a & ~m) == (-1 & (~m)) || a == -1 || a == 0;
}

bool ip_addr_isany(ipv4_t addr)
{
    if (addr == NULL)
        return true;
    u32 a = *(u32 *)addr;
    return a == 0;
}

bool ip_addr_ismulticast(ipv4_t addr)
{
    u32 a = *(u32 *)addr;
    return (a & ntohl(0xF0000000)) == ntohl(0xE0000000);
}

#include <textos/errno.h>
#include <textos/net/socket.h>

int nif_ioctl(nif_t *nif, int req, void *argp)
{
    ifreq_t *ifr = argp;
    switch (req)
    {
    case SIOCGIFADDR:
        ip_addr_copy(ifr->addr, nif->ip);
        return 0;

    case SIOCSIFADDR:
        ip_addr_copy(nif->ip, ifr->addr);
        return 0;

    case SIOCGIFNETMASK:
        ip_addr_copy(ifr->addr, nif->netmask);
        return 0;

    case SIOCSIFNETMASK:
        ip_addr_copy(nif->ip, ifr->addr);
        return 0;

    case SIOCGIFBRDADDR:
        ip_addr_copy(nif->broadcast, ifr->broadaddr);
        return 0;
    
    case SIOCSIFBRDADDR:
        ip_addr_copy(ifr->broadaddr, nif->broadcast);
        return 0;

    case SIOCGIFHWADDR:
        eth_addr_copy(ifr->hwaddr, nif->mac);
        return 0;

    default:
        return -EINVAL;
    }

    return 0;
}

nif_t *nif0;

static list_t nifs = LIST_INIT(nifs);

void nif_register(nif_t *nif)
{
    list_insert(&nifs, &nif->nifs);
}

nif_t *nif_find(char *name)
{
    list_t *ptr;
    LIST_FOREACH(ptr, &nifs)
    {
        nif_t *n = CR(ptr, nif_t, nifs);
        if (strcmp(n->name, name) == 0)
            return n;
    }

    return NULL;
}