// S7.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz

// Assigned ports on montserrat.salle.url.edu : 9700-9709
#define _GNU_SOURCE

#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdbool.h>
#include <errno.h>

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

ssize_t ess_read_until(const int fd, char **const string_ptr, const char end) {
    ssize_t bytes_read = 0;
    ssize_t buffer_size = 8;
    char *string = NULL;

    char c = '\0';
    int i = 0;
    while (c != end) {
        const ssize_t size = read(fd, &c, sizeof(char));
        if (c == '\0') c = '-';
        //write(STDOUT_FILENO, &c, 1);
        if (size == 0) {
            if (i == 0) {
                free(string);
                return -1;
            }
            break;
        }
        if (size < 0) {
            free(string);
            return -1;
        }
        bytes_read += size;
        if(c != end){
            if (i + 1 >= buffer_size || string == NULL) {
                buffer_size = buffer_size * 2;
                char *temp = (char *) realloc(string, sizeof(char) * buffer_size);
                if (temp == NULL) {
                    free(string);
                    ess_print_error("realloc failed.");
                    return -1;
                }
                string = temp;
            }
            string[i++] = c;
        }
    }
    if (buffer_size > 0 && string != NULL) string[i] = '\0';
    *string_ptr = string;
    return bytes_read;
}
ssize_t ess_read_line(const int fd, char **const string_ptr) {
    return ess_read_until(fd, string_ptr, '\n');
}
/************************************ ESSLIB END **********************************************************************/

/** Checks whether an operation can be performed on an FD, and if not, if its because it's an invalid FD -> fd is not open */
#define FD_ISOPEN(x) fcntl(x, F_GETFD) != -1 || errno != EBADF

#define SEARCH_WORD 1
#define ADD_WORD 2
#define LIST_WORDS 3

typedef struct {
    char *word;
    char *definition;
} DictionaryEntry;

struct Dictionary {
    size_t num_entries;
    DictionaryEntry *entries;
};

struct User {
    int socket;
    char *username;
};

bool control_c_flag = false;

bool read_dictionary_file (const int fd, struct Dictionary *const dictionary) {
    char *buffer = NULL;
    ess_read_line(fd, &buffer);
    if (buffer == NULL) {
        return false;
    }

    errno = 0;
    dictionary->num_entries = strtol(buffer, NULL, 10);
    if (errno == EINVAL || errno == ERANGE) {
        free(buffer);
        return false;
    }
    free(buffer);

    dictionary->entries = malloc(sizeof(struct Dictionary) * dictionary->num_entries);

    for (unsigned int i = 0; i < dictionary->num_entries; i++) {
        ess_read_until(fd, &buffer, ':');
        dictionary->entries[i].word = strdup(buffer);
        free(buffer);

        ess_read_line(fd, &buffer);
        dictionary->entries[i].definition = strdup(buffer);
        free(buffer);
    }

    return true;
}

void write_dictionary_file (const int fd, const struct Dictionary dictionary) {
    char *buffer;
    asprintf(&buffer, "%lu\n", dictionary.num_entries);
    write(fd, buffer, strlen(buffer));
    free(buffer);

    for (unsigned int i = 0; i < dictionary.num_entries; i++) {
        asprintf(&buffer, "%s:%s\n", dictionary.entries[i].word, dictionary.entries[i].definition);
        write(fd, buffer, strlen(buffer));
        free(buffer);
    }
}

void free_dictionary(const struct Dictionary dictionary) {
    if (dictionary.entries == NULL) return;
    for (unsigned int i = 0; i < dictionary.num_entries; i++) {
        free(dictionary.entries[i].word);
        free(dictionary.entries[i].definition);
    }
    free(dictionary.entries);
}

void free_users (struct User *const users, const int num_users) {
    if (users == NULL) return;
    for (int i = 0; i < num_users; i++) {
        if (users[i].username) free(users[i].username);
        if (FD_ISOPEN(users[i].socket)) close(users[i].socket);
    }
    free(users);
}

int get_max_fd(const int socket_fd, const fd_set *const set) {
    int max_fd = socket_fd;
    for (int i = 0; i < FD_SETSIZE; i++) {
        if (FD_ISSET(i, set) && i > max_fd) {
            max_fd = i;
        }
    }
    return max_fd;
}

void send_response(const int client_sock, const char *const response) {
    write(client_sock, response, strlen(response));
}

void search_word (const struct Dictionary *const dictionary, const struct User *const user, const char *const request) {
    char *response;
    asprintf(&response, "User %s has requested search word command.", user->username);
    ess_println(response);
    free(response);
    const char *const word = request + 2;
    asprintf(&response, "Searching for word -> %s", word);
    ess_println(response);
    free(response);
    for (unsigned int i = 0; i < dictionary->num_entries; i++) {
        if (strcmp(dictionary->entries[i].word, word) == 0) {
            asprintf(&response, "D*%s*%s\n", dictionary->entries[i].word, dictionary->entries[i].definition);
            send_response(user->socket, response);
            free(response);
            return;
        }
    }
    asprintf(&response, "E* The word %s has not been found in the dictionary.\n", word);
    send_response(user->socket, response);
    free(response);
}

void add_word (struct Dictionary *const dictionary, const struct User *const user, const char *const request) {
    char *response;
    asprintf(&response, "User %s has requested add word command.", user->username);
    ess_println(response);
    free(response);
    //split word*definition\n from request into word and definition
    const char *const word = strtok((char *)request + 2, "*");
    const char *const definition = strtok(NULL, "\n");

    dictionary->num_entries++;
    dictionary->entries = realloc(dictionary->entries, sizeof(DictionaryEntry) * dictionary->num_entries);
    dictionary->entries[dictionary->num_entries - 1].word = strdup(word);
    dictionary->entries[dictionary->num_entries - 1].definition = strdup(definition);

    asprintf(&response, "OK* The word %s has been added to the dictionary.\n", word);
    send_response(user->socket, response);
    free(response);
}

void list_words (const struct Dictionary *const dictionary, const struct User *const user) {
    char *response;
    asprintf(&response, "User %s has requested list words command.", user->username);
    ess_println(response);
    free(response);
    asprintf(&response, "L*%lu*", dictionary->num_entries);
    for (unsigned int i = 0; i < dictionary->num_entries; i++) {
        char *temp;
        asprintf(&temp, "%s\n", dictionary->entries[i].word);
        char *const temp2 = realloc(response, strlen(response) + strlen(temp) + 1);
        if (temp2 == NULL) {
            free(response);
            free(temp);
            ess_print_error("Error on realloc.");
            return;
        }
        response = temp2;
        strcat(response, temp);
        free(temp);
    }
    send_response(user->socket, response);
    free(response);
}

void new_user (struct User *const user, const char *const request) {
    char *response;
    asprintf(&response, "New user connected: %s\n", request + 2);
    ess_println(response);
    free(response);
    user->username = strdup(request + 2);
}

void handle_client (struct Dictionary *const dictionary, struct User *const user, const char *const request) {
    if (request[0] == 'C') {
        search_word(dictionary, user, request);
    } else if (request[0] == 'A') {
        add_word(dictionary, user, request);
    } else if (request[0] == 'L') {
        list_words(dictionary, user);
    } else if (request[0] == 'U') {
        new_user(user, request);
    }
}

bool check_new_connections (const int socket_fd, fd_set *const active_fds, const fd_set *const read_fds, struct User **const users, int *const num_users) {
    if (FD_ISSET(socket_fd, read_fds)) {
        struct sockaddr_in c_addr;
        socklen_t c_len = sizeof(c_addr);
        const int new_fd = accept(socket_fd, (struct sockaddr *)&c_addr, &c_len);

        if (new_fd < 0) {
            ess_print_error("Couldn't accept connection.");
            return false;
        }

        FD_SET(new_fd, active_fds);
        ess_println("New connection accepted.");

        *users = realloc(*users, sizeof(struct User) * (*num_users + 1));
        (*users)[*num_users].socket = new_fd;
        (*users)[*num_users].username = NULL;
        (*num_users)++;
    }
    return true;
}

int get_user_by_socket (const struct User *const users, const int num_users, const int socket) {
    for (int i = 0; i < num_users; i++) {
        if (users[i].socket == socket) {
            return i;
        }
    }
    return -1;
}

void remove_user (struct User **const users, int *const num_users, const int user_index) {
    if (users[user_index]->username) free(users[user_index]->username);

    for (int i = user_index; i < *num_users - 1; i++) {
        (*users)[i] = (*users)[i + 1];
    }
    (*num_users)--;
    *users = realloc(*users, sizeof(struct User) * (*num_users));
}

void handleCtrlC(const int signal __attribute__((unused))) {
    control_c_flag = true;
}

int main (const int argc, char *argv[]) {
    signal(SIGINT, handleCtrlC);

    if (argc != 4) {
        ess_print_error("Number of args is not 2. It is necessary to provide the IP, port and filename.");
        ess_println("Usage: ./server <server_ip> <port> <dictionary_file>");
        exit(EXIT_FAILURE);
    }

    int dictionary_file = open(argv[3], O_RDONLY);
    if (dictionary_file < 0) {
        ess_print_error("Couldn't open dictionary file");
        exit(EXIT_FAILURE);
    }

    struct Dictionary dictionary;
    if (!read_dictionary_file(dictionary_file, &dictionary)) {
        ess_print_error("Couldn't read dictionary file.");
        exit(EXIT_FAILURE);
    }
    close(dictionary_file);

    ess_println("Dictionary server started.");

    const int socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd < 0) {
        free_dictionary(dictionary);
        ess_print_error("Couldn't create socket.");
        exit (EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    memset(&s_addr, 0, sizeof(s_addr));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(strtol(argv[2], NULL, 10)); //PORT
    s_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(socket_fd, (void *) &s_addr, sizeof (s_addr)) < 0){
        free_dictionary(dictionary);
        close(socket_fd);
        ess_print_error("Couldn't bind socket.");
        exit (EXIT_FAILURE);
    }
    ess_println("Opening connections...");

    if (listen(socket_fd, 10) < 0) {
        free_dictionary(dictionary);
        close(socket_fd);
        ess_print_error("Couldn't listen on socket.");
        exit(EXIT_FAILURE);
    }

    ess_println("Waiting for a connection...\n");

    fd_set active_fds;
    FD_ZERO(&active_fds);
    FD_SET(socket_fd, &active_fds);

    struct User *users = NULL;
    int num_users = 0;
    while (1) {
        fd_set read_fds = active_fds;  // Copy active_fds to read_fds
        const int max_fd = get_max_fd(socket_fd, &active_fds);

        errno = 0;
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) {
                if (control_c_flag) {
                    ess_println("Ctrl+C pressed. Exiting...");
                    break;
                } else {
                    ess_print_error("Select interrupted, continuing...");
                    continue;
                }
            }
            ess_print_error("Error in select.");
            break;
        }

        // Check for new connection requests
        if (!check_new_connections(socket_fd, &active_fds, &read_fds, &users, &num_users)) continue;

        // Check for data on existing connections
        for (int client_fd = 0; client_fd <= max_fd; client_fd++) {
            if (client_fd != socket_fd && FD_ISSET(client_fd, &read_fds)) {
                char *request = NULL;
                const ssize_t bytes_read = ess_read_line(client_fd, &request);
                const int current_user_index = get_user_by_socket(users, num_users, client_fd);
                if (bytes_read <= 0) {
                    if (current_user_index >= 0) {
                        char *buffer;
                        asprintf(&buffer, "New exit petition: %s has left the server.", users[current_user_index].username);
                        ess_println(buffer);
                        free(buffer);

                        remove_user(&users, &num_users, current_user_index);
                    }

                    close(client_fd);
                    FD_CLR(client_fd, &active_fds);
                } else {
                    request[bytes_read] = '\0';
                    handle_client(&dictionary, &users[current_user_index], request);
                    free(request);
                }
            }
        }
    }

    dictionary_file = open(argv[3], O_WRONLY);
    if (dictionary_file < 0) {
        ess_print_error("Couldn't open dictionary file");
    }
    else {
        write_dictionary_file(dictionary_file, dictionary);
        close(dictionary_file);
    }

    free_users(users, num_users);
    free_dictionary(dictionary);
    close(socket_fd);
    exit(EXIT_SUCCESS);
}
