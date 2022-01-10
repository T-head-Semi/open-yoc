
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "aos/yloop.h"
#include "aos/kernel.h"
#include <k_config.h>

#ifdef CONFIG_KERNEL_WORKQUEUE
typedef struct work_para {
    aos_work_t *work;
    aos_loop_t loop;
    aos_call_t action;
    void *arg1;
    aos_call_t fini_cb;
    void *arg2;
} work_par_t;

static void free_wpar(work_par_t *wpar)
{
    aos_work_destroy(wpar->work);
    aos_free(wpar->work);
    aos_free(wpar);
}

static void run_my_work(void *arg)
{
    work_par_t *wpar = arg;

    wpar->action(wpar->arg1);

    if (wpar->fini_cb) {
        aos_schedule_call(wpar->fini_cb, wpar->arg2);
    }

    free_wpar(wpar);
}

void *aos_loop_schedule_work(int ms, aos_call_t action, void *arg1,
                             aos_call_t fini_cb, void *arg2)
{
    int ret;

    if (action == NULL) {
        return NULL;
    }

    aos_work_t *work = aos_malloc(sizeof(*work));
    work_par_t *wpar = aos_malloc(sizeof(*wpar));

    if (!work || !wpar) {
        goto err_out;
    }

    wpar->work = work;
    wpar->loop = aos_current_loop();
    wpar->action = action;
    wpar->arg1 = arg1;
    wpar->fini_cb = fini_cb;
    wpar->arg2 = arg2;

    ret = aos_work_init(work, run_my_work, wpar, ms);
    if (ret != 0) {
        goto err_out;
    }
    ret = aos_work_sched(work);
    if (ret != 0) {
        goto err_out;
    }

    return wpar;
err_out:
    aos_free(work);
    aos_free(wpar);

    return NULL;
}

void aos_cancel_work(void *w, aos_call_t action, void *arg1)
{
    work_par_t *wpar = w;

    if (wpar == NULL) {
        return;
    }

    if (wpar->action != action) {
        return;
    }

    if (wpar->arg1 != arg1) {
        return;
    }

    int ret = aos_work_cancel(wpar->work);
    if (ret != 0) {
        return;
    }

    free_wpar(wpar);
}
#endif
