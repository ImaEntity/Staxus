#ifndef HH_IO_MOUSE
#define HH_IO_MOUSE

#include <types.h>

#define MOUSE_BUTTON_LEFT    0x01
#define MOUSE_BUTTON_RIGHT   0x02
#define MOUSE_BUTTON_MIDDLE  0x04
#define MOUSE_BUTTON_BACK    0x10
#define MOUSE_BUTTON_FORWARD 0x20

typedef struct {
    u32 prevX; u32 prevY;
    u32 curX; u32 curY;
    i8 scrollAmount;
    byte prevButtons;
    byte buttons;
} MouseEvent;

u8 InitializeMouse(void (*onEvent)(MouseEvent));

#endif