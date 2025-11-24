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
# ifndef NIMBLE_CPP_STREAM_H
#  define NIMBLE_CPP_STREAM_H

#  include "syscfg/syscfg.h"
#  if CONFIG_BT_NIMBLE_ENABLED && (MYNEWT_VAL(BLE_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_ROLE_CENTRAL))

#   include "NimBLEUUID.h"
#   include <freertos/FreeRTOS.h>
#   include <freertos/ringbuf.h>

#   if NIMBLE_CPP_ARDUINO_STRING_AVAILABLE
#    include <Stream.h>
#   else
// Minimal Stream/Print stubs when Arduino not available
class Print {
  public:
    virtual ~Print() {}
    virtual size_t write(uint8_t)                            = 0;
    virtual size_t write(const uint8_t* buffer, size_t size) = 0;
    size_t         print(const char* s);
    size_t         println(const char* s);
    size_t         printf(const char* format, ...) __attribute__((format(printf, 2, 3)));
};

class Stream : public Print {
  public:
    virtual int   available() = 0;
    virtual int   read()      = 0;
    virtual int   peek()      = 0;
    void          setTimeout(unsigned long timeout) { m_timeout = timeout; }
    unsigned long getTimeout() const { return m_timeout; }

  protected:
    unsigned long m_timeout{0};
};
#   endif

class NimBLEStream : public Stream {
  public:
    NimBLEStream() = default;
    virtual ~NimBLEStream() { end(); }

    bool begin();
    bool end();

    // Configure TX/RX buffer sizes and task parameters before begin()
    void setTxBufSize(uint32_t size) { m_txBufSize = size; }
    void setRxBufSize(uint32_t size) { m_rxBufSize = size; }
    void setTxTaskStackSize(uint32_t size) { m_txTaskStackSize = size; }
    void setTxTaskPriority(uint32_t priority) { m_txTaskPriority = priority; }

    // Print/Stream TX methods
    virtual size_t write(const uint8_t* data, size_t len) override;
    virtual size_t write(uint8_t data) override { return write(&data, 1); }
    size_t         availableForWrite() const;
    void           flush() override;

    // Stream RX methods
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;

    // Serial-like helpers
    bool ready() const { return isReady(); }
         operator bool() const { return ready(); }

    using Print::write;

  protected:
    static void  txTask(void* arg);
    virtual bool send(const uint8_t* data, size_t len) = 0;
    virtual bool isReady() const                       = 0;

    // Push received data into RX ring (called by subclass callbacks)
    size_t pushRx(const uint8_t* data, size_t len);

    RingbufHandle_t m_txBuf{nullptr};
    RingbufHandle_t m_rxBuf{nullptr};
    TaskHandle_t    m_txTask{nullptr};
    uint32_t        m_txTaskStackSize{4096};
    uint32_t        m_txTaskPriority{tskIDLE_PRIORITY + 1};
    uint32_t        m_txBufSize{1024};
    uint32_t        m_rxBufSize{1024};

    // RX peek state
    mutable uint8_t m_peekByte{0};
    mutable bool    m_hasPeek{false};
};

#   if MYNEWT_VAL(BLE_ROLE_PERIPHERAL)
#    include "NimBLECharacteristic.h"

class NimBLEStreamServer : public NimBLEStream {
  public:
    NimBLEStreamServer() : m_charCallbacks(this) {}
    ~NimBLEStreamServer()                                    = default;
    // non-copyable
    NimBLEStreamServer(const NimBLEStreamServer&)            = delete;
    NimBLEStreamServer& operator=(const NimBLEStreamServer&) = delete;

    bool     init(const NimBLEUUID& svcUuid  = NimBLEUUID(uint16_t(0xc0de)),
                  const NimBLEUUID& chrUuid  = NimBLEUUID(uint16_t(0xfeed)),
                  bool              canWrite = false,
                  bool              secure   = false);
    void     deinit();
    size_t   write(const uint8_t* data, size_t len) override;
    uint16_t getPeerHandle() const { return m_charCallbacks.m_peerHandle; }
    bool     hasSubscriber() const { return m_charCallbacks.m_peerHandle != BLE_HS_CONN_HANDLE_NONE; }
    size_t   getMaxLength() const { return m_charCallbacks.m_maxLen; }
    void     setCallbacks(NimBLECharacteristicCallbacks* pCallbacks) { m_charCallbacks.m_userCallbacks = pCallbacks; }

  private:
    bool send(const uint8_t* data, size_t len) override;
    bool isReady() const override { return hasSubscriber(); }

    struct ChrCallbacks : public NimBLECharacteristicCallbacks {
        ChrCallbacks(NimBLEStreamServer* parent)
            : m_parent(parent), m_userCallbacks(nullptr), m_peerHandle(BLE_HS_CONN_HANDLE_NONE), m_maxLen(0) {}
        void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
        void onSubscribe(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, uint16_t subValue) override;
        void onStatus(NimBLECharacteristic* pCharacteristic, int code) override;
        // override this to avoid recursion when debug logs are enabled
        void onStatus(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo, int code) {
            if (m_userCallbacks) {
                m_userCallbacks->onStatus(pCharacteristic, connInfo, code);
            }
        }

        NimBLEStreamServer*            m_parent;
        NimBLECharacteristicCallbacks* m_userCallbacks;
        uint16_t                       m_peerHandle;
        uint16_t                       m_maxLen;
    } m_charCallbacks;

    NimBLECharacteristic* m_pChr{nullptr};
    int                   m_rc{0};
};
#   endif // BLE_ROLE_PERIPHERAL

#   if MYNEWT_VAL(BLE_ROLE_CENTRAL)
#    include "NimBLERemoteCharacteristic.h"

class NimBLEStreamClient : public NimBLEStream {
  public:
    NimBLEStreamClient()                                     = default;
    ~NimBLEStreamClient()                                    = default;
    // non-copyable
    NimBLEStreamClient(const NimBLEStreamClient&)            = delete;
    NimBLEStreamClient& operator=(const NimBLEStreamClient&) = delete;

    // Attach a discovered remote characteristic; app owns discovery/connection.
    // Set subscribeNotify=true to receive notifications into RX buffer.
    bool   init(NimBLERemoteCharacteristic* pChr, bool subscribeNotify = false);
    void   deinit();
    size_t write(const uint8_t* data, size_t len) override;
    void   setWriteWithResponse(bool useWithRsp) { m_writeWithRsp = useWithRsp; }
    void   setNotifyCallback(NimBLERemoteCharacteristic::notify_callback cb) { m_userNotifyCallback = cb; }

  private:
    bool send(const uint8_t* data, size_t len) override;
    bool isReady() const override { return m_pChr != nullptr; }
    void notifyCallback(NimBLERemoteCharacteristic* pChar, uint8_t* pData, size_t len, bool isNotify);

    NimBLERemoteCharacteristic*                 m_pChr{nullptr};
    bool                                        m_writeWithRsp{false};
    NimBLERemoteCharacteristic::notify_callback m_userNotifyCallback{nullptr};
};
#   endif // BLE_ROLE_CENTRAL

#  endif // CONFIG_BT_NIMBLE_ENABLED && (MYNEWT_VAL(BLE_ROLE_PERIPHERAL) || MYNEWT_VAL(BLE_ROLE_CENTRAL))

// These logging macros exist to provide log output over UART so that the stream classes can
// be used to redirect logs without causing recursion issues.
static int uart_log_printfv(const char* format, va_list arg);
static int uart_log_printf(const char* format, ...);

#  if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 4
#   define NIMBLE_UART_LOGD(tag, format, ...) uart_log_printf("D %s: " format "\n", tag, ##__VA_ARGS__)
#  else
#   define NIMBLE_UART_LOGD(tag, format, ...) (void)tag
#  endif

#  if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 3
#   define NIMBLE_UART_LOGI(tag, format, ...) uart_log_printf("I %s: " format "\n", tag, ##__VA_ARGS__)
#  else
#   define NIMBLE_UART_LOGI(tag, format, ...) (void)tag
#  endif

#  if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 2
#   define NIMBLE_UART_LOGW(tag, format, ...) uart_log_printf("W %s: " format "\n", tag, ##__VA_ARGS__)
#  else
#   define NIMBLE_UART_LOGW(tag, format, ...) (void)tag
#  endif

#  if MYNEWT_VAL(NIMBLE_CPP_LOG_LEVEL) >= 1
#   define NIMBLE_UART_LOGE(tag, format, ...) uart_log_printf("E %s: " format "\n", tag, ##__VA_ARGS__)
#  else
#   define NIMBLE_UART_LOGE(tag, format, ...) (void)tag
#  endif

# endif // NIMBLE_CPP_STREAM_H
#endif // ESP_PLATFORM
