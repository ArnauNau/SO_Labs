// S5.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz
#define _GNU_SOURCE
//#define _XOPEN_SOURCE 500

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
#include <netinet/in.h>
#include <sys/socket.h>

#define TTY_COLOR_DEFAULT "\033[m"
#define TTY_COLOR_MAGENTA "\033[35m"
#define TTY_COLOR_BLUE "\033[34m"
#define TTY_COLOR_YELLOW "\033[33m"
#define TTY_COLOR_GREEN "\033[32m"
#define TTY_COLOR_RED "\033[31m"

struct Challenge {
    char *question;
    char *answer;
    char *hint;
};

struct ChallengeList {
    struct Challenge *challenges;
    int challenge_count;
    int socket;
};

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
        if (c == '\0') return NULL;
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
#define REQUEST_CHALLENGE 1
#define SEND_ANSWER 2
#define REQUEST_HINT 3
#define CURRENT_STATUS 4
#define TERMINATE 5


int sockfd; //so I can close the socket on ctrl+C <3

void receive_message(const int sock, char **buffer) {
    *buffer = ess_read_line(sock);
    if (*buffer == NULL) {
        close(sock);
        exit(-1);
    }
}

void send_message(const int sock, const char *const string) {
    char *buff = strdup(string);
    buff[strlen(string)] = '\n';
    buff[strlen(string)+1] = '\0';
    write(sock, buff, strlen(buff));
    free(buff);
}

void ignore_sigint() {
    ess_println(TTY_COLOR_RED "Exiting server..." TTY_COLOR_DEFAULT);
}

//Question1|Answer1&Hint\n
void read_challenges_file (const int fd, struct Challenge **challenges, int *challenges_count) {
    *challenges_count = 0;
    *challenges = NULL;

    while (1) {
        char *buffer = ess_read_until(fd, '|');
        if (buffer == NULL) {
            break;
        }

        struct Challenge temp_challenge;
        temp_challenge.question = strdup(buffer);
        free(buffer);

        buffer = ess_read_until(fd, '&');
        temp_challenge.answer = strdup(buffer);
        free(buffer);

        buffer = ess_read_until(fd, '\n');
        temp_challenge.hint = strdup(buffer);
        free(buffer);

        struct Challenge *temp_list = realloc(*challenges, sizeof(struct Challenge) * (*challenges_count + 1));
        if (NULL == temp_list) {
            for (int i = 0; i < *challenges_count; i++) {
                free((*challenges)[i].question);
                free((*challenges)[i].answer);
                free((*challenges)[i].hint);
            }
            free(*challenges);
            free(temp_challenge.question);
            free(temp_challenge.answer);
            free(temp_challenge.hint);
            ess_print_error("Realloc failed");
            exit(EXIT_FAILURE);
        }
        *challenges = temp_list;
        (*challenges)[*challenges_count] = temp_challenge;
        (*challenges_count)++;
    }
}

void * handle_connection (void * cha_list) {
    struct ChallengeList *list = (struct ChallengeList *) cha_list;
    const int sockfd = list->socket;

    char *buffer;
    receive_message(sockfd, &buffer);
    char *username = strdup(buffer);
    free(buffer);

    asprintf(&buffer, "Welcome %s!", username);
    ess_println(buffer);
    free(buffer);

    int current_challenge = 0;
    while (1) {
        receive_message(sockfd, &buffer);
        const int option = atoi(buffer);
        free(buffer);
        switch (option) {
            case REQUEST_CHALLENGE:
                //send challenge
                    asprintf(&buffer, TTY_COLOR_BLUE "%s - request challenge..." TTY_COLOR_DEFAULT, username);
                    ess_println(buffer);
                    free(buffer);
                    send_message(sockfd, list->challenges[current_challenge].question);
                    break;
            case SEND_ANSWER:
                //receive solution
                    asprintf(&buffer, TTY_COLOR_BLUE "%s - sending answer...", username);
                    ess_println(buffer);
                    free(buffer);
                    ess_println(TTY_COLOR_YELLOW "Checking answer..." TTY_COLOR_DEFAULT);
                    receive_message(sockfd, &buffer);
                    if(strcmp(buffer, list->challenges[current_challenge].answer) == 0){
                        free(buffer);
                        //correct answer
                        current_challenge++;
                        if (current_challenge == list->challenge_count) {
                            buffer = TTY_COLOR_GREEN "Congratulations! You've completed all challenges. Press 4 to get the treasure coordinates." TTY_COLOR_DEFAULT;
                        } else {
                            buffer = TTY_COLOR_GREEN "Correct answer! Proceeding to next challenge." TTY_COLOR_DEFAULT;
                        }
                    }else{
                        free(buffer);
                        //wrong answer
                        buffer = "Incorrect answer!";
                    }
                    send_message(sockfd, buffer);
                    break;
            case REQUEST_HINT:
                //request hint
                    asprintf(&buffer, TTY_COLOR_BLUE "%s - Request hint..." TTY_COLOR_DEFAULT, username);
                    ess_println(buffer);
                    free(buffer);
                    send_message(sockfd, list->challenges[current_challenge].hint);
                    ess_println(TTY_COLOR_YELLOW "Hint sent!" TTY_COLOR_DEFAULT);
                    break;
            case CURRENT_STATUS:
                //current status
                    asprintf(&buffer, TTY_COLOR_BLUE "%s - Request to view current mission status" TTY_COLOR_DEFAULT, username);
                    ess_println(buffer);
                    free(buffer);
                    if (current_challenge == list->challenge_count) {
                        buffer = TTY_COLOR_GREEN "Congratulations! You've found the treasure at coordinates: X:100, Y:200. Disconnecting." TTY_COLOR_DEFAULT;
                        send_message(sockfd, buffer);
                        free(username);
                        close(sockfd);
                        exit(EXIT_SUCCESS);
                    } else {
                        asprintf(&buffer, TTY_COLOR_MAGENTA "You're currently on challenge %d out of %d." TTY_COLOR_DEFAULT, current_challenge, list->challenge_count);
                        send_message(sockfd, buffer);
                        free(buffer);
                    }
                    break;
            case TERMINATE:
                    asprintf(&buffer, TTY_COLOR_BLUE "%s - Decided to terminate the connection" TTY_COLOR_DEFAULT, username);
                    ess_println(buffer);
                    free(username);
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

int main (const int argc, char *argv[]) {
    signal(SIGINT, ignore_sigint);

    if (argc != 3) {
        ess_print_error("Wrong number of arguments.");
        ess_println("Usage: ./server <server_ip> <port>");
        exit(EXIT_FAILURE);
    }

    const int challenges_file = open("challenges.txt", O_RDONLY);
    if (challenges_file < 0) {
        ess_print_error("Couldn't open file");
        exit(EXIT_FAILURE);
    }

    struct ChallengeList list;
    list.challenge_count = 0;
    list.challenges = NULL;
    read_challenges_file(challenges_file, &(list.challenges), &(list.challenge_count));
    close(challenges_file);

    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        ess_print_error("socket");
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(atoi(argv[2])); //PORT
    s_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(sockfd, (void *) &s_addr, sizeof (s_addr)) < 0){
        for (int i = 0; i < list.challenge_count; i++) {
            free(list.challenges[i].question);
            free(list.challenges[i].answer);
            free(list.challenges[i].hint);
        }
        free(list.challenges);
        close(sockfd);
        ess_print_error("Couldn't bind socket.");
        exit (EXIT_FAILURE);
    }

    listen(sockfd, 5);

    const char *buffer = "Welcome to the Guardian of Enigma Server. Prepare to embark on a journey of puzzles and mysteries!";
    ess_println(buffer);

    while (1) {
        struct sockaddr_in c_addr;
        socklen_t c_len = sizeof(c_addr);

        const int newsock = accept(sockfd, (void *) &c_addr, &c_len);
        if(newsock < 0) {
            ess_print_error("Couldn't accept socket.");
            for (int i = 0; i < list.challenge_count; i++) {
                free(list.challenges[i].question);
                free(list.challenges[i].answer);
                free(list.challenges[i].hint);
            }
            free(list.challenges);
            close(sockfd);
            exit(EXIT_FAILURE);
        }
        list.socket = newsock;
        pthread_t connection_thread;
        pthread_create(&connection_thread, NULL, handle_connection, &list);
        void * ignore;
        pthread_join(connection_thread, (void **) &ignore);
        break;
    }
    //ess_println( TTY_COLOR_MAGENTA "Welcome to RiddleQuest. Prepare to unlock the secrets and discover the treasure!" TTY_COLOR_DEFAULT);

    for (int i = 0; i < list.challenge_count; i++) {
        free(list.challenges[i].question);
        free(list.challenges[i].answer);
        free(list.challenges[i].hint);
    }
    free(list.challenges);
    close(sockfd);
    exit(EXIT_SUCCESS);
}
