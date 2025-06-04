#include "format.h"
#include "../types.h"

u64 UIntToString(u64 num, wString str, byte radix) {
    byte i = 0;
    while(num > 0) {
        byte digit = num % radix;
        str[i] = digit < 10 ? digit + '0' : digit - 10 + 'A';
        
        num /= radix;
        i++;
    }

    str[i] = '\0';

    for(byte j = 0; j < i / 2; j++) {
        byte temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    return i;
}

u64 IntToString(i64 num, wString str, byte radix) {
    u64 inc = 0;
    if(num < 0) {
        num = -num;
        str[0] = '-';
        inc = 1;
        str++;
    }

    return UIntToString(num, str, radix) + inc;
}

u64 FloatToString(f64 num, wString str, byte radix, byte precision) {
    i64 integer = num;
    f64 fractional = num - integer;

    u64 len = IntToString(integer, str, radix);

    if(precision > 0) {
        str[len] = '.';
        len++;

        for(byte i = 0; i < precision; i++) {
            fractional *= radix;
            byte digit = fractional;
            str[len] = digit < 10 ? digit + '0' : digit - 10 + 'A';
            fractional -= digit;
            len++;
        }
    }

    str[len] = '\0';
    return len;
}