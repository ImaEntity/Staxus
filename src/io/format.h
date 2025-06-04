#ifndef HH_FORMAT
#define HH_FORMAT

#include "../types.h"

f64 Truncate(f64 num);

u64  UIntToString(u64 num, wString str, byte radix);
u64   IntToString(i64 num, wString str, byte radix);
u64 FloatToString(f64 num, wString str, byte radix, byte precision);

#endif