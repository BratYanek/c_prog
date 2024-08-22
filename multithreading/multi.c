#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

#define ARR_LEN 100

typedef struct {
    int *x, *y, *xcorr_arr;
    int size_x, size_y, size_xcorr_arr;
} xcorr_args;

void *arr_fill(void *arg);
void *xcorr(void *args);

int main(int argc, char *argv[])
{
    FILE *fp = NULL;
    FILE *gnupipe = NULL;

    pthread_t thread1;
    pthread_t thread2;

    int value1[ARR_LEN];
    int value2[ARR_LEN];

    srand(time(NULL));

    // Create two threads to fill arrays with random data
    pthread_create(&thread1, NULL, arr_fill, (void*)value1);
    pthread_create(&thread2, NULL, arr_fill, (void*)value2);

    // Wait for threads to finish
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    xcorr_args args;
    args.x = value1;
    args.y = value2;
    args.size_x = ARR_LEN;
    args.size_y = ARR_LEN;
    args.size_xcorr_arr = 2 * ARR_LEN - 1;  
    args.xcorr_arr = malloc(sizeof(int) * args.size_xcorr_arr);

    for (int i = 0; i < args.size_xcorr_arr; i++) {
        args.xcorr_arr[i] = 0;
    }

    xcorr((void*)&args);
    
    fp = fopen("data.tmp", "w");
    if (fp == NULL) {
        perror("Error opening data.tmp");
        free(args.xcorr_arr);
        return 1;
    }

    for (int i = 0; i < args.size_xcorr_arr; i++) {
        int shift = i - (args.size_xcorr_arr / 2); 
        fprintf(fp, "%d %d\n", shift, args.xcorr_arr[i]);
    }
    fclose(fp);

    gnupipe = popen("gnuplot -persistent", "w");
    if (gnupipe == NULL) {
        perror("Error opening pipe to gnuplot");
        free(args.xcorr_arr);
        return 1;
    }

    fprintf(gnupipe, "set title 'Cross-Correlation'\n");
    fprintf(gnupipe, "set xlabel 'Shift (k)'\n");
    fprintf(gnupipe, "set ylabel 'Cross-Correlation Value'\n");
    fprintf(gnupipe, "plot 'data.tmp' using 1:2 with lines title 'Cross-Correlation'\n");

    pclose(gnupipe);

    free(args.xcorr_arr);

    return 0;
}

void *arr_fill(void *arg)
{
    int *array = (int *)arg;
    
    for (int i = 0; i < ARR_LEN; i++) {
        array[i] = rand() % 10;
    }

    return NULL;
}

void *xcorr(void *args)
{
    xcorr_args *arguments = (xcorr_args *)args;

    int size_x = arguments->size_x;
    int size_y = arguments->size_y;
    int size_xcorr = arguments->size_xcorr_arr;
    int max_arg = 0;
    int max_idx = 0;

    for (int k = -size_y + 1; k < size_x; k++) { 
        int sum = 0;
        for (int i = 0; i < size_x; i++) {
            if (i - k >= 0 && i - k < size_y) {
                sum += arguments->x[i] * arguments->y[i - k];
            }
        }

        if (k + size_y - 1 >= 0 && k + size_y - 1 < size_xcorr) {
            arguments->xcorr_arr[k + size_y - 1] = sum;

            if (sum > max_arg) {
                max_arg = sum;
                max_idx = k + size_y - 1;
            }
        }
    }

    printf("Max xcorr arg = %d at shift: %d\n", max_arg, max_idx - (size_xcorr / 2));
    return NULL;
}
