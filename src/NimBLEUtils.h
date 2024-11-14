/*
 * NimBLEUtils.h
 *
 *  Created: on Jan 25 2020
 *      Author H2zero
 *
 */

#ifndef NIMBLE_CPP_UTILS_H_
#define NIMBLE_CPP_UTILS_H_

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED)
# include <string>

class NimBLEAddress;

/**
 * @brief A structure to hold data for a task that is waiting for a response.
 * @details This structure is used in conjunction with NimBLEUtils::taskWait() and NimBLEUtils::taskRelease().
 * All items are optional, the m_pHandle will be set in taskWait().
 */
struct NimBLETaskData {
    NimBLETaskData(void* pInstance = nullptr, int flags = 0, void* buf = nullptr);
    ~NimBLETaskData();
    void*       m_pInstance{nullptr};
    mutable int m_flags{0};
    void*       m_pBuf{nullptr};

  private:
    mutable void* m_pHandle{nullptr}; // semaphore or task handle
    friend class NimBLEUtils;
};

/**
 * @brief A BLE Utility class with methods for debugging and general purpose use.
 */
class NimBLEUtils {
  public:
    static const char*   gapEventToString(uint8_t eventType);
    static std::string   dataToHexString(const uint8_t* source, uint8_t length);
    static const char*   advTypeToString(uint8_t advType);
    static const char*   returnCodeToString(int rc);
    static NimBLEAddress generateAddr(bool nrpa);
    static bool          taskWait(const NimBLETaskData& taskData, uint32_t timeout);
    static void          taskRelease(const NimBLETaskData& taskData, int rc = 0);
};

#endif // CONFIG_BT_ENABLED
#endif // NIMBLE_CPP_UTILS_H_
