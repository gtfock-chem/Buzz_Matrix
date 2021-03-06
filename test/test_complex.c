#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>
#include <complex.h>

#include "GTMatrix.h"
#include "utils.h"

/*
Run with: mpirun -np 4 ./test_complex.x
Correct output:
Initial matrix:
0.0+0.0i 0.0+0.0i 0.0+0.0i 0.0+0.0i 1.0-1.0i 1.0-1.0i 
0.0+0.0i 0.0+0.0i 0.0+0.0i 0.0+0.0i 1.0-1.0i 1.0-1.0i 
2.0-2.0i 2.0-2.0i 2.0-2.0i 2.0-2.0i 3.0-3.0i 3.0-3.0i 
2.0-2.0i 2.0-2.0i 2.0-2.0i 2.0-2.0i 3.0-3.0i 3.0-3.0i 
2.0-2.0i 2.0-2.0i 2.0-2.0i 2.0-2.0i 3.0-3.0i 3.0-3.0i 
2.0-2.0i 2.0-2.0i 2.0-2.0i 2.0-2.0i 3.0-3.0i 3.0-3.0i 
Symmetrized matrix:
0.0+0.0i 0.0+0.0i 1.0+1.0i 1.0+1.0i 1.5+0.5i 1.5+0.5i 
0.0+0.0i 0.0+0.0i 1.0+1.0i 1.0+1.0i 1.5+0.5i 1.5+0.5i 
1.0-1.0i 1.0-1.0i 2.0+0.0i 2.0+0.0i 2.5-0.5i 2.5-0.5i 
1.0-1.0i 1.0-1.0i 2.0+0.0i 2.0+0.0i 2.5-0.5i 2.5-0.5i 
1.5-0.5i 1.5-0.5i 2.5+0.5i 2.5+0.5i 3.0+0.0i 3.0+0.0i 
1.5-0.5i 1.5-0.5i 2.5+0.5i 2.5+0.5i 3.0+0.0i 3.0+0.0i 
Added matrix:
5.0+0.0i 5.0+0.0i 6.0+1.0i 6.0+1.0i 6.5+0.5i 6.5+0.5i 
5.0+0.0i 5.0+0.0i 6.0+1.0i 6.0+1.0i 6.5+0.5i 6.5+0.5i 
6.0-1.0i 6.0-1.0i 7.0+0.0i 7.0+0.0i 7.5-0.5i 7.5-0.5i 
6.0-1.0i 6.0-1.0i 7.0+0.0i 7.0+0.0i 7.5-0.5i 7.5-0.5i 
6.5-0.5i 6.5-0.5i 7.5+0.5i 7.5+0.5i 8.0+0.0i 8.0+0.0i 
6.5-0.5i 6.5-0.5i 7.5+0.5i 7.5+0.5i 8.0+0.0i 8.0+0.0i 
*/


void print_complex_matrix(double _Complex *A, int n, int m, const char *Aname)
{
    printf("%s:\n", Aname);
    for (int i = 0; i < n; i++)
    {
        for (int j = 0; j < m; j++) 
        {
            double _Complex Aij = A[i * m + j];
            printf("%.1lf%+.1lfi ", creal(Aij), cimag(Aij));
        }
        printf("\n");
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    
    int r_displs[3] = {0, 2, 6};
    int c_displs[3] = {0, 4, 6};
    double _Complex mat[36];
    
    int my_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    
    MPI_Comm comm_world;
    MPI_Comm_dup(MPI_COMM_WORLD, &comm_world);
    
    GTMatrix_t gtm;
    
    // 2 * 2 proc grid, matrix size 6 * 6
    GTM_create(
        &gtm, comm_world, MPI_C_DOUBLE_COMPLEX, 16, my_rank, 6, 6, 
        2, 2, &r_displs[0], &c_displs[0]
    );
    
    double _Complex dc_fill = (double _Complex)my_rank - (double _Complex)my_rank * I;
    GTM_fill(gtm, &dc_fill);
    GTM_sync(gtm);
    
    if (my_rank == 0)
    {
        GTM_getBlock(gtm, 0, 6, 0, 6, &mat[0], 6);
        print_complex_matrix(&mat[0], 6, 6, "Initial matrix");
    }
    GTM_sync(gtm);
    
    // Symmetrizing
    GTM_symmetrize(gtm);
    if (my_rank == 0)
    {
        GTM_getBlock(gtm, 0, 6, 0, 6, &mat[0], 6);
        print_complex_matrix(&mat[0], 6, 6, "Symmetrized matrix");
    }
    GTM_sync(gtm);
    
    // Accumulation
    if (my_rank == 0)
    {
        for (int i = 0; i < 36; i++) mat[i] = (double _Complex)5.0;
        GTM_accBlock(gtm, 0, 6, 0, 6, &mat[0], 6);
    }
    GTM_sync(gtm);
    if (my_rank == 0)
    {
        GTM_getBlock(gtm, 0, 6, 0, 6, &mat[0], 6);
        print_complex_matrix(&mat[0], 6, 6, "Added matrix");
    }
    GTM_sync(gtm);
    
    GTM_destroy(gtm);
    MPI_Finalize();
}