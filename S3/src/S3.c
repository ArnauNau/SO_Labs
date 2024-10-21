// S3.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void ess_print(const char * const string) {
    write(STDOUT_FILENO, string, strlen(string));
}
void ess_println(const char * const string) {
    ess_print(string);
    ess_print("\n");
}
void ess_print_error(const char * const string) {
    ess_print("\033[0;91mError: ");
    ess_println(string);
    ess_print("\033[0m");
}

char * ess_read_until(const int fd, const char end) {
    ssize_t buffer_size = 8;
    char *string = (char *) malloc(sizeof(char) * buffer_size);
    if (string == NULL) {
        ess_print_error("malloc failed.");
        return NULL;
    }

    char c = '\0';
    int i = 0;
    while (c != end) {
        const ssize_t size = read(fd, &c, sizeof(char));
        if (c == '\0') c = '-';
        //write(STDOUT_FILENO, &c, 1);
        if (size == 0) {
            if (i == 0) {
                free(string);
                return NULL;
            }
            break;
        }
        if (size < 0) {
            free(string);
            return NULL;
        }
        if(c != end){
            if (i + 1 >= buffer_size) {
                buffer_size = buffer_size * 2;
                char *temp = (char *) realloc(string, sizeof(char) * buffer_size);
                if (temp == NULL) {
                    free(string);
                    ess_print_error("realloc failed.");
                    return NULL;
                }
                string = temp;
            }
            string[i++] = c;
        }
    }
    string[i] = '\0';
    return string;
}
char * ess_read_line(const int fd) {
    return ess_read_until(fd, '\n');
}

struct DataBox {
    double * data;
    size_t size;
};

struct VarianceData {
    struct DataBox data_box;
    double mean;
};

void read_data (const int file, double **data, size_t *data_size) {

    while (1) {
        char *in = ess_read_line(file);
        if (NULL == in) {
            break;
        }

        const double temp = atof(in);

        (*data_size)++;

        double * temp_ptr = realloc(*data, sizeof(double) * *data_size);
        if (NULL == temp_ptr) {
            ess_print_error("Unable to realloc");
            if (NULL != *data) free(*data);
            exit(-1);
        }
        *data = temp_ptr;

        (*data)[(*data_size)-1] = temp;

        free(in);
    }
}

void * calc_mean (void * _data_box) {
    const struct DataBox data_box = *(struct DataBox *) _data_box;

    double *mean = malloc(sizeof(double));
    *mean = 0;
    for (size_t i = 0; i < data_box.size; i++) {
        *mean += data_box.data[i];
    }

    *mean = *mean / (double)data_box.size;


    pthread_exit((void **) mean);
}

void * calc_median (void * _data_box) {
    const struct DataBox data_box = *(struct DataBox *) _data_box;

    for (size_t i = 0; i < data_box.size; i++) {
        for (size_t j = i+1; j < data_box.size; j++) {
            if (data_box.data[i] < data_box.data[j]) {
                const double temp = data_box.data[i];
                data_box.data[i] = data_box.data[j];
                data_box.data[j] = temp;
            }
        }
    }
    double * median = malloc(sizeof(double));
    *median = data_box.data[data_box.size / 2];

    pthread_exit((void **) median);
}

void * calc_max (void * _data_box) {
    const struct DataBox data_box = *(struct DataBox *) _data_box;

    double *max = malloc(sizeof(double));
    *max = 0;
    for (size_t i = 0; i < data_box.size; i++) {
        if (data_box.data[i] > *max) {
            *max = data_box.data[i];
        }
    }

    pthread_exit((void **) max);
}

void * calc_min (void * _data_box) {
    const struct DataBox data_box = *(struct DataBox *) _data_box;

    double *min = malloc(sizeof(double));
    *min = data_box.data[0];
    for (size_t i = 0; i < data_box.size; i++) {
        if (data_box.data[i] < *min) {
            *min = data_box.data[i];
        }
    }

    pthread_exit((void **) min);
}

void * calc_variance (void * _data) {
    const struct VarianceData data = *(struct VarianceData *) _data;

    double *variance = malloc(sizeof(double));
    *variance = 0;
    double sum = 0;
    for (size_t i = 0; i < data.data_box.size; i++) {
        const double point_minus_mean = data.data_box.data[i] - data.mean;
        sum += (point_minus_mean * point_minus_mean);
    }

    *variance = ((double)sum / (double)data.data_box.size);

    pthread_exit((void **) variance);
}

int main (const int argc, char *argv[]) {
    signal(SIGINT, SIG_IGN);

    if (argc != 2) {
        ess_print_error("Incorrect number of arguments");
        ess_println("Please provide the input file name");
        ess_println("Usage ./S3 <data_file>");
        exit(-1);
    }

    const char *filename = argv[1];

    const int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        char *buffer;
        asprintf(&buffer, "Unable to open the file '%s'", filename);
        ess_print_error(buffer);
        free(buffer);
        exit(-1);
    }

    double *data = NULL;
    size_t data_size = 0;
    read_data(fd, &data, &data_size);
    close(fd);

    struct DataBox data_box = {data, data_size};

    pthread_t thread_mean;
    pthread_t thread_median;
    pthread_t thread_max;
    pthread_t thread_min;
    pthread_t thread_variance;

    pthread_create(&thread_mean, NULL, calc_mean, &data_box);
    pthread_create(&thread_median, NULL, calc_median, &data_box);
    pthread_create(&thread_max, NULL, calc_max, &data_box);
    pthread_create(&thread_min, NULL, calc_min, &data_box);

    double *mean;
    pthread_join(thread_mean, (void **) &mean);

    struct VarianceData variance_data = {data_box, *mean};

    pthread_create(&thread_variance, NULL, calc_variance, &variance_data);

    double *median;
    pthread_join(thread_median, (void **) &median);
    double *max;
    pthread_join(thread_max, (void **) &max);
    double *min;
    pthread_join(thread_min, (void **) &min);
    double *variance;
    pthread_join(thread_variance, (void **) &variance);

    char *buffer;
    asprintf(&buffer, "Mean: %f", *mean);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "Median: %f", *median);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "Maximum value: %f", *max);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "Minimum value: %f", *min);
    ess_println(buffer);
    free(buffer);
    asprintf(&buffer, "Variance: %f", *variance);
    ess_println(buffer);
    free(buffer);

    if (mean != NULL) free(mean);
    if (median != NULL) free(median);
    if (max != NULL) free(max);
    if (min != NULL) free(min);
    if (variance != NULL) free(variance);
    if (data != NULL) free(data);

    exit(0);
}
