/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    //Reference to https://zhuanlan.zhihu.com/p/42754565
    int i, j, m, n, temp0, temp1, temp2, temp3, temp4, temp5, temp6, temp7;
    if (M == 32) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (m = i; m < i + 8; m++) {
                    temp0 = A[m][j];
                    temp1 = A[m][j + 1];
                    temp2 = A[m][j + 2];
                    temp3 = A[m][j + 3];
                    temp4 = A[m][j + 4];
                    temp5 = A[m][j + 5];
                    temp6 = A[m][j + 6];
                    temp7 = A[m][j + 7];
                    B[j][m] = temp0;
                    B[j + 1][m] = temp1;
                    B[j + 2][m] = temp2;
                    B[j + 3][m] = temp3;
                    B[j + 4][m] = temp4;
                    B[j + 5][m] = temp5;
                    B[j + 6][m] = temp6;
                    B[j + 7][m] = temp7;
                }
            }
        }
    } else if (M == 64) {
        for (i = 0; i < N; i += 8) {
            for (j = 0; j < M; j += 8) {
                for (m = i; m < i + 4; m++) {
                    temp0 = A[m][j + 0];
                    temp1 = A[m][j + 1];
                    temp2 = A[m][j + 2];
                    temp3 = A[m][j + 3];
                    temp4 = A[m][j + 4];
                    temp5 = A[m][j + 5];
                    temp6 = A[m][j + 6];
                    temp7 = A[m][j + 7];

                    B[j + 0][m] = temp0;
                    B[j + 1][m] = temp1;
                    B[j + 2][m] = temp2;
                    B[j + 3][m] = temp3;
                    B[j + 0][m + 4] = temp7;
                    B[j + 1][m + 4] = temp6;
                    B[j + 2][m + 4] = temp5;
                    B[j + 3][m + 4] = temp4;
                }
                for (n = 0; n < 4; n++) {
                    temp0 = A[i + 4][j + 3 - n];
                    temp1 = A[i + 5][j + 3 - n];
                    temp2 = A[i + 6][j + 3 - n];
                    temp3 = A[i + 7][j + 3 - n];
                    temp4 = A[i + 4][j + 4 + n];
                    temp5 = A[i + 5][j + 4 + n];
                    temp6 = A[i + 6][j + 4 + n];
                    temp7 = A[i + 7][j + 4 + n];

                    B[j + 4 + n][i + 4] = temp4;
                    B[j + 4 + n][i + 5] = temp5;
                    B[j + 4 + n][i + 6] = temp6;
                    B[j + 4 + n][i + 7] = temp7;

                    B[j + 4 + n][i + 0] = B[j + 3 - n][i + 4];
                    B[j + 4 + n][i + 1] = B[j + 3 - n][i + 5];
                    B[j + 4 + n][i + 2] = B[j + 3 - n][i + 6];
                    B[j + 4 + n][i + 3] = B[j + 3 - n][i + 7];

                    B[j + 3 - n][i + 4] = temp0;
                    B[j + 3 - n][i + 5] = temp1;
                    B[j + 3 - n][i + 6] = temp2;
                    B[j + 3 - n][i + 7] = temp3;
                }
            }
        }
    } else {
        for (i = 0; i < N; i += 16) {
            for (j = 0; j < M; j += 16) {
                for (m = i; m < i + 16 && m < N; m++) {
                    for (n = j; n < j + 16 && n < M; n++) {
                        B[n][m] = A[m][n];
                    }
                }
            }
        }
    } 
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}

