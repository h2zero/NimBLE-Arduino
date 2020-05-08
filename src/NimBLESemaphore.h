/*
 * NimBLESemaphore.h
 *
 *  Created: on May 7 2020
 *      Author H2zero
 * 
 * Originally:
 * FreeRTOS.h
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */

#ifndef MAIN_NIMBLE_SEMAPHORE_H_
#define MAIN_NIMBLE_SEMAPHORE_H_

#include <freertos/FreeRTOS.h>   // Include the base FreeRTOS definitions.
#include <freertos/semphr.h>     // Include the semaphore definitions.

#include <string>

#define NIMBLE_SEMAPHORE_TAKE(pSemaphore, owner) \
    if(pSemaphore == nullptr) { \
        pSemaphore = new NimBLESemaphore(); \
    } \
    pSemaphore->take(owner);
    
#define NIMBLE_SEMAPHORE_GIVE(pSemaphore, value) \
    if(pSemaphore != nullptr) { \
        pSemaphore->give(value); \
    }
    
#define NIMBLE_SEMAPHORE_WAIT(pSemaphore) \
    pSemaphore->wait();
    
#define NIMBLE_SEMAPHORE_DELETE(pSemaphore) \
    if(pSemaphore != nullptr) { \
        pSemaphore->give(); \
        delete(pSemaphore); \
        pSemaphore = nullptr; \
    }

/**
 * @brief Interface to %FreeRTOS functions.
 */
class NimBLESemaphore {
public:
    NimBLESemaphore(const char *name);
    NimBLESemaphore();
    ~NimBLESemaphore();
    void        give();
    void        give(uint32_t value);
    void        giveFromISR();
    bool        take();
    bool        take(const char* owner);
    bool        take(uint32_t timeoutMs);
    std::string toString();
    bool		timedWait(uint32_t timeoutMs = portMAX_DELAY);
    uint32_t    wait();
    uint32_t	value(){ return m_value; };

private:
    SemaphoreHandle_t m_semaphore;
    const char        *m_name;
    const char        *m_owner;
    uint32_t          m_value;
};

#endif /* MAIN_NIMBLE_SEMAPHORE_H_ */
