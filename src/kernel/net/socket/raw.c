// socket raw provides a interface to operate iphdr directly

#include <textos/mm.h>
#include <textos/task.h>
#include <textos/net.h>
#include <textos/net/ip.h>
#include <textos/net/socket.h>
#include <string.h>

#include "inter.h"
#include <textos/panic.h>
#include <textos/assert.h>

typedef struct
{
    bool hdrincl;
} raw_t;

static list_t intype = LIST_INIT(intype);

// if it doesn't have a iphdr, kernel will build one
static bool hdrincl(socket_t *s)
{
    raw_t *r = s->pri;
    return r->hdrincl
        || s->proto == IPPROTO_RAW;
}

static int raw_socket(socket_t *s)
{
    ASSERTK(s->domain == AF_INET);
    ASSERTK(s->type == SOCK_RAW);

    raw_t *r;
    r = s->pri = malloc(sizeof(raw_t));
    r->hdrincl = false; // iphdr not provided by user

    list_insert(&intype, &s->intype);
    return 0;
}

static ssize_t raw_sendmsg(socket_t *s, msghdr_t *msg, int flags)
{
    mbuf_t *m = mbuf_alloc(MBUF_DEFROOM);
    void *data = msg->iov[0].base;
    size_t len = msg->iov[0].len;

    void *payload = mbuf_put(m, len);
    memcpy(payload, data, len);

    sockaddr_in_t *in = (sockaddr_in_t *)msg->name;
    if (hdrincl(s))
    {
        // not supported yet
        PANIC("hdr included!!!\n");
    }
    else
    {
        net_tx_ip(nic0, m, in->addr, s->proto);
    }
    return len;
}

#include <irq.h>

static void block_as(lock_t *lock, int *as)
{
    // only one task is supported
    ASSERTK(*as == -1);

    *as = task_current()->pid;
    lock_release(lock);
    task_block();
    lock_acquire(lock);
    *as = -1;
}

// TODO: timeout
static ssize_t raw_recvmsg(socket_t *s, msghdr_t *msg, int flags)
{
    lock_acquire(&s->lock);

    // wait for input
    if (list_empty(&s->rx_queue))
    {
        block_as(&s->lock, &s->rx_waiter);
    }

    list_t *ptr = s->rx_queue.next;
    mbuf_t *m = CR(ptr, mbuf_t, list);
    list_remove(ptr);
    int len = MIN(msg->iov[0].len, m->len);
    memcpy(msg->iov[0].base, m->head, len);
    lock_release(&s->lock);
    return len;
}

// m includes iphdr
// retval 1 means that m has been handled
int sock_rx_raw(iphdr_t *hdr, mbuf_t *m)
{
    int ret = 0;

    list_t *ptr;
    LIST_FOREACH(ptr, &intype)
    {
        socket_t *s = CR(ptr, socket_t, intype);
        if ((u8)s->proto != IPPROTO_RAW)
        {
            if ((u8)s->proto != hdr->ptype)
                continue;
        }
        
        mbuf_pushhdr(m, iphdr_t);

        lock_acquire(&s->lock);
        list_insert(&s->rx_queue, &m->list);
        if (s->rx_waiter >= 0)
            task_unblock(s->rx_waiter);
        lock_release(&s->lock);

        ret = 1;
        break;
    }
    
    // XXX: we assume m is constant
    return ret;
}

static sockop_t op = {
    .socket = raw_socket,
    .sendmsg = raw_sendmsg,
    .recvmsg = raw_recvmsg,
};

void sock_raw_init()
{
    sockop_set(SOCK_T_RAW, &op);
}

