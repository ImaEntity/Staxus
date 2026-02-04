#include "format.h"
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

// abuse IEEE-754 to get a truncation that actually works
// at values above the signed 64-bit integer limit 
f64 Truncate(f64 num) {
    union {
        f64 dbl;
        u64 lng;
    } BitAccessor = {.dbl = num};

    u64 sign     =  BitAccessor.lng & 0x8000000000000000;
    u64 exponent = (BitAccessor.lng & 0x7FF0000000000000) >> 52;
    u64 mantissa =  BitAccessor.lng & 0x000FFFFFFFFFFFFF;
    i16 correctedExp = (i16) exponent - 1023;

    // nan/inf
    if(exponent == 0x7FF)
        return num;

    // already truncated
    if(correctedExp >= 52)
        return num;

    // fkn headache
    if(correctedExp < 0) {
        BitAccessor.lng = sign;
        return BitAccessor.dbl;
    }

    u64 mask = (1ull << (52 - correctedExp)) - 1ull;
    mantissa &= ~mask;

    BitAccessor.lng =
        sign           |
        exponent << 52 |
        mantissa;

    return BitAccessor.dbl;
}

u64 FloatToString(f64 num, wString str, byte radix, byte precision) {
    f64 integer = Truncate(num);
    f64 fractional = num - integer;

    u64 i = 0;
    if(integer < 0) {
        *(str++) = '-';
        integer = -integer;
        fractional = -fractional;
    }

    while(integer >= 1) {
        f64 truncDiv = Truncate(integer / radix);
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