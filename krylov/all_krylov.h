/*BHEADER**********************************************************************
 * (c) 2000   The Regents of the University of California
 *
 * See the file COPYRIGHT_and_DISCLAIMER for a complete copyright
 * notice, contact person, and disclaimer.
 *
 * $Revision$
 *********************************************************************EHEADER*/

/******************************************************************************
 *
 * WARNING: The file krylov.h should NOT be edited by hand.
 * It is automatically generated by the script "headers", which should be run
 * frequently.  Almost every other hypre directory is set up this way, the
 * krylov directory is no different.
 *
 *****************************************************************************
 *
 * krylov solver headers
 *
 *****************************************************************************/

#ifndef HYPRE_ALL_KRYLOV_HEADER
#define HYPRE_ALL_KRYLOV_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#ifndef max
#define max(a,b)  (((a)<(b)) ? (b) : (a))
#endif

#define hypre_CTAllocF(type, count, funcs) \
( (type *)(*(funcs->CAlloc))\
((unsigned int)(count), (unsigned int)sizeof(type)) )

#define hypre_TFreeF( ptr, funcs ) \
( (*(funcs->Free))((char *)ptr), ptr = NULL )

/* A pointer to a type which is never defined, sort of works like void* ... */
#ifndef HYPRE_SOLVER_STRUCT
#define HYPRE_SOLVER_STRUCT
struct hypre_Solver_struct;
typedef struct hypre_Solver_struct *HYPRE_Solver;
/* similar pseudo-void* for Matrix and Vector: */
#endif
#ifndef HYPRE_MATRIX_STRUCT
#define HYPRE_MATRIX_STRUCT
struct hypre_Matrix_struct;
typedef struct hypre_Matrix_struct *HYPRE_Matrix;
#endif
#ifndef HYPRE_VECTOR_STRUCT
#define HYPRE_VECTOR_STRUCT
struct hypre_Vector_struct;
typedef struct hypre_Vector_struct *HYPRE_Vector;
#endif

typedef int (*HYPRE_PtrToSolverFcn)(HYPRE_Solver,
                                    HYPRE_Matrix,
                                    HYPRE_Vector,
                                    HYPRE_Vector);

#endif
