#ifndef HH_MATH
#define HH_MATH

#include <types.h>

#define PI 3.14159265358979323
#define E 2.71828182845904523

f32 isnanf(f32 x);
f64 isnan(f64 x);
f32 isinff(f32 x);
f64 isinf(f64 x);

f32 sinf(f32 x);
f64 sin(f64 x);
f32 cosf(f32 x);
f64 cos(f64 x);
f32 tanf(f32 x);
f64 tan(f64 x);

f32 asinf(f32 x);
f64 asin(f64 x);
f32 acosf(f32 x);
f64 acos(f64 x);
f32 atanf(f32 x);
f64 atan(f64 x);
f32 atan2f(f32 y, f32 x);
f64 atan2(f64 y, f64 x);

f32 expf(f32 x);
f64 exp(f64 x);
f32 logf(f32 x);
f64 log(f64 x);
f32 log10f(f32 x);
f64 log10(f64 x);
f32 powf(f32 x, f32 y);
f64 pow(f64 x, f64 y);
f32 sqrtf(f32 x);
f64 sqrt(f64 x);
f32 fmodf(f32 x, f32 y);
f64 fmod(f64 x, f64 y);

f32 ceilf(f32 x);
f64 ceil(f64 x);
f32 floorf(f32 x);
f64 floor(f64 x);
f32 roundf(f32 x);
f64 round(f64 x);
f32 truncf(f32 x);
f64 trunc(f64 x);

f32 fabsf(f32 x);
f64 fabs(f64 x);
f32 fminf(f32 a, f32 b);
f64 fmin(f64 a, f64 b);
f32 fmaxf(f32 a, f32 b);
f64 fmax(f64 a, f64 b);

#endif