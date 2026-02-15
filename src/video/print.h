#ifndef HH_PRINT
#define HH_PRINT

#include <types.h>
#include <file-formats/psf.h>
#include "gop.h"

void InitializePrint(FrameBuffer *fb, PSFFont *font);

void vsprintf(String buf, String fmt, va_list args);
void  sprintf(String buf, String fmt, ...);
void   printf(            String fmt, ...);

void vslprintf(wString buf, wString fmt, va_list args);
void  slprintf(wString buf, wString fmt, ...);
void   lprintf(             wString fmt, ...);

#endif