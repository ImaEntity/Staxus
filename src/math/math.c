#include "math.h"
#include <types.h>

#define LOG2E 1.44269504088896340
#define LOG10E 0.43429448190325182

inline f32 isnanf(f32 x) {
    union { f32 flt; u32 lng; } BitAccessor = {.flt = x};
    return (BitAccessor.lng & 0x7FFFFFFF) > 0x7F800000;
}

inline f64 isnan(f64 x) {
    union { f64 dbl; u64 lng; } BitAccessor = {.dbl = x};
    return (BitAccessor.lng & 0x7FFFFFFFFFFFFFFF) > 0x7FF0000000000000;
}

inline f32 isinff(f32 x) {
    union { f32 flt; u32 lng; } BitAccessor = {.flt = x};
    return (BitAccessor.lng & 0x7FFFFFFF) == 0x7F800000;
}

inline f64 isinf(f64 x) {
    union { f64 dbl; u64 lng; } BitAccessor = {.dbl = x};
    return (BitAccessor.lng & 0x7FFFFFFFFFFFFFFF) == 0x7FF0000000000000;
}


inline f32 sinf(f32 x) {
    f32 r;
    asm volatile(
        "flds %1\n"
        "fsin\n"
        "fstps %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f64 sin(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fsin\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f32 cosf(f32 x) {
    f32 r;
    asm volatile(
        "flds %1\n"
        "fcos\n"
        "fstps %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f64 cos(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fcos\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f32 tanf(f32 x) {
    f32 r;
    asm volatile(
        "flds %1\n"
        "fptan\n"
        "fstp %%st(0)\n"
        "fstps %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f64 tan(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fptan\n"
        "fstp %%st(0)\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}


inline f32 asinf(f32 x) {
    f32 t = 1.0f - x * x;
    if(t < 0.0f) return __builtin_nan("");
    return atan2f(x, sqrtf(t));
}

inline f64 asin(f64 x) {
    f64 t = 1.0 - x * x;
    if(t < 0.0) return __builtin_nan("");
    return atan2(x, sqrt(t));
}

inline f32 acosf(f32 x) {
    f32 t = 1.0f - x * x;
    if(t < 0.0f) return __builtin_nan("");
    return atan2f(sqrtf(t), x);
}

inline f64 acos(f64 x) {
    f64 t = 1.0 - x * x;
    if(t < 0.0) return __builtin_nan("");
    return atan2(sqrt(t), x);
}

inline f32 atanf(f32 x) {
    return (f32) atan((f64) x);
}

inline f64 atan(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fld1\n"
        "fpatan\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}


inline f32 atan2f(f32 y, f32 x) {
    return (f32) atan2((f64) y, (f64) x);
}

inline f64 atan2(f64 y, f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fldl %2\n"
        "fpatan\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(y), "m"(x)
    );
    
    return r;
}

inline f32 expf(f32 x) {
    return (f32) exp((f64) x);
}

inline f64 exp(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fldl2e\n"
        "fmulp\n"

        // round to int
        "fld %%st(0)\n"
        "frndint\n"

        // dupe int, sub
        "fld %%st(1)\n"
        "fsub %%st(1), %%st(0)\n"

        // 2 ^ frac - 1
        "f2xm1\n"
        "fld1\n"
        "faddp\n"

        // mult by 2 ^ int
        "fscale\n"

        "fstp %%st(1)\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f32 logf(f32 x) {
    return (f32) log((f64) x);
}

inline f64 log(f64 x) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fxtract\n"

        // st0 = sig, st1 = exp
        "fld %%st(0)\n"
        "fld1\n"
        "fyl2x\n"

        // st0 = log2(sig), st1 = sig, st2 = exp
        "fxch %%st(2)\n"
        "faddp\n"

        "fldln2\n"
        "fmulp\n"

        "fstp %%st(1)\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x)
    );

    return r;
}

inline f32 log10f(f32 x) {
    return (f32) log10((f64) x);
}

inline f64 log10(f64 x) {
    return log(x) * LOG10E;
}

inline f32 powf(f32 x, f32 y) {
    return (f32) pow((f64) x, (f64) y);
}

inline f64 pow(f64 x, f64 y) {
    return exp(y * log(x));
}

inline f32 sqrtf(f32 x) {
    f32 r;
    asm volatile(
        "sqrtss %1, %0"
        : "=x"(r)
        : "x"(x)
    );

    return r;
}

inline f64 sqrt(f64 x) {
    f64 r;
    asm volatile(
        "sqrtsd %1, %0"
        : "=x"(r)
        : "x"(x)
    );

    return r;
}

f32 fmodf(f32 x, f32 y) {
    f32 r;
    asm volatile(
        "flds %1\n"
        "flds %2\n"
        "1:\n"
        "fprem\n"
        "fnstsw %%ax\n"
        "testw $0x0400, %%ax\n"
        "jnz 1b\n"
        "fstp %%st(0)\n"
        "fstps %0"
        : "=m"(r)
        : "m"(x), "m"(y)
        : "ax"
    );

    return r;
}

f64 fmod(f64 x, f64 y) {
    f64 r;
    asm volatile(
        "fldl %1\n"
        "fldl %2\n"
        "1:\n"
        "fprem\n"
        "fnstsw %%ax\n"
        "testw $0x0400, %%ax\n"
        "jnz 1b\n"
        "fstp %%st(0)\n"
        "fstpl %0"
        : "=m"(r)
        : "m"(x), "m"(y)
        : "ax"
    );

    return r;
}


inline f32 ceilf(f32 x) {
    f32 t = truncf(x);
    return x > t ? t + 1.0f : t;
}

inline f64 ceil(f64 x) {
    f64 t = trunc(x);
    return x > t ? t + 1.0 : t;
}

inline f32 floorf(f32 x) {
    f32 t = truncf(x);
    return x < t ? t - 1.0f : t;
}

inline f64 floor(f64 x) {
    f64 t = trunc(x);
    return x < t ? t - 1.0 : t;
}

inline f32 roundf(f32 x) {
    f32 t = truncf(x);
    return
        x - t >=  0.5f ? t + 1.0f :
        x - t <= -0.5f ? t - 1.0f :
        t;
}

inline f64 round(f64 x) {
    f64 t = trunc(x);
    return
        x - t >=  0.5 ? t + 1.0 :
        x - t <= -0.5 ? t - 1.0 :
        t;
}

// abuse IEEE-754 to get a truncation that actually works
// at values above the signed 64-bit integer limit
f32 truncf(f32 x) {
    union {
        f32 flt;
        u32 lng;
    } BitAccessor = {.flt = x};

    u32 sign     =  BitAccessor.lng & 0x80000000;
    u32 exponent = (BitAccessor.lng & 0x7F800000) >> 23;
    u32 mantissa =  BitAccessor.lng & 0x007FFFFF;
    i8 correctedExp = (i8) exponent - 127;

    if(exponent == 0x7FF) return x; // nan/inf
    if(correctedExp >= 23) return x; // already truncated

    // fkn headache
    if(correctedExp < 0) {
        BitAccessor.lng = sign;
        return BitAccessor.flt;
    }

    u64 mask = (1ull << (23 - correctedExp)) - 1ull;
    mantissa &= ~mask;

    BitAccessor.lng =
        sign           |
        exponent << 23 |
        mantissa;

    return BitAccessor.flt;
}

f64 trunc(f64 x) {
    union {
        f64 dbl;
        u64 lng;
    } BitAccessor = {.dbl = x};

    u64 sign     =  BitAccessor.lng & 0x8000000000000000;
    u64 exponent = (BitAccessor.lng & 0x7FF0000000000000) >> 52;
    u64 mantissa =  BitAccessor.lng & 0x000FFFFFFFFFFFFF;
    i16 correctedExp = (i16) exponent - 1023;

    if(exponent == 0x7FF) return x; // nan/inf
    if(correctedExp >= 52) return x; // already truncated

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

inline f32 fabsf(f32 x) { return x < 0 ? -x : x; }
inline f64 fabs(f64 x) { return x < 0 ? -x : x; }
inline f32 fminf(f32 a, f32 b) { return a < b ? a : b; }
inline f64 fmin(f64 a, f64 b) { return a < b ? a : b; }
inline f32 fmaxf(f32 a, f32 b) { return a > b ? a : b; }
inline f64 fmax(f64 a, f64 b) { return a > b ? a : b; }