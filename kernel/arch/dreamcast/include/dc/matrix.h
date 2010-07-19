/* KallistiOS ##version##

   matrix.h
   (c)2000 Dan Potter

*/

/** \file   dc/matrix.h
    \brief  Basic matrix operations.

    This file contains various basic matrix math functionality for using the
    SH4's matrix transformation unit. Higher level functionality, like the 3D
    functionality is built off of these operations.

    \author Dan Potter
    \see    dc/matrix3d.h
*/

#ifndef __DC_MATRIX_H
#define __DC_MATRIX_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <kos/vector.h>

/** \brief  Copy the internal matrix to a memory one.

    This function stores the current internal matrix to one in memory.

    \param  out             A pointer to where to store the matrix (must be at
                            least 8-byte aligned, should be 32-byte aligned).
*/
void mat_store(matrix_t *out);

/** \brief  Copy a memory matrix into the internal one.

    This function loads the internal matrix with the values of one in memory.

    \param  out             A pointer to where to load the matrix from (must be
                            at least 8-byte aligned, should be 32-byte aligned).
*/
void mat_load(matrix_t *out);

/** \brief  Clear the internal matrix to identity.

    This function clears the internal matrix to a standard identity matrix.
*/
void mat_identity();

/** \brief  Apply a matrix.

    This function multiplies a matrix in memory onto the internal matrix.

    \param  src             A poitner to the matrix to multiply.
*/
void mat_apply(matrix_t *src);

/** \brief  Transform vectors by the internal matrix.

    This function transforms zero or more sets of vectors by the current
    internal matrix. Each vector is 3 single-precision floats long.

    \param  invecs          The list of input vectors.
    \param  outvecs         The list of output vectors.
    \param  veccnt          How many vectors are in the list.
    \param  vecskip         Number of empty bytes between vectors.
*/
void mat_transform(vector_t *invecs, vector_t *outvecs, int veccnt, int vecskip);

/** \brief  Transform vectors by the internal matrix into the store queues.

    This function transforms one or more sets of vertices using the current
    internal matrix directly into the store queues. Each vertex is exactly
    32-bytes long, and the non-xyz data that is with it will be copied over with
    the transformed coordinates. This is perfect, for instance, for transforming
    pvr_vertex_t vertices.

    \param  input           The list of input vertices.
    \param  output          The output pointer.
    \param  veccnt          The number of vertices to transform.
    \note                   The \ref QACR0 and \ref QACR1 registers must be set
                            appropriately BEFORE calling this function.
    \author Jim Ursetto
*/
void mat_transform_sq(void * input, void * output, int veccnt);

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, and z is in
    fr2 before using the macro.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
*/
#define mat_trans_single(x, y, z) { \
	register float __x __asm__("fr0") = (x); \
	register float __y __asm__("fr1") = (y); \
	register float __z __asm__("fr2") = (z); \
	__asm__ __volatile__( \
		"fldi1	fr3\n" \
		"ftrv	xmtrx,fv0\n" \
		"fldi1	fr2\n" \
		"fdiv	fr3,fr2\n" \
		"fmul	fr2,fr0\n" \
		"fmul	fr2,fr1\n" \
		: "=f" (__x), "=f" (__y), "=f" (__z) \
		: "0" (__x), "1" (__y), "2" (__z) \
		: "fr3" ); \
	x = __x; y = __y; z = __z; \
}

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, z is in
    fr2, and w is in fr3 before using the macro. This macro is similar to
    mat_trans_single(), but this one allows an input to and preserves the Z/W
    value.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
    \param  w               The W coordinate to transform.
*/
#define mat_trans_single4(x, y, z, w) { \
	register float __x __asm__("fr0") = (x); \
	register float __y __asm__("fr1") = (y); \
	register float __z __asm__("fr2") = (z); \
	register float __w __asm__("fr3") = (w); \
	__asm__ __volatile__( \
		"ftrv	xmtrx,fv0\n" \
		"fdiv	fr3,fr0\n" \
		"fdiv	fr3,fr1\n" \
		"fdiv	fr3,fr2\n" \
		"fldi1	fr4\n" \
		"fdiv	fr3,fr4\n" \
		"fmov	fr4,fr3\n" \
		: "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
		: "0" (__x), "1" (__y), "2" (__z), "3" (__w) \
		: "fr4" ); \
	x = __x; y = __y; z = __z; w = __w; \
}

/** \brief  Macro to transform a single vertex by the internal matrix.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, and z is in
    fr2 before using the macro. This macro is similar to mat_trans_single(), but
    this one leaves z/w instead of 1/w for the z component.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
*/
#define mat_trans_single3(x, y, z) { \
	register float __x __asm__("fr0") = (x); \
	register float __y __asm__("fr1") = (y); \
	register float __z __asm__("fr2") = (z); \
	__asm__ __volatile__( \
		"fldi1	fr3\n" \
		"ftrv	xmtrx,fv0\n" \
		"fdiv	fr3,fr0\n" \
		"fdiv	fr3,fr1\n" \
		"fdiv	fr3,fr2\n" \
		: "=f" (__x), "=f" (__y), "=f" (__z) \
		: "0" (__x), "1" (__y), "2" (__z) \
		: "fr3" ); \
	x = __x; y = __y; z = __z; \
}

/** \brief  Macro to transform a single vertex by the internal matrix with no
            perspective division.

    This macro is an inline assembly operation to transform a single vertex. It
    works most efficiently if the x value is in fr0, y is in fr1, z is in
    fr2, and w is in fr3 before using the macro. This macro is similar to
    mat_trans_single(), but this one does not do any perspective division.

    \param  x               The X coordinate to transform.
    \param  y               The Y coordinate to transform.
    \param  z               The Z coordinate to transform.
    \param  w               The W coordinate to transform.
*/
#define mat_trans_nodiv(x, y, z, w) { \
	register float __x __asm__("fr0") = (x); \
	register float __y __asm__("fr1") = (y); \
	register float __z __asm__("fr2") = (z); \
	register float __w __asm__("fr3") = (w); \
	__asm__ __volatile__( \
		"ftrv   xmtrx,fv0\n" \
		: "=f" (__x), "=f" (__y), "=f" (__z), "=f" (__w) \
		: "0" (__x), "1" (__y), "2" (__z), "3" (__w) ); \
	x = __x; y = __y; z = __z; w = __w; \
}


__END_DECLS

#endif	/* __DC_MATRIX_H */

