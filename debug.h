#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>
#include <stdio.h>

// RTL8720DN (AmebaD) 'de Serial.printf() ve Print.printf() bulunmaz.
// Asagidaki makrolar snprintf + Serial.print kullanarak ayni isi yapar.
// RTL SDK diag.h DBG_PRINTF(MODULE, LEVEL, pFormat, ...) seklinde 4 parametreli tanimlar.
// Biz 1-2 parametre ile kullandigimiz icin SDK versiyonu uyumsuz. Once undef, sonra kendi tanimimiz.
#ifdef DBG_PRINTF
#undef DBG_PRINTF
#endif
#define DBG_PRINTF(fmt, ...) do { \
    char dbg_buf[200]; \
    snprintf(dbg_buf, sizeof(dbg_buf), fmt, ##__VA_ARGS__); \
    Serial.print(dbg_buf); \
} while(0)

// Print sinifi icin (output.printf yerine)
// RTL SDK diag.h DBG_PRINTF_PRINT tanimlamiyor ama emin olmak icin #ifndef
#ifndef DBG_PRINTF_PRINT
#define DBG_PRINTF_PRINT(out, fmt, ...) do { \
    char dbg_buf[200]; \
    snprintf(dbg_buf, sizeof(dbg_buf), fmt, ##__VA_ARGS__); \
    out.print(dbg_buf); \
} while(0)
#endif // DBG_PRINTF_PRINT

#endif // DEBUG_H
