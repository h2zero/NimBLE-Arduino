/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "syscfg/syscfg.h"

#if !CONFIG_BT_LE_CONTROLLER_NPL_OS_PORTING_SUPPORT

#include <assert.h>
#include <stddef.h>
#include <string.h>

#include "nimble/nimble/include/nimble/nimble_npl.h"

#ifdef ESP_PLATFORM
#include "freertos/portable.h"
#include "esp_log.h"
portMUX_TYPE ble_port_mutex = portMUX_INITIALIZER_UNLOCKED;
#  if CONFIG_BT_NIMBLE_USE_ESP_TIMER
static const char *LOG_TAG = "Timer";
#  endif

#else
#include "nrf.h"
static void *radio_isr_addr;
static void *rng_isr_addr;
static void *rtc0_isr_addr;
#endif

#ifdef ESP_PLATFORM
static inline bool
in_isr(void)
{
    /* XXX hw specific! */
    return xPortInIsrContext() != 0;
}

#else
static inline bool
in_isr(void)
{
    /* XXX hw specific! */
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
}

void
RADIO_IRQHandler(void)
{
    ((void (*)(void))radio_isr_addr)();
}

void
RNG_IRQHandler(void)
{
    ((void (*)(void))rng_isr_addr)();
}

void
RTC0_IRQHandler(void)
{
    ((void (*)(void))rtc0_isr_addr)();
}

/* This is called by NimBLE radio driver to set interrupt handlers */
void
npl_freertos_hw_set_isr(int irqn, void (*addr)(void))
{
    switch (irqn) {
    case RADIO_IRQn:
        radio_isr_addr = addr;
        break;
    case RNG_IRQn:
        rng_isr_addr = addr;
        break;
    case RTC0_IRQn:
        rtc0_isr_addr = addr;
        break;
    }
}
#endif

struct ble_npl_event *
npl_freertos_eventq_get(struct ble_npl_eventq *evq, ble_npl_time_t tmo)
{
    struct ble_npl_event *ev = NULL;
    BaseType_t woken;
    BaseType_t ret;

    if (in_isr()) {
        assert(tmo == 0);
        ret = xQueueReceiveFromISR(evq->q, &ev, &woken);
#ifdef ESP_PLATFORM
        if( woken == pdTRUE ) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken);
#endif
    } else {
        ret = xQueueReceive(evq->q, &ev, tmo);
    }
    assert(ret == pdPASS || ret == errQUEUE_EMPTY);

    if (ev) {
        ev->queued = false;
    }

    return ev;
}

void
npl_freertos_eventq_put(struct ble_npl_eventq *evq, struct ble_npl_event *ev)
{
    BaseType_t woken;
    BaseType_t ret;

    if (ev->queued) {
        return;
    }

    ev->queued = true;

    if (in_isr()) {
        ret = xQueueSendToBackFromISR(evq->q, &ev, &woken);
#ifdef ESP_PLATFORM
        if( woken == pdTRUE ) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken);
#endif
    } else {
        ret = xQueueSendToBack(evq->q, &ev, portMAX_DELAY);
    }

    assert(ret == pdPASS);
}

void
npl_freertos_eventq_remove(struct ble_npl_eventq *evq,
                      struct ble_npl_event *ev)
{
    struct ble_npl_event *tmp_ev;
    BaseType_t ret;
    int i;
    int count;
    BaseType_t woken, woken2;

    if (!ev->queued) {
        return;
    }

    /*
     * XXX We cannot extract element from inside FreeRTOS queue so as a quick
     * workaround we'll just remove all elements and add them back except the
     * one we need to remove. This is silly, but works for now - we probably
     * better use counting semaphore with os_queue to handle this in future.
     */

    if (in_isr()) {
        woken = pdFALSE;

        count = uxQueueMessagesWaitingFromISR(evq->q);
        for (i = 0; i < count; i++) {
            ret = xQueueReceiveFromISR(evq->q, &tmp_ev, &woken2);
            assert(ret == pdPASS);
            woken |= woken2;

            if (tmp_ev == ev) {
                continue;
            }

            ret = xQueueSendToBackFromISR(evq->q, &tmp_ev, &woken2);
            assert(ret == pdPASS);
            woken |= woken2;
        }

#ifdef ESP_PLATFORM
        if( woken == pdTRUE ) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken);
#endif
    } else {
#ifdef ESP_PLATFORM
        portMUX_TYPE ble_npl_mut = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL(&ble_npl_mut);
#else
        vPortEnterCritical();
#endif
        count = uxQueueMessagesWaiting(evq->q);
        for (i = 0; i < count; i++) {
            ret = xQueueReceive(evq->q, &tmp_ev, 0);
            assert(ret == pdPASS);

            if (tmp_ev == ev) {
                continue;
            }

            ret = xQueueSendToBack(evq->q, &tmp_ev, 0);
            assert(ret == pdPASS);
        }
#ifdef ESP_PLATFORM
        portEXIT_CRITICAL(&ble_npl_mut);
#else
        vPortExitCritical();
#endif
    }

    ev->queued = 0;
}

ble_npl_error_t
npl_freertos_mutex_init(struct ble_npl_mutex *mu)
{
    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    mu->handle = xSemaphoreCreateRecursiveMutex();
    assert(mu->handle);

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_mutex_deinit(struct ble_npl_mutex *mu)
{
    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    if (mu->handle) {
        vSemaphoreDelete(mu->handle);
    }

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_mutex_pend(struct ble_npl_mutex *mu, ble_npl_time_t timeout)
{
    BaseType_t ret;

    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(mu->handle);

    if (in_isr()) {
        ret = pdFAIL;
        assert(0);
    } else {
        ret = xSemaphoreTakeRecursive(mu->handle, timeout);
    }

    return ret == pdPASS ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
npl_freertos_mutex_release(struct ble_npl_mutex *mu)
{
    if (!mu) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(mu->handle);

    if (in_isr()) {
        assert(0);
    } else {
        if (xSemaphoreGiveRecursive(mu->handle) != pdPASS) {
            return BLE_NPL_BAD_MUTEX;
        }
    }

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_sem_init(struct ble_npl_sem *sem, uint16_t tokens)
{
    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    sem->handle = xSemaphoreCreateCounting(128, tokens);
    assert(sem->handle);

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_sem_deinit(struct ble_npl_sem *sem)
{
    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    if (sem->handle) {
        vSemaphoreDelete(sem->handle);
    }

    return BLE_NPL_OK;
}

ble_npl_error_t
npl_freertos_sem_pend(struct ble_npl_sem *sem, ble_npl_time_t timeout)
{
    BaseType_t woken;
    BaseType_t ret;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(sem->handle);

    if (in_isr()) {
        assert(timeout == 0);
        ret = xSemaphoreTakeFromISR(sem->handle, &woken);
#ifdef ESP_PLATFORM
        if( woken == pdTRUE ) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken);
#endif
    } else {
        ret = xSemaphoreTake(sem->handle, timeout);
    }

    return ret == pdPASS ? BLE_NPL_OK : BLE_NPL_TIMEOUT;
}

ble_npl_error_t
npl_freertos_sem_release(struct ble_npl_sem *sem)
{
    BaseType_t ret;
    BaseType_t woken;

    if (!sem) {
        return BLE_NPL_INVALID_PARAM;
    }

    assert(sem->handle);

    if (in_isr()) {
        ret = xSemaphoreGiveFromISR(sem->handle, &woken);
#ifdef ESP_PLATFORM
        if( woken == pdTRUE ) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken);
#endif
    } else {
        ret = xSemaphoreGive(sem->handle);
    }

    assert(ret == pdPASS);
    return BLE_NPL_OK;
}


#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
static void
ble_npl_event_fn_wrapper(void *arg)
{
    struct ble_npl_callout *co = (struct ble_npl_callout *)arg;

    if (co->evq) {
        ble_npl_eventq_put(co->evq, &co->ev);
    } else {
        co->ev.fn(&co->ev);
    }
}

static
ble_npl_error_t esp_err_to_npl_error(esp_err_t err)
{
    switch(err) {
    case ESP_ERR_INVALID_ARG:
        return BLE_NPL_INVALID_PARAM;

    case ESP_ERR_INVALID_STATE:
        return BLE_NPL_EINVAL;

    case ESP_OK:
        return BLE_NPL_OK;

    default:
        return BLE_NPL_ERROR;
    }
}
#else
static void
os_callout_timer_cb(TimerHandle_t timer)
{
    struct ble_npl_callout *co;

    co = pvTimerGetTimerID(timer);
    assert(co);

    if (co->evq) {
        ble_npl_eventq_put(co->evq, &co->ev);
    } else {
        co->ev.fn(&co->ev);
    }
}
#endif

int
npl_freertos_callout_init(struct ble_npl_callout *co, struct ble_npl_eventq *evq,
                          ble_npl_event_fn *ev_cb, void *ev_arg)
{
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
    co->ev.fn = ev_cb;
    co->ev.arg = ev_arg;
    co->evq = evq;

    esp_timer_create_args_t create_args = {
      .callback = ble_npl_event_fn_wrapper,
      .arg = co,
      .name = "nimble_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&create_args, &co->handle));
#else
    if (co->handle == NULL) {
        co->handle = xTimerCreate("co", 1, pdFALSE, co, os_callout_timer_cb);
        if (co->handle == NULL) {
            return -1;
        }
    }

    co->evq = evq;
    ble_npl_event_init(&co->ev, ev_cb, ev_arg);
#endif
    return 0;
}

void
npl_freertos_callout_deinit(struct ble_npl_callout *co)
{
    if (!co->handle) {
        return;
    }

#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
    if(esp_timer_stop(co->handle))
	ESP_LOGW(LOG_TAG, "Timer not stopped");

    if(esp_timer_delete(co->handle))
	ESP_LOGW(LOG_TAG, "Timer not deleted");
#else
    xTimerDelete(co->handle, portMAX_DELAY);
    ble_npl_event_deinit(&co->ev);
#endif
    memset(co, 0, sizeof(struct ble_npl_callout));
}

ble_npl_error_t
npl_freertos_callout_reset(struct ble_npl_callout *co, ble_npl_time_t ticks)
{
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
    esp_timer_stop(co->handle);

    return esp_err_to_npl_error(esp_timer_start_once(co->handle, ticks*1000));
#else

    BaseType_t woken1, woken2, woken3;

    if (ticks == 0) {
        ticks = 1;
    }

    if (in_isr()) {
        xTimerStopFromISR(co->handle, &woken1);
        xTimerChangePeriodFromISR(co->handle, ticks, &woken2);
        xTimerResetFromISR(co->handle, &woken3);
#ifdef ESP_PLATFORM
        if( woken1 == pdTRUE || woken2 == pdTRUE || woken3 == pdTRUE) {
            portYIELD_FROM_ISR();
        }
#else
        portYIELD_FROM_ISR(woken1 || woken2 || woken3);
#endif
    } else {
        xTimerStop(co->handle, portMAX_DELAY);
        xTimerChangePeriod(co->handle, ticks, portMAX_DELAY);
        xTimerReset(co->handle, portMAX_DELAY);
    }

    return BLE_NPL_OK;
#endif
}

void
npl_freertos_callout_stop(struct ble_npl_callout *co)
{
    if (!co->handle) {
        return;
    }
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
    esp_timer_stop(co->handle);
#else
    xTimerStop(co->handle, portMAX_DELAY);
#endif
}

bool
npl_freertos_callout_is_active(struct ble_npl_callout *co)
{
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
    return esp_timer_is_active(co->handle);
#else
    /* Workaround for bug in xTimerIsTimerActive with FreeRTOS V10.2.0, fixed in V10.4.4
     * See: https://github.com/FreeRTOS/FreeRTOS-Kernel/pull/305
     * Sometimes xTimerIsTimerActive returns pdTRUE even though the timer has expired, so we double check.
     */
    return xTimerIsTimerActive(co->handle) == pdTRUE && xTimerGetExpiryTime(co->handle) > xTaskGetTickCountFromISR();
#endif
}

ble_npl_time_t
npl_freertos_callout_get_ticks(struct ble_npl_callout *co)
{
#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
   /* Currently, esp_timer does not support an API which gets the expiry time for
    * current timer.
    * Returning 0 from here should not cause any effect.
    * Drawback of this approach is that existing code to reset timer would be called
    * more often (since the if condition to invoke reset timer would always succeed if
    * timer is active).
    */

    return 0;
#else
    return xTimerGetExpiryTime(co->handle);
#endif
}

ble_npl_time_t
npl_freertos_callout_remaining_ticks(struct ble_npl_callout *co,
                                     ble_npl_time_t now)
{
    ble_npl_time_t rt;
    uint32_t exp = 0;

#if CONFIG_BT_NIMBLE_USE_ESP_TIMER
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    uint64_t expiry = 0;
    esp_err_t err;

    //Fetch expiry time in microseconds
    err = esp_timer_get_expiry_time((esp_timer_handle_t)(co->handle), &expiry);
    if (err != ESP_OK) {
        //Error. Could not fetch the expiry time
        return 0;
    }

    //Convert microseconds to ticks
    npl_freertos_time_ms_to_ticks((uint32_t)(expiry / 1000), &exp);
#else
    //esp_timer_get_expiry_time() is only available from IDF 5.0 onwards
    //Set expiry to 0
    exp = 0;
#endif //ESP_IDF_VERSION
#else
    exp = xTimerGetExpiryTime(co->handle);
#endif

    if (exp > now) {
        rt = exp - now;
    } else {
        rt = 0;
    }

    return rt;
}

ble_npl_error_t
npl_freertos_time_ms_to_ticks(uint32_t ms, ble_npl_time_t *out_ticks)
{
    uint64_t ticks;

    ticks = ((uint64_t)ms * configTICK_RATE_HZ) / 1000;
    if (ticks > UINT32_MAX) {
        return BLE_NPL_EINVAL;
    }

    *out_ticks = ticks;

    return 0;
}

ble_npl_error_t
npl_freertos_time_ticks_to_ms(ble_npl_time_t ticks, uint32_t *out_ms)
{
    uint64_t ms;

    ms = ((uint64_t)ticks * 1000) / configTICK_RATE_HZ;
    if (ms > UINT32_MAX) {
        return BLE_NPL_EINVAL;
     }

    *out_ms = ms;

    return 0;
}

#else // !CONFIG_BT_LE_CONTROLLER_NPL_OS_PORTING_SUPPORT

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "nimble/esp_port/port/include/bt_osi_mem.h"
#include "nimble/porting/nimble/include/os/os_mempool.h"
#include "freertos/FreeRTOS.h"
#include "nimble/porting/nimble/include/os/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/portable.h"
#include "nimble/porting/npl/freertos/include/nimble/npl_freertos.h"

extern struct os_mempool ble_freertos_ev_pool;
static os_membuf_t *ble_freertos_ev_buf = NULL;

extern struct os_mempool ble_freertos_evq_pool;
static os_membuf_t *ble_freertos_evq_buf = NULL;

extern struct os_mempool ble_freertos_co_pool;
static os_membuf_t *ble_freertos_co_buf = NULL;

extern struct os_mempool ble_freertos_sem_pool;
static os_membuf_t *ble_freertos_sem_buf = NULL;

extern struct os_mempool ble_freertos_mutex_pool;
static os_membuf_t *ble_freertos_mutex_buf = NULL;

static uint16_t ble_freertos_total_event_cnt = 0;

int na_npl_freertos_mempool_init(void)
{
    int rc = -1;
    // These values taken from esp-idf: bt/nimble/porting/npl/freertos/src/npl_os_freertos.c
    uint16_t ble_total_evt_count = 19;
    uint16_t ble_total_co_count = 8;
    uint16_t ble_total_evtq_count = 3;
    uint16_t ble_total_sem_count = 10;
    uint16_t ble_total_mutex_count = 4;
    ble_freertos_total_event_cnt = ble_total_evt_count;

    if (ble_total_evt_count) {
        ble_freertos_ev_buf  = bt_osi_mem_malloc_internal(OS_MEMPOOL_SIZE(ble_total_evt_count,
                                      sizeof (struct ble_npl_event_freertos)) *
                                      sizeof(os_membuf_t));
        if (!ble_freertos_ev_buf) {
            goto _error;
        }
        rc = os_mempool_init(&ble_freertos_ev_pool, ble_total_evt_count,
                             sizeof (struct ble_npl_event_freertos), ble_freertos_ev_buf,
                             "ble_freertos_ev_pool");
        if (rc) {
            goto _error;
        }
    }

    if (ble_total_evtq_count) {
        ble_freertos_evq_buf  = bt_osi_mem_malloc_internal(OS_MEMPOOL_SIZE(ble_total_evtq_count,
                                       sizeof (struct ble_npl_eventq_freertos)) *
                                       sizeof(os_membuf_t));
        if (!ble_freertos_evq_buf) {
            goto _error;
        }
        rc = os_mempool_init(&ble_freertos_evq_pool, ble_total_evtq_count,
                             sizeof (struct ble_npl_eventq_freertos), ble_freertos_evq_buf,
                             "ble_freertos_evq_pool");
        if (rc) {
            goto _error;
        }
    }

    if (ble_total_co_count) {
        ble_freertos_co_buf  = bt_osi_mem_malloc_internal(OS_MEMPOOL_SIZE(ble_total_co_count,
                                      sizeof (struct ble_npl_callout_freertos)) *
                                      sizeof(os_membuf_t));
        if (!ble_freertos_co_buf) {
            goto _error;
        }
        rc = os_mempool_init(&ble_freertos_co_pool, ble_total_co_count,
                             sizeof (struct ble_npl_callout_freertos), ble_freertos_co_buf,
                             "ble_freertos_co_pool");
        if (rc) {
            goto _error;
        }
    }

    if (ble_total_sem_count) {
        ble_freertos_sem_buf  = bt_osi_mem_malloc_internal(OS_MEMPOOL_SIZE(ble_total_sem_count,
                                       sizeof (struct ble_npl_sem_freertos)) *
                                       sizeof(os_membuf_t));
        if (!ble_freertos_sem_buf) {
            goto _error;
        }
        rc = os_mempool_init(&ble_freertos_sem_pool, ble_total_sem_count,
                             sizeof (struct ble_npl_sem_freertos), ble_freertos_sem_buf,
                             "ble_freertos_sem_pool");
        if (rc) {
            goto _error;
        }
    }

    if (ble_total_mutex_count) {
        ble_freertos_mutex_buf  = bt_osi_mem_malloc_internal(OS_MEMPOOL_SIZE(ble_total_mutex_count,
                                         sizeof (struct ble_npl_mutex_freertos)) *
                                         sizeof(os_membuf_t));
        if (!ble_freertos_mutex_buf) {
            goto _error;
        }
        rc = os_mempool_init(&ble_freertos_mutex_pool, ble_total_mutex_count,
                             sizeof (struct ble_npl_mutex_freertos), ble_freertos_mutex_buf,
                             "ble_freertos_mutex_pool");
        if (rc) {
            goto _error;
        }
    }

    return 0;
_error:
    if (ble_freertos_ev_buf) {
        bt_osi_mem_free(ble_freertos_ev_buf);
        ble_freertos_ev_buf = NULL;
    }

    if (ble_freertos_evq_buf) {
        bt_osi_mem_free(ble_freertos_evq_buf);
        ble_freertos_evq_buf = NULL;
    }

    if (ble_freertos_co_buf) {
        bt_osi_mem_free(ble_freertos_co_buf);
        ble_freertos_co_buf = NULL;
    }

    if (ble_freertos_sem_buf) {
        bt_osi_mem_free(ble_freertos_sem_buf);
        ble_freertos_sem_buf = NULL;
    }

    if (ble_freertos_mutex_buf) {
        bt_osi_mem_free(ble_freertos_mutex_buf);
        ble_freertos_mutex_buf = NULL;
    }
    return -1;
}

void na_npl_freertos_mempool_deinit(void)
{
    if (ble_freertos_ev_buf) {
        bt_osi_mem_free(ble_freertos_ev_buf);
        ble_freertos_ev_buf = NULL;
    }
    if (ble_freertos_evq_buf) {
        bt_osi_mem_free(ble_freertos_evq_buf);
        ble_freertos_evq_buf = NULL;
    }
    if (ble_freertos_co_buf) {
        bt_osi_mem_free(ble_freertos_co_buf);
        ble_freertos_co_buf = NULL;
    }
    if (ble_freertos_sem_buf) {
        bt_osi_mem_free(ble_freertos_sem_buf);
        ble_freertos_sem_buf = NULL;
    }
    if (ble_freertos_mutex_buf) {
        bt_osi_mem_free(ble_freertos_mutex_buf);
        ble_freertos_mutex_buf = NULL;
    }
}

void
na_npl_freertos_eventq_init(struct ble_npl_eventq *evq)
{
    struct ble_npl_eventq_freertos *eventq = NULL;
    if (!os_memblock_from(&ble_freertos_evq_pool,evq->eventq)) {
        evq->eventq = os_memblock_get(&ble_freertos_evq_pool);
        eventq = (struct ble_npl_eventq_freertos*)evq->eventq;
        BLE_LL_ASSERT(eventq);
        memset(eventq, 0, sizeof(*eventq));
        eventq->q = xQueueCreate(ble_freertos_total_event_cnt, sizeof(struct ble_npl_eventq *));
        BLE_LL_ASSERT(eventq->q);
    } else {
        eventq = (struct ble_npl_eventq_freertos*)evq->eventq;
        xQueueReset(eventq->q);
    }
}

#endif
