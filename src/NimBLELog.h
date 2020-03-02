/*
 * NimBLELog.h
 *
 *  Created: on Feb 24 2020
 *		Author H2zero
 *
 */
 /*
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#else
#include "esp_log.h"
#endif
*/
#include "modlog/modlog.h"
 
#define NIMBLE_LOGE( tag, format, ... ) MODLOG_DFLT(ERROR, 		"\033[0;31mE %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGW( tag, format, ... ) MODLOG_DFLT(WARN, 		"\033[0;33mW %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGI( tag, format, ... ) MODLOG_DFLT(INFO, 		"\033[0;32mI %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGD( tag, format, ... ) MODLOG_DFLT(DEBUG, 		"D %s: "#format"\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGC( tag, format, ... ) MODLOG_DFLT(CRITICAL, 	"\033[1;31mCRIT %s: "#format"\033[0m\n",tag,##__VA_ARGS__)