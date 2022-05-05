/*
 *	Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/*******************************************************
 * @file       dw_wdt.c
 * @brief      source file for wdt csi driver
 * @version    V1.0
 * @date       17. July 2020
 * ******************************************************/

#include <drv/wdt.h>
#include <drv/irq.h>

extern void hal_watchdog_disable(void);
extern void hal_watchdog_reset(int timeout);
extern void hal_watchdog_restart(void);
extern void hal_watchdog_info(void);
extern void hal_watchdog_init(void);
extern void hal_watchdog_stop(int timeout);
extern void hal_watchdog_start(int timeout);
extern void hal_watchdog_feed(void);
extern int hal_watchdog_suspend(int timeout);
extern int hal_watchdog_resume(int timeout);
extern int hal_watchdog_is_running(void);

static int _timeout;
void dw_wdt_irq_handler(void *arg)
{
//    csi_wdt_t *wdt = (csi_wdt_t *)arg;
//    dw_wdt_regs_t *wdt_base = (dw_wdt_regs_t *)wdt->dev.reg_base;
//
//    if (dw_wdt_stat_status_get(wdt_base)) {
//        if (wdt->callback) {
//            wdt->callback(wdt, wdt->arg);
//        }
//    }
}

/**
  \brief       Initialize WDT Interface. Initializes the resources needed for the WDT interface
  \param[in]   wdt    wdt handle to operate
  \param[in]   idx    wdt index
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_init(csi_wdt_t *wdt, uint32_t idx)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    hal_watchdog_init();

    return ret;
}
/**
  \brief       De-initialize WDT Interface. stops operation and releases the software resources used by the interface
  \param[in]   wdt    handle to operate
  \return      None
*/
void csi_wdt_uninit(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK_NORETVAL(wdt);
    //TODO:
}
/**
  \brief       Set the WDT value
  \param[in]   wdt    handle to operate
  \param[in]   ms     the timeout value(ms)
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_set_timeout(csi_wdt_t *wdt, uint32_t ms)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    _timeout = ms;

    return ret;
}
/**
  \brief       Start the WDT
  \param[in]   wdt    handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_start(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    hal_watchdog_start(_timeout);

    return ret;
}
/**
  \brief       Stop the WDT
  \param[in]   wdt    handle to operate
  \return      None
*/
void csi_wdt_stop(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK_NORETVAL(wdt);
    hal_watchdog_stop(_timeout);
}

/**
  \brief       Feed the WDT
  \param[in]   wdt    handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_feed(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    hal_watchdog_feed();

    return ret;
}

/**
  \brief       Get the remaining time to timeout
  \param[in]   wdt    handle to operate
  \return      tne remaining time of wdt(ms)
*/
uint32_t csi_wdt_get_remaining_time(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, 0U);
    //TODO:

    return 0;
}

/**
  \brief       Check wdt is running
  \param[in]   wdt    handle wdt handle to operate
  \return      true->running, false->stopped
*/
bool csi_wdt_is_running(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, false);

    return (hal_watchdog_is_running() ? true : false);
}
/**
  \brief       Attach the callback handler to wdt
  \param[in]   wdt         operate handle
  \param[in]   callback    callback function
  \param[in]   arg         callback's param
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_attach_callback(csi_wdt_t *wdt, void *callback, void *arg)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    //TODO:

    return CSI_OK;
}

/**
  \brief       Detach the callback handler
  \param[in]   wdt    operate handle
  \return      None
*/
void csi_wdt_detach_callback(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK_NORETVAL(wdt);
    //TODO:
}

#ifdef CONFIG_PM
csi_error_t dw_wdt_pm_action(csi_dev_t *dev, csi_pm_dev_action_t action)
{
    CSI_PARAM_CHK(dev, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    csi_pm_dev_t *pm_dev = &dev->pm_dev;

    switch (action) {
    case PM_DEV_SUSPEND:
        csi_pm_dev_save_regs(pm_dev->reten_mem, (uint32_t *)(dev->reg_base), 2U);
        break;

    case PM_DEV_RESUME:
        csi_pm_dev_restore_regs(pm_dev->reten_mem, (uint32_t *)(dev->reg_base), 2U);
        break;

    default:
        ret = CSI_ERROR;
        break;
    }

    return ret;
}

csi_error_t csi_wdt_enable_pm(csi_wdt_t *wdt)
{
    return csi_pm_dev_register(&wdt->dev, dw_wdt_pm_action, 8U, 0U);
}

void csi_wdt_disable_pm(csi_wdt_t *wdt)
{
    csi_pm_dev_unregister(&wdt->dev);
}
#endif

