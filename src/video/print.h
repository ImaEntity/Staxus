#ifndef HH_PRINT
#define HH_PRINT

#include <types.h>
#include <file-formats/psf.h>
#include "gop.h"

void InitializePrint(FrameBuffer *fb, PSFFont *font);

int vsprintf(String buf, String fmt, va_list args);
int  sprintf(String buf, String fmt, ...);
int   printf(            String fmt, ...);

int vslprintf(wString buf, wString fmt, va_list args);
int  slprintf(wString buf, wString fmt, ...);
int   lprintf(             wString fmt, ...);

#endif