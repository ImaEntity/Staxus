#ifndef HH_PRINT
#define HH_PRINT

#include "../types.h"
#include "../file-formats/psf.h"
#include "../graphics/gop.h"

void InitializePrint(FrameBuffer *fb, PSFFont *font);

void vsprintf(wString buf, wString fmt, va_list args);
void  sprintf(wString buf, wString fmt, ...);
void   printf(             wString fmt, ...);

#endif