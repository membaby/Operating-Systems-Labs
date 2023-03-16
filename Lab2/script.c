#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

struct thread_data {
    int method;
    double **matrixA;
    double **matrixB;
    int rows_1;
    int cols_1;
    int cols_2;
    double **result;
};

void matrix_multiply(void *arg) {
    struct thread_data *t_data = (struct thread_data *) arg;
    clock_t start = clock();
    int i, j, k;
    if (t_data->method == 0){ // Thread per matrix
        for (i = 0; i < t_data->rows_1; i++) {
            t_data->result[i] = (double *)malloc(t_data->cols_2 * sizeof(double));
            for (j = 0; j < t_data->cols_2; j++) {
                t_data->result[i][j] = 0.0;
                for (k = 0; k < t_data->cols_1; k++) {
                    t_data->result[i][j] += t_data->matrixA[i][k] * t_data->matrixB[k][j];
                }
            }
        }
    } else if (t_data->method == 1){ // Thread per row
        pthread_t threads[t_data->rows_1];

        for (i=0; i<t_data->rows_1; i++){
            double **m = (double **)malloc(t_data->rows_1 * sizeof(double *));
            m[0] = (double *)malloc(t_data->cols_2 * sizeof(double));
            for (j=0; j<t_data->cols_1; j++){
                m[0][j] = t_data->matrixA[i][j];
            }

            struct thread_data t2_data;
            t2_data.method = 0;
            t2_data.matrixA = t_data->matrixA;
            t2_data.matrixB = t_data->matrixB;
            t2_data.rows_1 = t_data->rows_1;
            t2_data.cols_1 = t_data->cols_1;
            t2_data.cols_2 = t_data->cols_2;
            t2_data.result = t_data->result;

            int rc = pthread_create(&threads[i], NULL, (void *(*)(void *))matrix_multiply, &t2_data);
            if (rc) {
                printf("Error creating thread %d\n", i);
                exit(-1);
            }
        }

        for (i=0; i<t_data->rows_1; i++){
            pthread_join(threads[i], NULL);
        }

    } else if (t_data->method == 2){ // Thread per element
        pthread_t threads[t_data->rows_1][t_data->cols_2];

        for (i=0; i<t_data->rows_1; i++){
            for (j=0; j<t_data->cols_2; j++){
                double **m = (double **)malloc(t_data->rows_1 * sizeof(double *));
                m[0] = (double *)malloc(t_data->cols_2 * sizeof(double));
                m[0][0] = t_data->matrixA[i][j];

                struct thread_data t2_data;
                t2_data.method = 0;
                t2_data.matrixA = t_data->matrixA;
                t2_data.matrixB = t_data->matrixB;
                t2_data.rows_1 = t_data->rows_1;
                t2_data.cols_1 = t_data->cols_1;
                t2_data.cols_2 = t_data->cols_2;
                t2_data.result = t_data->result;

                int rc = pthread_create(&threads[i][j], NULL, (void *(*)(void *))matrix_multiply, &t2_data);
                if (rc) {
                    printf("Error creating thread %d", i);
                    exit(-1);
                }
            }
        }
    }
    clock_t end = clock();
    if (pthread_self() == 1){
        double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
        printf("Method %d: Elapsed time: %f seconds\n", t_data->method, elapsed_time);
    }
    return;
}

void write_matrix_to_file(double **matrix, char* method, int rows, int cols, char* filename){
    FILE *fp = fopen(filename, "w");
    fprintf(fp, "row=%d col=%d\n", rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            fprintf(fp, "%.f ", matrix[i][j]);
        }
        fprintf(fp, "\n");
    }
}

int main(int argc, char *argv[]) {
    // Default input and output file names
    char *input_file1 = "a.txt";
    char *input_file2 = "b.txt";
    char *output_file = "c";

    // Check if the user has provided input and output file names
    if (argc >= 3) {
        input_file1 = argv[1];
        input_file2 = argv[2];
    }
    if (argc >= 4) {
        output_file = argv[3];
    }


    FILE *f1 = fopen(input_file1, "r");
    FILE *f2 = fopen(input_file2, "r");
    if (!f1 || !f2) {
        printf("One or more files could not be opened. Exiting.\n");
        return 1;
    }
    
    int rows_1 = 0; // Number of rows in file 1
    int cols_1 = 0; // Number of columns in file 1
    int rows_2 = 0; // Number of rows in file 2
    int cols_2 = 0; // Number of rows in file 2

    double **matrixA, **matrixB;

    // Read the first file
    fscanf(f1, "row=%d col=%d", &rows_1, &cols_1);
    matrixA = (double **) malloc(rows_1 * sizeof(double *));
    for (int i = 0; i < rows_1; i++) {
        matrixA[i] = (double *) malloc(cols_1 * sizeof(double));
    }
    // Read the second file
    fscanf(f2, "row=%d col=%d", &rows_2, &cols_2);
    matrixB = (double **) malloc(rows_2 * sizeof(double *));
    for (int i = 0; i < rows_2; i++) {
        matrixB[i] = (double *) malloc(cols_2 * sizeof(double));
    }
    double **result = (double **) malloc(rows_1 * sizeof(double *));
    for (int i = 0; i < cols_2; i++) {
        matrixB[i] = (double *) malloc(cols_2 * sizeof(double));
    }

    // Read Matrices
    for (int i = 0; i < rows_1; i++) {
        for (int j = 0; j < cols_1; j++) {
            fscanf(f1, "%lf", &matrixA[i][j]);
        }
    }
    for (int i = 0; i < rows_2; i++) {
        for (int j = 0; j < cols_2; j++) {
            fscanf(f2, "%lf", &matrixB[i][j]);
        }
    }
    
    char *extension;
    for (int method = 0; method < 3; method++){
        if (method == 0) extension = "_per_matrix.txt";
        else if (method == 1) extension = "_per_row.txt";
        else if (method == 2) extension = "_per_element.txt";

        struct thread_data t_data;
        t_data.method = method;
        t_data.matrixA = matrixA;
        t_data.matrixB = matrixB;
        t_data.rows_1 = rows_1;
        t_data.cols_1 = cols_1;
        t_data.cols_2 = cols_2;
        t_data.result = result;
        

        matrix_multiply(&t_data);
        int output_size = strlen(output_file) + strlen(extension) + 1;
        char *output_with_extension = malloc(output_size);
        strcpy(output_with_extension, output_file);
        strcat(output_with_extension, extension);
        write_matrix_to_file(result, "A thread per matrix", cols_1, rows_2, output_with_extension);
    }
    fclose(f1);
    fclose(f2);
}