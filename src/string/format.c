#include "format.h"
#include <math/math.h>
#include <types.h>

u64 UIntToString(u64 num, wString str, byte radix) {
    byte i = 0;
    while(num > 0) {
        byte digit = num % radix;
        str[i] = digit < 10 ? digit + '0' : digit - 10 + 'A';
        
        num /= radix;
        i++;
    }

    if(i == 0) str[i++] = '0';
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
    f64 integer = trunc(num);
    f64 fractional = num - integer;

    u64 i = 0;
    if(integer < 0) {
        *(str++) = '-';
        integer = -integer;
        fractional = -fractional;
    }

    while(integer >= 1) {
        f64 truncDiv = trunc(integer / radix);
        f64 rem = integer - truncDiv * radix;
        byte digit = (byte) rem;

        str[i++] = digit < 10 ? digit + '0' : digit - 10 + 'A';
        integer = truncDiv;
    }

    if(i == 0) str[i++] = '0';
    
    for(byte j = 0; j < i / 2; j++) {
        byte temp = str[j];
        str[j] = str[i - j - 1];
        str[i - j - 1] = temp;
    }

    if(precision > 0 && fractional != 0) {
        str[i++] = '.';

        for(byte j = 0; j < precision; j++) {
            if(fractional == 0) break;
            fractional *= radix;

            byte digit = fractional;
            str[i++] = digit < 10 ? digit + '0' : digit - 10 + 'A';
            
            fractional -= digit;
        }
    }

    str[i] = '\0';
    return i;
}