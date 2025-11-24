/*
 * Copyright 2020-2025 Ryan Powell <ryan@nable-embedded.io> and
 * esp-nimble-cpp, NimBLE-Arduino contributors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef ESP_PLATFORM
# include "NimBLEStream.h"
# if CONFIG_BT_NIMBLE_ENABLED && (MYNEWT_VAL(BLE_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_ROLE_CENTRAL))

#  include "NimBLEDevice.h"
#  include "rom/uart.h"

static const char* LOG_TAG = "NimBLEStream";

// Stub Print/Stream implementations when Arduino not available
#  if !NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#   include <cstring>

size_t Print::print(const char* s) {
    if (!s) return 0;
    return write(reinterpret_cast<const uint8_t*>(s), strlen(s));
}

size_t Print::println(const char* s) {
    size_t            n       = print(s);
    static const char crlf[]  = "\r\n";
    n                        += write(reinterpret_cast<const uint8_t*>(crlf), 2);
    return n;
}

size_t Print::printf(const char* fmt, ...) {
    if (!fmt) {
        return 0;
    }

    char    stackBuf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(stackBuf, sizeof(stackBuf), fmt, ap);
    va_end(ap);
    if (n < 0) {
        return 0;
    }

    if (static_cast<size_t>(n) < sizeof(stackBuf)) {
        return write(reinterpret_cast<const uint8_t*>(stackBuf), static_cast<size_t>(n));
    }

    // allocate for larger output
    size_t needed = static_cast<size_t>(n) + 1;
    char*  buf    = static_cast<char*>(malloc(needed));
    if (!buf) {
        return 0;
    }

    va_start(ap, fmt);
    vsnprintf(buf, needed, fmt, ap);
    va_end(ap);
    size_t ret = write(reinterpret_cast<const uint8_t*>(buf), static_cast<size_t>(n));
    free(buf);
    return ret;
}
#  endif

void NimBLEStream::txTask(void* arg) {
    NimBLEStream* pStream = static_cast<NimBLEStream*>(arg);
    for (;;) {
        size_t itemSize = 0;
        void*  item     = xRingbufferReceive(pStream->m_txBuf, &itemSize, portMAX_DELAY);
        if (item) {
            pStream->send(reinterpret_cast<uint8_t*>(item), itemSize);
            vRingbufferReturnItem(pStream->m_txBuf, item);
        }
    }
}

bool NimBLEStream::begin() {
    if (m_txBuf || m_rxBuf || m_txTask) {
        NIMBLE_UART_LOGW(LOG_TAG, "Already initialized");
        return true;
    }

    if (m_txBufSize) {
        m_txBuf = xRingbufferCreate(m_txBufSize, RINGBUF_TYPE_BYTEBUF);
        if (!m_txBuf) {
            NIMBLE_UART_LOGE(LOG_TAG, "Failed to create TX ringbuffer");
            return false;
        }
    }

    if (m_rxBufSize) {
        m_rxBuf = xRingbufferCreate(m_rxBufSize, RINGBUF_TYPE_BYTEBUF);
        if (!m_rxBuf) {
            NIMBLE_UART_LOGE(LOG_TAG, "Failed to create RX ringbuffer");
            if (m_txBuf) {
                vRingbufferDelete(m_txBuf);
                m_txBuf = nullptr;
            }
            return false;
        }
    }

    if (xTaskCreate(txTask, "NimBLEStreamTx", m_txTaskStackSize, this, m_txTaskPriority, &m_txTask) != pdPASS) {
        NIMBLE_UART_LOGE(LOG_TAG, "Failed to create stream tx task");
        if (m_rxBuf) {
            vRingbufferDelete(m_rxBuf);
            m_rxBuf = nullptr;
        }
        if (m_txBuf) {
            vRingbufferDelete(m_txBuf);
            m_txBuf = nullptr;
        }
        return false;
    }

    return true;
}

bool NimBLEStream::end() {
    if (m_txTask) {
        vTaskDelete(m_txTask);
        m_txTask = nullptr;
    }
    if (m_txBuf) {
        vRingbufferDelete(m_txBuf);
        m_txBuf = nullptr;
    }
    if (m_rxBuf) {
        vRingbufferDelete(m_rxBuf);
        m_rxBuf = nullptr;
    }
    m_hasPeek = false;
    return true;
}

size_t NimBLEStream::write(const uint8_t* data, size_t len) {
    if (!m_txBuf || !data || len == 0) {
        return 0;
    }

    ble_npl_time_t timeout = 0;
    ble_npl_time_ms_to_ticks(getTimeout(), &timeout);
    size_t chunk = std::min(len, xRingbufferGetCurFreeSize(m_txBuf));
    if (xRingbufferSend(m_txBuf, data, chunk, static_cast<TickType_t>(timeout)) != pdTRUE) {
        return 0;
    }
    return chunk;
}

size_t NimBLEStream::availableForWrite() const {
    return m_txBuf ? xRingbufferGetCurFreeSize(m_txBuf) : 0;
}

void NimBLEStream::flush() {
    // Wait until TX ring is drained
    while (m_txBuf && xRingbufferGetCurFreeSize(m_txBuf) < m_txBufSize) {
        ble_npl_time_delay(ble_npl_time_ms_to_ticks32(1));
    }
}

int NimBLEStream::available() {
    if (!m_rxBuf) {
        NIMBLE_UART_LOGE(LOG_TAG, "Invalid RX buffer");
        return 0;
    }

    if (m_hasPeek) {
        return 1; // at least the peeked byte
    }

    // Query items in RX ring
    UBaseType_t waiting = 0;
    vRingbufferGetInfo(m_rxBuf, nullptr, nullptr, nullptr, nullptr, &waiting);
    return static_cast<int>(waiting);
}

int NimBLEStream::read() {
    if (!m_rxBuf) {
        return -1;
    }

    // Return peeked byte if available
    if (m_hasPeek) {
        m_hasPeek = false;
        return static_cast<int>(m_peekByte);
    }

    size_t   itemSize = 0;
    uint8_t* item     = static_cast<uint8_t*>(xRingbufferReceive(m_rxBuf, &itemSize, 0));
    if (!item || itemSize == 0) return -1;

    uint8_t byte = item[0];

    // If item has more bytes, put the rest back
    if (itemSize > 1) {
        xRingbufferSend(m_rxBuf, item + 1, itemSize - 1, 0);
    }

    vRingbufferReturnItem(m_rxBuf, item);
    return static_cast<int>(byte);
}

int NimBLEStream::peek() {
    if (!m_rxBuf) {
        return -1;
    }

    if (m_hasPeek) {
        return static_cast<int>(m_peekByte);
    }

    size_t   itemSize = 0;
    uint8_t* item     = static_cast<uint8_t*>(xRingbufferReceive(m_rxBuf, &itemSize, 0));
    if (!item || itemSize == 0) {
        return -1;
    }

    m_peekByte = item[0];
    m_hasPeek  = true;

    // Put the entire item back
    xRingbufferSend(m_rxBuf, item, itemSize, 0);
    vRingbufferReturnItem(m_rxBuf, item);

    return static_cast<int>(m_peekByte);
}

size_t NimBLEStream::pushRx(const uint8_t* data, size_t len) {
    if (!m_rxBuf || !data || len == 0) {
        NIMBLE_UART_LOGE(LOG_TAG, "Invalid RX buffer or data");
        return 0;
    }

    // Clear peek state when new data arrives
    m_hasPeek = false;

    if (xRingbufferSend(m_rxBuf, data, len, 0) != pdTRUE) {
        NIMBLE_UART_LOGE(LOG_TAG, "RX buffer full, dropping %zu bytes", len);
        return 0;
    }
    return len;
}

#  if MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
bool NimBLEStreamServer::init(const NimBLEUUID& svcUuid, const NimBLEUUID& chrUuid, bool canWrite, bool secure) {
    if (!NimBLEDevice::isInitialized()) {
        NIMBLE_UART_LOGE(LOG_TAG, "NimBLEDevice not initialized");
        return false;
    }

    NimBLEServer* pServer = NimBLEDevice::getServer();
    if (!pServer) {
        pServer = NimBLEDevice::createServer();
    }

    NimBLEService* pSvc = pServer->getServiceByUUID(svcUuid);
    if (!pSvc) {
        pSvc = pServer->createService(svcUuid);
    }

    if (!pSvc) {
        NIMBLE_UART_LOGE(LOG_TAG, "Failed to create service");
        return false;
    }

    // Create characteristic with notify + write properties for bidirectional stream
    uint32_t props = NIMBLE_PROPERTY::NOTIFY;
    if (secure) {
        props |= NIMBLE_PROPERTY::READ_ENC;
    }

    if (canWrite) {
        props |= NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR;
        if (secure) {
            props |= NIMBLE_PROPERTY::WRITE_ENC;
        }
    } else {
        m_rxBufSize = 0; // disable RX if not writable
    }

    m_pChr = pSvc->getCharacteristic(chrUuid);
    if (!m_pChr) {
        m_pChr = pSvc->createCharacteristic(chrUuid, props);
    }

    if (!m_pChr) {
        NIMBLE_UART_LOGE(LOG_TAG, "Failed to create characteristic");
        return false;
    }

    m_pChr->setCallbacks(&m_charCallbacks);
    return pSvc->start();
}

void NimBLEStreamServer::deinit() {
    if (m_pChr) {
        NimBLEService* pSvc = m_pChr->getService();
        if (pSvc) {
            pSvc->removeCharacteristic(m_pChr, true);
        }
        m_pChr = nullptr;
    }
    NimBLEStream::end();
}

size_t NimBLEStreamServer::write(const uint8_t* data, size_t len) {
    if (!m_pChr || len == 0 || !hasSubscriber()) {
        return 0;
    }

#   if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 4
    // Skip server gap events to avoid log recursion
    static const char filterStr[] = "handleGapEvent";
    constexpr size_t  filterLen   = sizeof(filterStr) - 1;
    if (len >= filterLen + 3) {
        for (size_t i = 3; i <= len - filterLen; i++) {
            if (memcmp(data + i, filterStr, filterLen) == 0) {
                return len; // drop to avoid recursion
            }
        }
    }
#   endif

    return NimBLEStream::write(data, len);
}

bool NimBLEStreamServer::send(const uint8_t* data, size_t len) {
    if (!m_pChr || !len || !hasSubscriber()) {
        return false;
    }

    size_t offset = 0;
    while (offset < len) {
        size_t chunkLen = std::min(len - offset, getMaxLength());
        while (!m_pChr->notify(data + offset, chunkLen, getPeerHandle())) {
            // Retry on ENOMEM (mbuf shortage)
            if (m_rc == BLE_HS_ENOMEM || os_msys_num_free() <= 2) {
                ble_npl_time_delay(ble_npl_time_ms_to_ticks32(8)); // wait for a minimum connection event time
                continue;
            }
            return false;
        }

        offset += chunkLen;
    }
    return true;
}

void NimBLEStreamServer::ChrCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
    // Push received data into RX buffer
    auto val = pCharacteristic->getValue();
    if (val.size() > 0) {
        m_parent->pushRx(val.data(), val.size());
    }

    if (m_userCallbacks) {
        m_userCallbacks->onWrite(pCharacteristic, connInfo);
    }
}

void NimBLEStreamServer::ChrCallbacks::onSubscribe(NimBLECharacteristic* pCharacteristic,
                                                   NimBLEConnInfo&       connInfo,
                                                   uint16_t              subValue) {
    // only one subscriber supported
    if (m_peerHandle != BLE_HS_CONN_HANDLE_NONE && subValue) {
        return;
    }

    m_peerHandle = subValue ? connInfo.getConnHandle() : BLE_HS_CONN_HANDLE_NONE;
    if (m_peerHandle != BLE_HS_CONN_HANDLE_NONE) {
        m_maxLen = ble_att_mtu(m_peerHandle) - 3;
        if (!m_parent->begin()) {
            NIMBLE_UART_LOGE(LOG_TAG, "NimBLEStreamServer failed to begin");
        }
        return;
    }

    m_parent->end();
    if (m_userCallbacks) {
        m_userCallbacks->onSubscribe(pCharacteristic, connInfo, subValue);
    }
}

void NimBLEStreamServer::ChrCallbacks::onStatus(NimBLECharacteristic* pCharacteristic, int code) {
    m_parent->m_rc = code;
    if (m_userCallbacks) {
        m_userCallbacks->onStatus(pCharacteristic, code);
    }
}

#  endif // MYNEWT_VAL(BLE_ROLE_PERIPHERAL)

#  if MYNEWT_VAL(BLE_ROLE_CENTRAL)
bool NimBLEStreamClient::init(NimBLERemoteCharacteristic* pChr, bool subscribe) {
    if (!pChr) {
        return false;
    }

    m_pChr         = pChr;
    m_writeWithRsp = !pChr->canWriteNoResponse();

    // Subscribe to notifications/indications for RX if requested
    if (subscribe && (pChr->canNotify() || pChr->canIndicate())) {
        using namespace std::placeholders;
        if (!pChr->subscribe(pChr->canNotify(), std::bind(&NimBLEStreamClient::notifyCallback, this, _1, _2, _3, _4))) {
            NIMBLE_UART_LOGE(LOG_TAG, "Failed to subscribe for notifications");
        }
    }

    if (!subscribe) {
        m_rxBufSize = 0; // disable RX if not subscribing
    }

    return true;
}

void NimBLEStreamClient::deinit() {
    if (m_pChr && (m_pChr->canNotify() || m_pChr->canIndicate())) {
        m_pChr->unsubscribe();
    }
    NimBLEStream::end();
    m_pChr = nullptr;
}

size_t NimBLEStreamClient::write(const uint8_t* data, size_t len) {
    if (!m_pChr || !data || len == 0) {
        return 0;
    }
    return NimBLEStream::write(data, len);
}

bool NimBLEStreamClient::send(const uint8_t* data, size_t len) {
    if (!m_pChr || !data || len == 0) {
        return false;
    }
    return m_pChr->writeValue(data, len, m_writeWithRsp);
}

void NimBLEStreamClient::notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify) {
    if (pData && len > 0) {
        pushRx(pData, len);
    }

    if (m_userNotifyCallback) {
        m_userNotifyCallback(pChar, pData, len, isNotify);
    }
}

// UART logging support
int uart_log_printfv(const char* format, va_list arg) {
    static char loc_buf[64];
    char*       temp = loc_buf;
    uint32_t    len;
    va_list     copy;
    va_copy(copy, arg);
    len = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if (len >= sizeof(loc_buf)) {
        temp = (char*)malloc(len + 1);
        if (temp == NULL) {
            return 0;
        }
    }

    int wlen = vsnprintf(temp, len + 1, format, arg);
    for (int i = 0; i < wlen; i++) {
        uart_tx_one_char(temp[i]);
    }

    if (len >= sizeof(loc_buf)) {
        free(temp);
    }

    return wlen;
}

int uart_log_printf(const char* format, ...) {
    int     len;
    va_list arg;
    va_start(arg, format);
    len = uart_log_printfv(format, arg);
    va_end(arg);
    return len;
}

#  endif // MYNEWT_VAL(BLE_ROLE_CENTRAL)
# endif  // CONFIG_BT_NIMBLE_ENABLED && (MYNEWT_VAL(BLE_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_ROLE_CENTRAL))
#endif   // ESP_PLATFORM