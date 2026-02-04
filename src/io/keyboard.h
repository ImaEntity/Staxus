#ifndef HH_KEYBOARD
#define HH_KEYBOARD

#include "../types.h"

#define KEY_NOT_READY 0xFFFF
u16 GetScancode();

u16 TranslateScancode(u16 scancode);

#endif