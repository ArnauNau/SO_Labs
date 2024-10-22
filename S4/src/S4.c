// S4.c
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
#include <arpa/inet.h>

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

// 9700-9709
#define SERVER_PORT 9705

int main () {

    const int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        ess_print_error("socket TCP");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof (s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(9705);
    s_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind (socket_fd, (void *) &s_addr, sizeof (s_addr)) < 0) {
        ess_print_error("Unable to bind");
        exit(EXIT_FAILURE);
    }

    listen (socket_fd, 5);

    while (1) {
        struct sockaddr incoming_addr;
        socklen_t incoming_len = sizeof(incoming_addr);

        const int new_socket = accept(socket_fd, &incoming_addr, &incoming_len);
        if (new_socket < 0) {
            ess_print_error("error accepting");
            exit(EXIT_FAILURE);
        }
        printf("accepted connection.\n");

        ssize_t length;
        char test_string[21];
        memset(test_string, '\0', sizeof(char) * 21);

        do {
            length = read(new_socket, test_string, 20);
            printf("read %ld bytes.\n", length);
            ess_print("received: ");
            ess_println(test_string);
        } while (length > 0);
        close(new_socket);

    }
    close(socket_fd);

    exit(EXIT_SUCCESS);
}
