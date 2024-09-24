/*
 * NimBLERemoteValueAttribute.cpp
 *
 *  Created: on July 28 2024
 *      Author H2zero
 */

#include "nimconfig.h"
#if defined(CONFIG_BT_ENABLED) && defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL)

# include "NimBLERemoteValueAttribute.h"
# include "NimBLEClient.h"

# include <climits>

const char* LOG_TAG = "NimBLERemoteValueAttribute";

bool NimBLERemoteValueAttribute::writeValue(const uint8_t* data, size_t length, bool response) const {
    NIMBLE_LOGD(LOG_TAG, ">> writeValue()");

    const NimBLEClient* pClient    = getClient();
    TaskHandle_t        cur_task   = xTaskGetCurrentTaskHandle();
    ble_task_data_t     taskData   = {const_cast<NimBLERemoteValueAttribute*>(this), cur_task, 0, nullptr};
    int                 retryCount = 1;
    int                 rc         = 0;
    uint16_t            mtu        = pClient->getMTU() - 3;

    // Check if the data length is longer than we can write in one connection event.
    // If so we must do a long write which requires a response.
    if (length <= mtu && !response) {
        rc = ble_gattc_write_no_rsp_flat(pClient->getConnId(), getHandle(), data, length);
        goto Done;
    }

    do {
        if (length > mtu) {
            NIMBLE_LOGI(LOG_TAG, "writeValue: long write");
            os_mbuf* om = ble_hs_mbuf_from_flat(data, length);
            rc = ble_gattc_write_long(pClient->getConnId(), getHandle(), 0, om, NimBLERemoteValueAttribute::onWriteCB, &taskData);
        } else {
            rc = ble_gattc_write_flat(pClient->getConnId(),
                                      getHandle(),
                                      data,
                                      length,
                                      NimBLERemoteValueAttribute::onWriteCB,
                                      &taskData);
        }

        if (rc != 0) {
            goto Done;
        }

# ifdef ulTaskNotifyValueClear
        // Clear the task notification value to ensure we block
        ulTaskNotifyValueClear(cur_task, ULONG_MAX);
# endif
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rc = taskData.rc;

        switch (rc) {
            case 0:
            case BLE_HS_EDONE:
                rc = 0;
                break;
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
                NIMBLE_LOGE(LOG_TAG, "Long write not supported by peer; Truncating length to %d", mtu);
                retryCount++;
                length = mtu;
                break;

            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
                if (retryCount && pClient->secureConnection()) break;
            /* Else falls through. */
            default:
                goto Done;
        }
    } while (rc != 0 && retryCount--);

Done:
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "<< writeValue failed, rc: %d %s", rc, NimBLEUtils::returnCodeToString(rc));
    } else {
        NIMBLE_LOGD(LOG_TAG, "<< writeValue, rc: %d", rc);
    }

    return (rc == 0);
} // writeValue

/**
 * @brief Callback for characteristic write operation.
 * @return success == 0 or error code.
 */
int NimBLERemoteValueAttribute::onWriteCB(uint16_t conn_handle, const ble_gatt_error* error, ble_gatt_attr* attr, void* arg) {
    auto       pTaskData = static_cast<ble_task_data_t*>(arg);
    const auto pAtt      = static_cast<NimBLERemoteValueAttribute*>(pTaskData->pATT);

    if (pAtt->getClient()->getConnId() != conn_handle) {
        return 0;
    }

    NIMBLE_LOGI(LOG_TAG, "Write complete; status=%d", error->status);
    pTaskData->rc = error->status;
    xTaskNotifyGive(pTaskData->task);
    return 0;
}

/**
 * @brief Read the value of the remote characteristic.
 * @param [in] timestamp A pointer to a time_t struct to store the time the value was read.
 * @return The value of the remote characteristic.
 */
NimBLEAttValue NimBLERemoteValueAttribute::readValue(time_t* timestamp) const {
    NIMBLE_LOGD(LOG_TAG, ">> readValue()");

    NimBLEAttValue      value{};
    const NimBLEClient* pClient    = getClient();
    int                 rc         = 0;
    int                 retryCount = 1;
    TaskHandle_t        cur_task   = xTaskGetCurrentTaskHandle();
    ble_task_data_t     taskData   = {const_cast<NimBLERemoteValueAttribute*>(this), cur_task, 0, &value};

    do {
        rc = ble_gattc_read_long(pClient->getConnId(), getHandle(), 0, NimBLERemoteValueAttribute::onReadCB, &taskData);
        if (rc != 0) {
            goto Done;
        }

# ifdef ulTaskNotifyValueClear
        // Clear the task notification value to ensure we block
        ulTaskNotifyValueClear(cur_task, ULONG_MAX);
# endif
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        rc = taskData.rc;

        switch (rc) {
            case 0:
            case BLE_HS_EDONE:
                rc = 0;
                break;
            // Characteristic is not long-readable, return with what we have.
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_ATTR_NOT_LONG):
                NIMBLE_LOGI(LOG_TAG, "Attribute not long");
                rc = ble_gattc_read(pClient->getConnId(), getHandle(), NimBLERemoteValueAttribute::onReadCB, &taskData);
                if (rc != 0) {
                    goto Done;
                }
                retryCount++;
                break;
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHEN):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_AUTHOR):
            case BLE_HS_ATT_ERR(BLE_ATT_ERR_INSUFFICIENT_ENC):
                if (retryCount && pClient->secureConnection()) break;
            /* Else falls through. */
            default:
                goto Done;
        }
    } while (rc != 0 && retryCount--);

    value.setTimeStamp();
    m_value = value;
    if (timestamp != nullptr) {
        *timestamp = value.getTimeStamp();
    }

Done:
    if (rc != 0) {
        NIMBLE_LOGE(LOG_TAG, "<< readValue failed rc=%d, %s", rc, NimBLEUtils::returnCodeToString(rc));
    } else {
        NIMBLE_LOGD(LOG_TAG, "<< readValue rc=%d", rc);
    }

    return value;
} // readValue

/**
 * @brief Callback for characteristic read operation.
 * @return success == 0 or error code.
 */
int NimBLERemoteValueAttribute::onReadCB(uint16_t conn_handle, const ble_gatt_error* error, ble_gatt_attr* attr, void* arg) {
    auto       pTaskData = static_cast<ble_task_data_t*>(arg);
    const auto pAtt      = static_cast<NimBLERemoteValueAttribute*>(pTaskData->pATT);

    if (pAtt->getClient()->getConnId() != conn_handle) {
        return 0;
    }

    int rc = error->status;
    NIMBLE_LOGI(LOG_TAG, "Read complete; status=%d", rc);

    if (rc == 0) {
        if (attr) {
            auto     valBuf   = static_cast<NimBLEAttValue*>(pTaskData->buf);
            uint16_t data_len = OS_MBUF_PKTLEN(attr->om);
            if ((valBuf->size() + data_len) > BLE_ATT_ATTR_MAX_LEN) {
                rc = BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            } else {
                NIMBLE_LOGD(LOG_TAG, "Got %u bytes", data_len);
                valBuf->append(attr->om->om_data, data_len);
                return 0;
            }
        }
    }

    pTaskData->rc = rc;
    xTaskNotifyGive(pTaskData->task);

    return rc;
} // onReadCB

#endif // CONFIG_BT_ENABLED && CONFIG_BT_NIMBLE_ROLE_CENTRAL
