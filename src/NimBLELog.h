/*
 * NimBLELog.h
 *
 *  Created: on Feb 24 2020
 *      Author H2zero
 *
 */
#ifndef MAIN_NIMBLELOG_H_
#define MAIN_NIMBLELOG_H_
#include "sdkconfig.h"
#if defined(CONFIG_BT_ENABLED)

#include "modlog/modlog.h"
 
#define NIMBLE_LOGE( tag, format, ... ) MODLOG_DFLT(ERROR,      "\033[0;31mE %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGW( tag, format, ... ) MODLOG_DFLT(WARN,       "\033[0;33mW %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGI( tag, format, ... ) MODLOG_DFLT(INFO,       "\033[0;32mI %s: "#format"\033[0m\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGD( tag, format, ... ) MODLOG_DFLT(DEBUG,      "D %s: "#format"\n",tag,##__VA_ARGS__)
#define NIMBLE_LOGC( tag, format, ... ) MODLOG_DFLT(CRITICAL,   "\033[1;31mCRIT %s: "#format"\033[0m\n",tag,##__VA_ARGS__)

#endif /*CONFIG_BT_ENABLED*/
#endif /*MAIN_NIMBLELOG_H_*/