/*
 * Copyright (C) 2018-2020 Alibaba Group Holding Limited
 */

#include "avutil/named_sfifo_cls.h"
#include "avutil/straight_fifo.h"
#ifdef __linux__
#include "aos_port/list.h"
#else
#include <aos/list.h>
#endif

struct nsfifo_in {
    int                ref;
    char               *name;
    sfifo_t            *sfifo;
    dlist_t            node;
};


struct nsfifo_in_priv {
    struct nsfifo_in *fifo_in;
};

#define GET_DERIVED_NSFIFO_IN(p) ((struct nsfifo_in_priv*)GET_DERIVED_NSFIFO(p))

static struct {
    int                init;
    dlist_t            head;
    aos_mutex_t        lock;
} g_fifo_list;

#define TAG                    "nsfifo_in"

static int _nsfifo_in_init(nsfifo_t *fifo, const char *name, int mode)
{
    sfifo_t *sfifo = NULL;
    struct nsfifo_in *fifo_in = NULL;
    dlist_t *node  = &g_fifo_list.head;
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    if (!g_fifo_list.init) {
        dlist_init(&g_fifo_list.head);
        aos_mutex_new(&g_fifo_list.lock);
        g_fifo_list.init = 1;
    }

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    if (!(mode & O_CREAT)) {
        /* read only */
        for (node = node->next; node != &g_fifo_list.head; node = node->next) {
            fifo_in = list_entry(node, struct nsfifo_in, node);
            if (!strcmp(fifo_in->name, name)) {
                fifo_in->ref++;
                priv->fifo_in = fifo_in;
                aos_mutex_unlock(&g_fifo_list.lock);

                return 0;
            }
        }

        goto err;
    }
    sfifo = sfifo_create(fifo->size);

    fifo_in =(struct nsfifo_in*)av_zalloc(sizeof(struct nsfifo_in));
    fifo_in->ref   = 1;
    fifo_in->sfifo = sfifo;
    fifo_in->name  = strdup(name);

    priv->fifo_in = fifo_in;
    dlist_add_tail(&fifo_in->node, &g_fifo_list.head);
    aos_mutex_unlock(&g_fifo_list.lock);

    return 0;
err:
    aos_mutex_unlock(&g_fifo_list.lock);
    sfifo_destroy(sfifo);
    av_free(fifo_in);
    return -1;
}

static int _nsfifo_in_get_rpos(nsfifo_t *fifo, char **pos, uint32_t timeout)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_get_rpos(priv->fifo_in->sfifo, pos, timeout);
}

static int _nsfifo_in_set_rpos(nsfifo_t* fifo, size_t count)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_set_rpos(priv->fifo_in->sfifo, count);
}

static int _nsfifo_in_get_wpos(nsfifo_t* fifo, char **pos, uint32_t timeout)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_get_wpos(priv->fifo_in->sfifo, pos, timeout);
}

static int _nsfifo_in_set_wpos(nsfifo_t* fifo, size_t count)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_set_wpos(priv->fifo_in->sfifo, count);
}

static int _nsfifo_in_set_eof(nsfifo_t* fifo, uint8_t reof, uint8_t weof)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_set_eof(priv->fifo_in->sfifo, reof, weof);
}

static int _nsfifo_in_get_eof(nsfifo_t* fifo, uint8_t *reof, uint8_t *weof)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_get_eof(priv->fifo_in->sfifo, reof, weof);
}

static int _nsfifo_in_reset(nsfifo_t *fifo)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_reset(priv->fifo_in->sfifo);
}

static int _nsfifo_in_get_len(nsfifo_t *fifo)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);

    return sfifo_get_len(priv->fifo_in->sfifo);
}

static void _nsfifo_in_uninit(nsfifo_t *fifo)
{
    struct nsfifo_in_priv *priv = GET_DERIVED_NSFIFO_IN(fifo);
    struct nsfifo_in *fifo_in   = priv->fifo_in;

    aos_mutex_lock(&g_fifo_list.lock, AOS_WAIT_FOREVER);
    fifo_in->ref--;
    if (0 == fifo_in->ref) {
        dlist_t *item, *tmp;

        dlist_for_each_safe(item, tmp, &g_fifo_list.head) {
            if (&fifo_in->node == item) {
                dlist_del(item);
                break;
            }
        }

        sfifo_destroy(fifo_in->sfifo);
        av_free(fifo_in->name);
        av_free(fifo_in);
    }
    aos_mutex_unlock(&g_fifo_list.lock);
}

static const struct nsfifo_ops nsfifo_ops_fifo = {
    .init            = _nsfifo_in_init,
    .get_rpos        = _nsfifo_in_get_rpos,
    .set_rpos        = _nsfifo_in_set_rpos,
    .get_wpos        = _nsfifo_in_get_wpos,
    .set_wpos        = _nsfifo_in_set_wpos,
    .get_eof         = _nsfifo_in_get_eof,
    .set_eof         = _nsfifo_in_set_eof,
    .reset           = _nsfifo_in_reset,
    .get_len         = _nsfifo_in_get_len,
    .uninit          = _nsfifo_in_uninit,
};

const struct nsfifo_cls nsfifo_cls_fifo = {
    .name            = "fifo",
    .priv_size       = sizeof(struct nsfifo_in_priv),
    .ops             = &nsfifo_ops_fifo,
};



