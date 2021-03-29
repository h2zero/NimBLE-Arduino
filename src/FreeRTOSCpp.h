/*
 * FreeRTOS.h
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */

#ifndef MAIN_FREERTOS_H_
#define MAIN_FREERTOS_H_

#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions.
#include <freertos/task.h>       // Include the task definitions.
#include <freertos/semphr.h>     // Include the semaphore definitions.
#ifdef ESP_PLATFORM
#include <freertos/ringbuf.h>    // Include the ringbuffer definitions.
#include <pthread.h>
#endif
#include <stdint.h>


/**
 * @brief Interface to %FreeRTOS functions.
 */
class FreeRTOS {
public:
    static void sleep(uint32_t ms);
    static void startTask(void task(void*), const char *taskName, void* param = nullptr, uint32_t stackSize = 2048);
    static void deleteTask(TaskHandle_t pTask = nullptr);

    static uint32_t getTimeSinceStart();

/**
 * @brief A binary semaphore class that operates like a mutex, it is already given when constructed.
 */
    class Semaphore {
    public:
        Semaphore(const char* = nullptr);
        ~Semaphore();
        void        give();
        void        give(uint32_t value);
        void        giveFromISR();
        void        setName(const char *name);
        bool        take(const char *owner = nullptr);
        bool        take(uint32_t timeoutMs, const char *owner = nullptr);
        bool        timedWait(const char *owner = nullptr, uint32_t timeoutMs = portMAX_DELAY);
        uint32_t    wait(const char *owner = nullptr);
        /**
         * @brief Get the value of the semaphore.
         * @return The value stored if the semaphore was given with give(value);
         */
        uint32_t    value(){ return m_value; };

    private:
        SemaphoreHandle_t m_semaphore;

        const char*       m_name;
        uint32_t          m_value;
#ifdef ESP_PLATFORM
        pthread_mutex_t   m_pthread_mutex;
        bool              m_usePthreads;
#endif
    };
};

#ifdef ESP_PLATFORM
/**
 * @brief A wrapper class for a freeRTOS ringbuffer.
 */
class Ringbuffer {
public:
#ifdef ESP_IDF_VERSION //Quick hack to detect if using IDF version that replaced ringbuf_type_t
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 0, 0)
    Ringbuffer(size_t length, RingbufferType_t type = RINGBUF_TYPE_NOSPLIT);
#else
    Ringbuffer(size_t length, ringbuf_type_t type = RINGBUF_TYPE_NOSPLIT);
#endif
#else
    Ringbuffer(size_t length, ringbuf_type_t type = RINGBUF_TYPE_NOSPLIT);
#endif
    ~Ringbuffer();

    void*    receive(size_t* size, TickType_t wait = portMAX_DELAY);
    void     returnItem(void* item);
    bool     send(void* data, size_t length, TickType_t wait = portMAX_DELAY);
private:
    RingbufHandle_t m_handle;
};
#endif
#endif /* MAIN_FREERTOS_H_ */
