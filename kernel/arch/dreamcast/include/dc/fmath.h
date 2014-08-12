/* KallistiOS ##version##

   dc/fmath.h
   Copyright (C) 2001 Andrew Kieschnick
   Copyright (C) 2013, 2014 Lawrence Sebald

*/

#ifndef __DC_FMATH_H
#define __DC_FMATH_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>
#include <dc/fmath_base.h>

/**
    \file   dc/fmath.h
    \brief  Inline functions for the DC's special math instructions
    \author Andrew Kieschnick
    \author Lawrence Sebald
*/

/* Sigh... C99 treats inline stuff a lot differently than traditional GCC did,
   so we need to take care of that... */
#if __STDC_VERSION__ >= 199901L
#define __FMINLINE static inline
#elif __GNUC__
#define __FMINLINE extern inline
#else
/* Uhm... I guess this is the best we can do? */
#define __FMINLINE static
#endif

/**
    \brief  Floating point inner product.
    \return v1 dot v2 (inner product)
*/
__FMINLINE float fipr(float x, float y, float z, float w,
                      float a, float b, float c, float d) {
    return __fipr(x, y, z, w, a, b, c, d);
}

/**
    \brief  Floating point inner product w/self (square of vector magnitude)
    \return v1 dot v1 (square of magnitude)
*/
__FMINLINE float fipr_magnitude_sqr(float x, float y, float z, float w) {
    return __fipr_magnitude_sqr(x, y, z, w);
}

/**
    \brief Floating point sine
    \param r a floating point number between 0 and 2*PI
    \return sin(r), where r is [0..2*PI]
*/
__FMINLINE float fsin(float r) {
    return __fsin(r);
}

/**
    \brief Floating point cosine
    \param r a floating point number between 0 and 2*PI
    \return cos(r), where r is [0..2*PI]
*/
__FMINLINE float fcos(float r) {
    return __fcos(r);
}

/**
    \brief Floating point tangent
    \param r a floating point number between 0 and 2*PI
    \return tan(r), where r is [0..2*PI]
*/
__FMINLINE float ftan(float r) {
    return __ftan(r);
}

/**
    \brief Integer sine
    \param d an integer between 0 and 65535
    \return sin(d), where d is [0..65535]
*/
__FMINLINE float fisin(int d) {
    return __fisin(d);
}

/**
    \brief Integer cosine
    \param d an integer between 0 and 65535
    \return cos(d), where d is [0..65535]
*/
__FMINLINE float ficos(int d) {
    return __ficos(d);
}

/**
    \brief Integer tangent
    \param d an integer between 0 and 65535
    \return tan(d), where d is [0..65535]
*/
__FMINLINE float fitan(int d) {
    return __fitan(d);
}

/**
    \brief Floating point square root
    \return sqrt(f)
*/
__FMINLINE float fsqrt(float f) {
    return __fsqrt(f);
}

/**
    \return 1.0f / sqrt(f)
*/
__FMINLINE float frsqrt(float f) {
    return __frsqrt(f);
}

/** \brief  Calculate the sine and cosine of a value in degrees.

    This function uses the fsca instruction to calculate an approximation of the
    sine and cosine of the input value.

    \param  f               The value to calculate the sine and cosine of.
    \param  s               Storage for the returned sine value.
    \param  c               Storage for the returned cosine value.
*/
__FMINLINE void fsincos(float f, float *s, float *c) {
    __fsincos(f, *s, *c);
}

/** \brief  Calculate the sine and cosine of a value in radians.

    This function uses the fsca instruction to calculate an approximation of the
    sine and cosine of the input value.

    \param  f               The value to calculate the sine and cosine of.
    \param  s               Storage for the returned sine value.
    \param  c               Storage for the returned cosine value.
*/
__FMINLINE void fsincosr(float f, float *s, float *c) {
    __fsincosr(f, *s, *c);
}

/* Make sure we declare the non-inline versions for C99 and non-gcc. Why they'd
   ever be needed, since they're inlined above, who knows? I guess in case
   someone tries to take the address of one of them? */
/** \cond */
#if __STDC_VERSION__ >= 199901L || !defined(__GNUC__)
extern float fipr(float x, float y, float z, float w, float a, float b, float c,
                  float d);
extern float fipr_magnitude_sqr(float x, float y, float z, float w);
extern float fsin(float r);
extern float fcos(float r);
extern float ftan(float r);
extern float fisin(int d);
extern float ficos(int d);
extern float fitan(int d);
extern float fsqrt(float f);
extern float frsqrt(float f);
extern void fsincos(float f, float *s, float *c);
extern void fsincosr(float f, float *s, float *c);
#endif /* __STDC_VERSION__ >= 199901L || !defined(__GNUC__) */
/** \endcond */

__END_DECLS

#endif  /* __DC_FMATH_H */
