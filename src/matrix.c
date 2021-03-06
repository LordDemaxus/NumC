#include "matrix.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// Include SSE intrinsics
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#include <x86intrin.h>
#endif

/* Generates a random double between low and high */
double rand_double(double low, double high) {
    double range = (high - low);
    double div = RAND_MAX / range;
    return low + (rand() / div);
}

/* Generates a random matrix */
void rand_matrix(matrix *result, unsigned int seed, double low, double high) {
    srand(seed);
    for (int i = 0; i < result->rows; i++) {
        for (int j = 0; j < result->cols; j++) {
            set(result, i, j, rand_double(low, high));
        }
    }
}

/*
 * Returns the double value of the matrix at the given row and column.
 * You may assume `row` and `col` are valid. Note that the matrix is in row-major order.
 */
double get(matrix *mat, int row, int col) {
    // Task 1.1 TODO
    int e = row * (mat->cols) + col;
    return (mat->data)[e];
}

/*
 * Sets the value at the given row and column to val. You may assume `row` and
 * `col` are valid. Note that the matrix is in row-major order.
 */
void set(matrix *mat, int row, int col, double val) {
    // Task 1.1 TODO
    int e = row * (mat->cols) + col;
    (mat->data)[e] = val;
}

/*
 * Allocates space for a matrix struct pointed to by the double pointer mat with
 * `rows` rows and `cols` columns. You should also allocate memory for the data array
 * and initialize all entries to be zeros. `parent` should be set to NULL to indicate that
 * this matrix is not a slice. You should also set `ref_cnt` to 1.
 * You should return -1 if either `rows` or `cols` or both have invalid values. Return -2 if any
 * call to allocate memory in this function fails.
 * Return 0 upon success.
 */
int allocate_matrix(matrix **mat, int rows, int cols) {

    if (rows <= 0 || cols <= 0) {
        return -1;
    }
    matrix* newM = (matrix*) malloc(sizeof(matrix));
    if (newM == NULL) {
        return -2;
    }
    newM->data = (double*) calloc(rows*cols, sizeof(double)); 
    if (newM->data == NULL) {
        return -2;
    }
    newM->rows = rows;
    newM->cols = cols;
    newM->parent = NULL;
    newM->ref_cnt = 1;
    *mat = newM;
    return 0;
}

/*
 * You need to make sure that you only free `mat->data` if `mat` is not a slice and has no existing slices,
 * or that you free `mat->parent->data` if `mat` is the last existing slice of its parent matrix and its parent
 * matrix has no other references (including itself).
 */
void deallocate_matrix(matrix *mat) {
    if (mat == NULL) {
        return;
    }
    if (mat->parent == NULL) {
        mat->ref_cnt -= 1;
        if (mat->ref_cnt == 0) {
            free(mat->data);
            free(mat);
        }
    } else {
        deallocate_matrix(mat->parent);
        free(mat);
    }
}
/*
 * Allocates space for a matrix struct pointed to by `mat` with `rows` rows and `cols` columns.
 * Its data should point to the `offset`th entry of `from`'s data (you do not need to allocate memory)
 * for the data field. `parent` should be set to `from` to indicate this matrix is a slice of `from`
 * and the reference counter for `from` should be incremented. Lastly, do not forget to set the
 * matrix's row and column values as well.
 * You should return -1 if either `rows` or `cols` or both have invalid values. Return -2 if any
 * call to allocate memory in this function fails.
 * Return 0 upon success.
 * NOTE: Here we're allocating a matrix struct that refers to already allocated data, so
 * there is no need to allocate space for matrix data.
 */
int allocate_matrix_ref(matrix **mat, matrix *from, int offset, int rows, int cols) {
   
    if (rows <= 0 || cols <= 0) {
        return -1;
    }
    matrix* newM = (matrix*) malloc(sizeof(matrix));
    if (newM == NULL) {
        return -2;
    }
    newM->data = from->data + offset; 
    newM->rows = rows;
    newM->cols = cols;
    newM->parent = from;
    from->ref_cnt += 1;
    *mat = newM;
    return 0;
}

/*
 * Sets all entries in mat to val. Note that the matrix is in row-major order.
 */
void fill_matrix(matrix *mat, double val) {
    // Task 1.5 TODO
        __m256d t = _mm256_set1_pd(val);
        #pragma omp parallel for
        for (int i = 0; i < (mat->rows * mat->cols) / 16 * 16; i+=16) {
            _mm256_storeu_pd(mat->data + i, t);
            _mm256_storeu_pd(mat->data + i + 4, t);
            _mm256_storeu_pd(mat->data + i + 8, t);
            _mm256_storeu_pd(mat->data + i + 12, t);
        }
        #pragma omp parallel for
        for(int i = (mat->rows * mat->cols) / 16 * 16; i < (mat->rows * mat->cols); i+=1) {
                mat->data[i] = val;
            }
}

/*
 * Store the result of taking the absolute value element-wise to `result`.
 * Return 0 upon success.
 * Note that the matrix is in row-major order.
 */
int abs_matrix(matrix *result, matrix *mat) {
        __m256d zero = _mm256_set_pd(0, 0, 0, 0);
        #pragma omp parallel for
        for (int i = 0; i < (mat->rows * mat->cols) / 16 * 16; i+=16) {
            __m256d t = _mm256_loadu_pd(mat->data + i);
            __m256d neg = _mm256_sub_pd(zero, t);
            t = _mm256_max_pd(t, neg);
            _mm256_storeu_pd(result->data+i, t);

            t = _mm256_loadu_pd(mat->data + i + 4);
            neg = _mm256_sub_pd(zero, t);
            t = _mm256_max_pd(t, neg);
            _mm256_storeu_pd(result->data+i + 4, t);

            t = _mm256_loadu_pd(mat->data + i + 8);
            neg = _mm256_sub_pd(zero, t);
            t = _mm256_max_pd(t, neg);
            _mm256_storeu_pd(result->data+i + 8, t);

            t = _mm256_loadu_pd(mat->data + i + 12);
            neg = _mm256_sub_pd(zero, t);
            t = _mm256_max_pd(t, neg);
            _mm256_storeu_pd(result->data+i+12, t);

            }
        #pragma omp parallel for
        for(int i = (mat->rows * mat->cols) / 16 * 16; i < (mat->rows * mat->cols); i+=1) {
            result->data[i] = fabs(mat->data[i]);
        }
    return 0;
}

/*
 * Store the result of element-wise negating mat's entries to `result`.
 * Return 0 upon success.
 * Note that the matrix is in row-major order.
 */
int neg_matrix(matrix *result, matrix *mat) {
            __m256d zero = _mm256_set_pd(0, 0, 0, 0);
        #pragma omp parallel for
        for (int i = 0; i < (mat->rows * mat->cols) / 16 * 16; i+=16) {
            __m256d t = _mm256_loadu_pd(mat->data + i);
            __m256d neg = _mm256_sub_pd(zero, t);
            _mm256_storeu_pd(result->data+i, neg);

            t = _mm256_loadu_pd(mat->data + i + 4);
            neg = _mm256_sub_pd(zero, t);
            _mm256_storeu_pd(result->data+i + 4, neg);

            t = _mm256_loadu_pd(mat->data + i + 8);
            neg = _mm256_sub_pd(zero, t);
            _mm256_storeu_pd(result->data+i + 8, neg);

            t = _mm256_loadu_pd(mat->data + i + 12);
            neg = _mm256_sub_pd(zero, t);
            _mm256_storeu_pd(result->data+i+12, neg);

            }
        #pragma omp parallel for
        for(int i = (mat->rows * mat->cols) / 16 * 16; i < (mat->rows * mat->cols); i+=1) {
            result->data[i] = -1 * mat->data[i];
        }
    return 0;
}

/*
 * Store the result of adding mat1 and mat2 to `result`.
 * Return 0 upon success.
 * You may assume `mat1` and `mat2` have the same dimensions.
 * Note that the matrix is in row-major order.
 */
int add_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    #pragma omp parallel for
    for (int i = 0; i < (mat1->rows * mat1->cols) / 16 * 16; i+=16) {
        __m256d t1 = _mm256_loadu_pd(mat1->data + i);
        __m256d t2 = _mm256_loadu_pd(mat2->data + i);
        _mm256_storeu_pd(result->data+i, _mm256_add_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 4);
        t2 = _mm256_loadu_pd(mat2->data + i + 4);
        _mm256_storeu_pd(result->data+i + 4, _mm256_add_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 8);
        t2 = _mm256_loadu_pd(mat2->data + i + 8);
        _mm256_storeu_pd(result->data+i + 8, _mm256_add_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 12);
        t2 = _mm256_loadu_pd(mat2->data + i + 12);
        _mm256_storeu_pd(result->data+i + 12, _mm256_add_pd(t1, t2));
        }
    #pragma omp parallel for
    for(int i = (mat1->rows * mat1->cols) / 16 * 16; i < (mat1->rows * mat1->cols); i+=1) {
        result->data[i] = mat1->data[i]+mat2->data[i];   
        }
    return 0;
}

/*
 * Store the result of subtracting mat2 from mat1 to `result`.
 * Return 0 upon success.
 * You may assume `mat1` and `mat2` have the same dimensions.
 * Note that the matrix is in row-major order.
 */
int sub_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    #pragma omp parallel for
    for (int i = 0; i < (mat1->rows * mat1->cols) / 16 * 16; i+=16) {
        __m256d t1 = _mm256_loadu_pd(mat1->data + i);
        __m256d t2 = _mm256_loadu_pd(mat2->data + i);
        _mm256_storeu_pd(result->data+i, _mm256_sub_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 4);
        t2 = _mm256_loadu_pd(mat2->data + i + 4);
        _mm256_storeu_pd(result->data+i + 4, _mm256_sub_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 8);
        t2 = _mm256_loadu_pd(mat2->data + i + 8);
        _mm256_storeu_pd(result->data+i + 8, _mm256_sub_pd(t1, t2));
    
        t1 = _mm256_loadu_pd(mat1->data + i + 12);
        t2 = _mm256_loadu_pd(mat2->data + i + 12);
        _mm256_storeu_pd(result->data+i + 12, _mm256_sub_pd(t1, t2));
        }
    #pragma omp parallel for
    for(int i = (mat1->rows * mat1->cols) / 16 * 16; i < (mat1->rows * mat1->cols); i+=1) {
        result->data[i] = mat1->data[i] - mat2->data[i];   
        }
    return 0;
}

/* 
* Helper function
* Transposes matrix for efficient matrix multiplication 
*/
void transpose_matrix(matrix *result, matrix *mat) {
    #pragma omp parallel for
    for (int i = 0; i < mat->rows; i++) {
        for (int j = 0; j < mat->cols; j++) {
                set(result, j, i, get(mat, i, j));    
        }
    }
}

/*
 * Store the result of multiplying mat1 and mat2 to `result`.
 * Return 0 upon success.
 * Remember that matrix multiplication is not the same as multiplying individual elements.
 * You may assume `mat1`'s number of columns is equal to `mat2`'s number of rows.
 * Note that the matrix is in row-major order.
 */
int mul_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    // Task 1.6 TODO
    matrix* tmat2 = NULL;
    allocate_matrix(&tmat2, mat2->cols, mat2->rows);
    transpose_matrix(tmat2, mat2);
    #pragma omp parallel for collapse(2)
        for (int i = 0; i < mat1->rows; i++) {
            for (int j = 0; j < mat2->cols; j++) {
                __m256d t = _mm256_set1_pd(0);
                for (int k = 0; k < tmat2->cols / 16 * 16; k+=16) {
                    t = _mm256_fmadd_pd(_mm256_loadu_pd((mat1->data)+i*(mat1->cols)+k), _mm256_loadu_pd((tmat2->data)+j*(tmat2->cols)+k), t);
                
                    t = _mm256_fmadd_pd(_mm256_loadu_pd((mat1->data)+i*(mat1->cols)+k + 4), _mm256_loadu_pd((tmat2->data)+j*(tmat2->cols)+k + 4), t);

                    t = _mm256_fmadd_pd(_mm256_loadu_pd((mat1->data)+i*(mat1->cols)+k + 8), _mm256_loadu_pd((tmat2->data)+j*(tmat2->cols)+k + 8), t);

                    t = _mm256_fmadd_pd(_mm256_loadu_pd((mat1->data)+i*(mat1->cols)+k + 12), _mm256_loadu_pd((tmat2->data)+j*(tmat2->cols)+k + 12), t);
            }

                double full[4]; 
                _mm256_storeu_pd(full, t);
                double res = full[0]+full[1]+full[2]+full[3];
                for (int k = tmat2->cols / 16* 16; k < tmat2->cols; k++) {
                    res += get(mat1, i, k) * get(tmat2, j, k);
                }
                set(result, i, j, res);
            } 
        }    
    return 0; 
}



/*
 * Store the result of raising mat to the (pow)th power to `result`.
 * Return 0 upon success.
 * Remember that pow is defined with matrix multiplication, not element-wise multiplication.
 * You may assume `mat` is a square matrix and `pow` is a non-negative integer.
 * Note that the matrix is in row-major order.
 */
int pow_matrix(matrix *result, matrix *mat, int pow) {
    // Task 1.6 TODO
    if (pow == 0) {
        for (int i = 0; i < mat->rows; i++) {
            for (int j = 0; j < mat->cols; j++) {
                if (i == j) {
                    set(result, i, j, 1);
                } else {
                    set(result, i, j, 0);
                }
            }
        }
        return 0;
    } 
    matrix* temp = NULL;
    allocate_matrix(&temp, mat->rows, mat->cols);
    if (pow % 2 == 0) {
        mul_matrix(temp, mat, mat);
        pow_matrix(result, temp, pow/2);
        
    } else {
        matrix* temp2 = NULL;
        allocate_matrix(&temp2, mat->rows, mat->cols);
        mul_matrix(temp, mat, mat);
        pow_matrix(temp2, temp, (pow-1)/2);
        mul_matrix(result, mat, temp2);
    }
    deallocate_matrix(temp);
    return 0;
}
