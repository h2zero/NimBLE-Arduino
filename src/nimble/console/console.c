#ifndef ESP_PLATFORM

/*#include <stdio.h>*/
#include <stdarg.h>
#include "nrf.h"
#include "console.h"

void ar_printf(const char *format, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf), format, ap);
    for(char *p = &buf[0]; *p; p++)
    {
        if(*p == '\n') {
            NRF_UART0->TXD = '\r';
            while(!NRF_UART0->EVENTS_TXDRDY);
            NRF_UART0->EVENTS_TXDRDY = 0;
        }

        NRF_UART0->TXD = *p;
        while(!NRF_UART0->EVENTS_TXDRDY);
        NRF_UART0->EVENTS_TXDRDY = 0;
    }
    va_end(ap);
}
/*
int _write(int file, const char * p_char, int len)
{
    (void)file;
    int i;

    for (i = 0; i < len; i++)
    {
        NRF_UART0->TXD = *p_char++;
        while(!NRF_UART0->EVENTS_TXDRDY);
        NRF_UART0->EVENTS_TXDRDY = 0;
    }

    return len;
}
*/
#endif
