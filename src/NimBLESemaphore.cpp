/*
 * NimBLESemaphore.cpp
 *
 *  Created: on May 7 2020
 *      Author H2zero
 * 
 * Originally:
 * FreeRTOS.cpp
 *
 *  Created on: Feb 24, 2017
 *      Author: kolban
 */
#include "sdkconfig.h"
#include "NimBLESemaphore.h"
#include "NimBLELog.h"

static const char* LOG_TAG = "NimBLESemaphore";


/**
 * @brief Wait for a semaphore to be released by trying to take it and
 * then releasing it again.
 * @param [in] owner A debug tag.
 * @return The value associated with the semaphore.
 */
uint32_t NimBLESemaphore::wait() {
    m_refCount++;
    NIMBLE_LOGD(LOG_TAG, ">> wait: Semaphore waiting: %s", toString().c_str());
    xSemaphoreTake(m_semaphore, portMAX_DELAY);

    m_refCount--;
    xSemaphoreGive(m_semaphore);

    NIMBLE_LOGD(LOG_TAG, "<< wait: Semaphore released: %s", toString().c_str());
    return m_value;
} // wait


/**
 * @brief Wait for a semaphore to be released in a given period of time by trying to take it and
 * then releasing it again. The value associated with the semaphore can be taken by value() call after return
 * @param [in] owner A debug tag.
 * @param [in] timeoutMs timeout to wait in ms.
 * @return True if we took the semaphore within timeframe.
 */
bool NimBLESemaphore::timedWait(uint32_t timeoutMs) {
	NIMBLE_LOGD(LOG_TAG, ">> wait: Semaphore waiting: %s", toString().c_str());

	auto ret = xSemaphoreTake(m_semaphore, timeoutMs);

    xSemaphoreGive(m_semaphore);

	NIMBLE_LOGD(LOG_TAG, "<< wait: Semaphore %s released: %d", toString().c_str(), ret);
	return ret;
} // timedWait


NimBLESemaphore::NimBLESemaphore(const char *name) {
    m_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(m_semaphore);
    m_name       = name;
    m_owner      = "";
    m_value      = 0;
    m_refCount   = 0;
}

NimBLESemaphore::NimBLESemaphore() {
    m_semaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(m_semaphore);
    m_owner      = "";
    m_value      = 0;
    m_refCount   = 0;
}


NimBLESemaphore::~NimBLESemaphore() {
    //NIMBLE_LOGD(LOG_TAG, "Semaphore delete: %s", toString().c_str());
    vSemaphoreDelete(m_semaphore);
}


/**
 * @brief Give a semaphore.
 * The Semaphore is given.
 */
void NimBLESemaphore::give() {
    m_refCount--;
    NIMBLE_LOGD(LOG_TAG, "Semaphore giving: %s", toString().c_str());
    xSemaphoreGive(m_semaphore);
} // Semaphore::give


/**
 * @brief Give a semaphore.
 * The Semaphore is given with an associated value.
 * @param [in] value The value to associate with the semaphore.
 */
void NimBLESemaphore::give(uint32_t value) {
    m_value = value;
    give();
} // give


/**
 * @brief Give a semaphore from an ISR.
 */
void NimBLESemaphore::giveFromISR() {
    BaseType_t higherPriorityTaskWoken;
    xSemaphoreGiveFromISR(m_semaphore, &higherPriorityTaskWoken);
} // giveFromISR


bool NimBLESemaphore::take() {
    return take("");
}
/**
 * @brief Take a semaphore.
 * Take a semaphore and wait indefinitely.
 * @param [in] owner The new owner (for debugging)
 * @return True if we took the semaphore.
 */
bool NimBLESemaphore::take(const char* owner) {
    m_refCount++;
    NIMBLE_LOGD(LOG_TAG, "Semaphore taking: %s", toString().c_str());
    bool rc = xSemaphoreTake(m_semaphore, portMAX_DELAY) == pdTRUE;
    if (rc) {
        m_owner = owner;
        NIMBLE_LOGD(LOG_TAG, "Semaphore taken:  %s", toString().c_str());
    } else {
        NIMBLE_LOGE(LOG_TAG, "Semaphore NOT taken:  %s", toString().c_str());
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
bool NimBLESemaphore::take(uint32_t timeoutMs) {
    NIMBLE_LOGD(LOG_TAG, "Semaphore taking: %s", toString().c_str());
    bool rc = xSemaphoreTake(m_semaphore, timeoutMs / portTICK_PERIOD_MS) == pdTRUE;
    if (rc) {
        NIMBLE_LOGD(LOG_TAG, "Semaphore taken:  %s", toString().c_str());
    } else {
        NIMBLE_LOGE(LOG_TAG, "Semaphore NOT taken:  %s", toString().c_str());
    }
    return rc;
} // Semaphore::take


size_t NimBLESemaphore::getRefCount() {
    return m_refCount;
}

/**
 * @brief Create a string representation of the semaphore.
 * @return A string representation of the semaphore.
 */
std::string NimBLESemaphore::toString() {
    char hex[14];
    std::string res = "owner: ";
    res += m_owner;
    snprintf(hex, sizeof(hex), " (0x%08x)", (uint32_t)m_semaphore);
    res += hex;
    snprintf(hex, sizeof(hex), " RefCnt: %d", m_refCount);
    res += hex;

    return res;
} // toString

