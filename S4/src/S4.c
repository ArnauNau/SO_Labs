// S4.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz

#define _GNU_SOURCE

#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

/************************************ ESSLIB START ********************************************************************/
#define TTY_COLOR_DEFAULT "\033[m"
#define TTY_COLOR_MAGENTA "\033[35m"
#define TTY_COLOR_BLUE "\033[34m"
#define TTY_COLOR_YELLOW "\033[33m"
#define TTY_COLOR_GREEN "\033[32m"
#define TTY_COLOR_RED "\033[31m"

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
/************************************ ESSLIB END ********************************************************************/

// Assigned ports on montserrat.salle.url.edu : 9700-9709
#define REQUEST_CHALLENGE 1
#define SEND_ANSWER 2
#define REQUEST_HINT 3
#define CURRENT_STATUS 4
#define TERMINATE 5

void print_menu() {
    ess_println("====================================");
    ess_println("||\t\tNavigation Menu\t\t||");
    ess_println("====================================");
    ess_println("1. Receive Current Challenge");
    ess_println("2. Send Response to Challenge");
    ess_println("3. Request Hint");
    ess_println("4. View Current Mission");
    ess_println("5. Terminate Connection and Exit");
    ess_print("Select an option: ");
}

void receive_message(const int sock, char **buffer) {
    *buffer = ess_read_line(sock);
    if (*buffer == NULL) {
        close(sock);
        exit(-1);
    }
    ess_println(*buffer);
}

void send_message(const int sock, const char *const string) {
    char *buff = malloc(sizeof(char) * strlen(string)+2);
    strcpy(buff, string);
    buff[strlen(string)] = '\n';
    buff[strlen(string)+1] = '\0';
    write(sock, buff, strlen(buff));
    free(buff);
}

void ignore_sigint() {
    ess_println("Ignoring ctrl+C.");
}

int main (const int argc, char *argv[]) {
    signal(SIGINT, ignore_sigint);

    if (argc != 3) {
        ess_print_error("Wrong number of arguments.");
        ess_println("Usage: ./client <server_ip> <port>");
        exit(EXIT_FAILURE);
    }

    struct in_addr ip_addr;
    inet_aton(argv[1], &ip_addr);
    int sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        ess_print_error("socket");
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(atoi(argv[2]));
    s_addr.sin_addr = ip_addr;
    if(connect(sockfd, (void *) &s_addr, sizeof (s_addr)) < 0) {
        ess_print_error("connect");
        close(sockfd);
        exit (EXIT_FAILURE);
    }
    ess_println( TTY_COLOR_MAGENTA "Welcome to RiddleQuest. Prepare to unlock the secrets and discover the treasure!" TTY_COLOR_DEFAULT);

    ess_print("Please enter your username: ");
    char *buffer = ess_read_line(STDIN_FILENO);
    send_message(sockfd, buffer);
    free(buffer);

    while (1) {
        print_menu();
        buffer = ess_read_line(STDIN_FILENO);
        const int option = atoi(buffer);

        send_message(sockfd, buffer);
        free(buffer);

        switch (option) {
            case REQUEST_CHALLENGE:
                //request challenge
                    ess_print(TTY_COLOR_MAGENTA "Challenge: " TTY_COLOR_DEFAULT);
                    receive_message(sockfd, &buffer);
                    free(buffer);
                    ess_println("");
                    break;
            case SEND_ANSWER:
                //send solution
                    ess_print(TTY_COLOR_MAGENTA "Enter your response to the challenge: " TTY_COLOR_DEFAULT);
                    buffer = ess_read_line(STDIN_FILENO);
                    send_message(sockfd, buffer);
                    free(buffer);
                    receive_message(sockfd, &buffer);
                    free(buffer);
                    break;
            case REQUEST_HINT:
                //request hint
                    ess_println(TTY_COLOR_MAGENTA "Your hint is coming..." TTY_COLOR_DEFAULT);
                    receive_message(sockfd, &buffer);
                    free(buffer);
                    ess_println("");
                    break;
            case CURRENT_STATUS:
                //current status
                    ess_println(TTY_COLOR_MAGENTA "Current mission status:" TTY_COLOR_DEFAULT);
                    receive_message(sockfd, &buffer);
                    free(buffer);
                    ess_println("");
                    break;
            case TERMINATE:
                    ess_println(TTY_COLOR_YELLOW "Terminating connection..." TTY_COLOR_DEFAULT);
                    close(sockfd);
                    exit(EXIT_SUCCESS);
                    break;
            default:
                    receive_message(sockfd, &buffer);
                    free(buffer);
                    ess_println("");
                break;
        }
    }

    close(sockfd);
    exit(EXIT_SUCCESS);
}
