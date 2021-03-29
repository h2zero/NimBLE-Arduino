/*
 * FreeRTOS.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */
#include "nimconfig.h"
#include "FreeRTOSCpp.h"
#include "NimBLELog.h"

#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions
#include <freertos/task.h>       // Include the task definitions
#include <freertos/semphr.h>     // Include the semaphore definitions
#include <string>

static const char* LOG_TAG = "FreeRTOS";


/**
 * Sleep for the specified number of milliseconds.
 * @param[in] ms The period in milliseconds for which to sleep.
 */
void FreeRTOS::sleep(uint32_t ms) {
    vTaskDelay(ms / portTICK_PERIOD_MS);
} // sleep


/**
 * Start a new task.
 * @param[in] task The function pointer to the function to be run in the task.
 * @param[in] taskName A string identifier for the task.
 * @param[in] param An optional parameter to be passed to the started task.
 * @param[in] stackSize An optional paremeter supplying the size of the stack in which to run the task.
 */
void FreeRTOS::startTask(void task(void*), const char *taskName, void* param, uint32_t stackSize) {
    xTaskCreate(task, taskName, stackSize, param, 5, NULL);
} // startTask


/**
 * Delete the task.
 * @param[in] pTask An optional handle to the task to be deleted.  If not supplied the calling task will be deleted.
 */
void FreeRTOS::deleteTask(TaskHandle_t pTask) {
    vTaskDelete(pTask);
} // deleteTask


/**
 * Get the time in milliseconds since the %FreeRTOS scheduler started.
 * @return The time in milliseconds since the %FreeRTOS scheduler started.
 */
uint32_t FreeRTOS::getTimeSinceStart() {
    return (uint32_t) (xTaskGetTickCount() * portTICK_PERIOD_MS);
} // getTimeSinceStart


/**
 * @brief Wait for a semaphore to be released by trying to take it and
 * then releasing it again.
 * @param [in] owner A debug tag.
 * @return The value associated with the semaphore.
 */
uint32_t FreeRTOS::Semaphore::wait(const char *owner) {
    NIMBLE_LOGD(LOG_TAG, ">> wait: Semaphore waiting: %s for %s", m_name, owner);
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_lock(&m_pthread_mutex);
    }
    else
#endif
    {
        xSemaphoreTake(m_semaphore, portMAX_DELAY);
    }
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_unlock(&m_pthread_mutex);
    }
    else
#endif
    {
        xSemaphoreGive(m_semaphore);
    }

    NIMBLE_LOGD(LOG_TAG, "<< wait: Semaphore released: %s", m_name);
    return m_value;
} // wait


/**
 * @brief Wait for a semaphore to be released in a given period of time by trying to take it and
 * then releasing it again. The value associated with the semaphore can be taken by value() call after return
 * @param [in] owner A debug tag.
 * @param [in] timeoutMs timeout to wait in ms.
 * @return True if we took the semaphore within timeframe.
 */
bool FreeRTOS::Semaphore::timedWait(const char *owner, uint32_t timeoutMs) {
    NIMBLE_LOGD(LOG_TAG, ">> wait: Semaphore waiting: %s for %s", m_name, owner);
#ifdef ESP_PLATFORM
    if (m_usePthreads && timeoutMs != portMAX_DELAY) {
        assert(false);  // We apparently don't have a timed wait for pthreads.
    }
#endif
    auto ret = pdTRUE;
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_lock(&m_pthread_mutex);
    }
    else
#endif
    {
        ret = xSemaphoreTake(m_semaphore, timeoutMs);
    }

#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_unlock(&m_pthread_mutex);
    }

    else
#endif
    {
        xSemaphoreGive(m_semaphore);
    }

    NIMBLE_LOGD(LOG_TAG, "<< wait: Semaphore %s released: %d", m_name, ret == pdTRUE? 1 : 0);
    return ret;
} // wait


/**
 * @brief Construct a semaphore, the semaphore is given when created.
 * @param [in] name A name string to provide debugging support.
 */
FreeRTOS::Semaphore::Semaphore(const char *name) {
#ifdef ESP_PLATFORM
    m_usePthreads = false;      // Are we using pThreads or FreeRTOS?
    if (m_usePthreads) {
        pthread_mutex_init(&m_pthread_mutex, nullptr);
    }
    else
#endif
    {
        m_semaphore = xSemaphoreCreateBinary();
        xSemaphoreGive(m_semaphore);
    }

    m_name      = name;
    m_value     = 0;
}


FreeRTOS::Semaphore::~Semaphore() {
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_destroy(&m_pthread_mutex);
    }
    else
#endif
    {
        vSemaphoreDelete(m_semaphore);
    }
}


/**
 * @brief Give the semaphore.
 */
void FreeRTOS::Semaphore::give() {
    NIMBLE_LOGD(LOG_TAG, "Semaphore giving: %s", m_name);
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_unlock(&m_pthread_mutex);
    }
    else
#endif
    {
        xSemaphoreGive(m_semaphore);
    }
} // Semaphore::give


/**
 * @brief Give a semaphore.
 * The Semaphore is given with an associated value.
 * @param [in] value The value to associate with the semaphore.
 */
void FreeRTOS::Semaphore::give(uint32_t value) {
    m_value = value;
    give();
} // give


/**
 * @brief Give a semaphore from an ISR.
 */
void FreeRTOS::Semaphore::giveFromISR() {
    BaseType_t higherPriorityTaskWoken;
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        assert(false);
    }
    else
#endif
    {
        xSemaphoreGiveFromISR(m_semaphore, &higherPriorityTaskWoken);
    }
} // giveFromISR


/**
 * @brief Take a semaphore.
 * Take a semaphore and wait indefinitely.
 * @param [in] owner The new owner (for debugging)
 * @return True if we took the semaphore.
 */
bool FreeRTOS::Semaphore::take(const char *owner) {
    NIMBLE_LOGD(LOG_TAG, "Semaphore taking: %s for %s", m_name, owner);
    bool rc = false;
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        pthread_mutex_lock(&m_pthread_mutex);
    }
    else
#endif
    {
        rc = xSemaphoreTake(m_semaphore, portMAX_DELAY) == pdTRUE;
    }

    if (rc) {
        NIMBLE_LOGD(LOG_TAG, "Semaphore taken:  %s", m_name);
    } else {
        NIMBLE_LOGE(LOG_TAG, "Semaphore NOT taken:  %s", m_name);
    }
    return rc;
} // Semaphore::take


/**
 * @brief Take a semaphore.
 * Take a semaphore but return if we haven't obtained it in the given period of milliseconds.
 * @param [in] timeoutMs Timeout in milliseconds.
 * @param [in] owner The new owner (for debugging)
 * @return True if we took the semaphore.
 */
bool FreeRTOS::Semaphore::take(uint32_t timeoutMs, const char *owner) {
    NIMBLE_LOGD(LOG_TAG, "Semaphore taking: %s for %s", m_name, owner);
    bool rc = false;
#ifdef ESP_PLATFORM
    if (m_usePthreads) {
        assert(false);  // We apparently don't have a timed wait for pthreads.
    }
    else
#endif
    {
        rc = xSemaphoreTake(m_semaphore, timeoutMs / portTICK_PERIOD_MS) == pdTRUE;
    }

    if (rc) {
        NIMBLE_LOGD(LOG_TAG, "Semaphore taken: %s", m_name);
    } else {
        NIMBLE_LOGE(LOG_TAG, "Semaphore NOT taken: %s", m_name);
    }
    return rc;
} // Semaphore::take

/**
 * @brief Set the name of the semaphore.
 * @param [in] name The name of the semaphore.
 */
void FreeRTOS::Semaphore::setName(const char *name) {
    m_name = name;
} // setName

#ifdef ESP_PLATFORM
/**
 * @brief Create a ring buffer.
 * @param [in] length The amount of storage to allocate for the ring buffer.
 * @param [in] type The type of buffer.  One of RINGBUF_TYPE_NOSPLIT, RINGBUF_TYPE_ALLOWSPLIT, RINGBUF_TYPE_BYTEBUF.
 */
#ifdef ESP_IDF_VERSION //Quick hack to detect if using IDF version that replaced ringbuf_type_t
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    Ringbuffer::Ringbuffer(size_t length, RingbufferType_t type) {
#else
    Ringbuffer::Ringbuffer(size_t length, ringbuf_type_t type) {
#endif
#else
    Ringbuffer::Ringbuffer(size_t length, ringbuf_type_t type) {
#endif
    m_handle = ::xRingbufferCreate(length, type);
} // Ringbuffer


Ringbuffer::~Ringbuffer() {
    ::vRingbufferDelete(m_handle);
} // ~Ringbuffer


/**
 * @brief Receive data from the buffer.
 * @param [out] size On return, the size of data returned.
 * @param [in] wait How long to wait.
 * @return A pointer to the storage retrieved.
 */
void* Ringbuffer::receive(size_t* size, TickType_t wait) {
    return ::xRingbufferReceive(m_handle, size, wait);
} // receive


/**
 * @brief Return an item.
 * @param [in] item The item to be returned/released.
 */
void Ringbuffer::returnItem(void* item) {
    ::vRingbufferReturnItem(m_handle, item);
} // returnItem


/**
 * @brief Send data to the buffer.
 * @param [in] data The data to place into the buffer.
 * @param [in] length The length of data to place into the buffer.
 * @param [in] wait How long to wait before giving up.  The default is to wait indefinitely.
 * @return
 */
bool Ringbuffer::send(void* data, size_t length, TickType_t wait) {
    return ::xRingbufferSend(m_handle, data, length, wait) == pdTRUE;
} // send
#endif

