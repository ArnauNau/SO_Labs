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

int read_dictionary_file (const int fd, struct Dictionary *const dictionary) {
    char *buffer = ess_read_line(fd);
    if (buffer == NULL) {
        return 0;
    }

    dictionary->num_entries = atoi(buffer);
    free(buffer);

    dictionary->entries = malloc(sizeof(struct Dictionary) * dictionary->num_entries);

    for (unsigned int i = 0; i < dictionary->num_entries; i++) {
        buffer = ess_read_until(fd, ':');
        dictionary->entries[i].word = strdup(buffer);
        free(buffer);

        buffer = ess_read_line(fd);
        dictionary->entries[i].definition = strdup(buffer);
        free(buffer);
    }

    return 1;
}

void free_dictionary(const struct Dictionary dictionary) {
    if (dictionary.entries == NULL) return;
    for (unsigned int i = 0; i < dictionary.num_entries; i++) {
        free(dictionary.entries[i].word);
        free(dictionary.entries[i].definition);
    }
    free(dictionary.entries);
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

void handle_client (struct Dictionary *const dictionary, struct User *user, char request[256]) {
    char *response;
    if (request[0] == 'C') {
        // search for a word
        asprintf(&response, "User %s has requested search word command.", user->username);
        ess_println(response);
        free(response);
        char *word = request + 2;
        word[strlen(word) - 1] = '\0';
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
    } else if (request[0] == 'A') {
        // add word to dictionary
        asprintf(&response, "User %s has requested add word command.", user->username);
        ess_println(response);
        free(response);
        //split word*definition\n from request into word and definition
        char *word = strtok(request + 2, "*");
        char *definition = strtok(NULL, "\n");

        dictionary->num_entries++;
        dictionary->entries = realloc(dictionary->entries, sizeof(DictionaryEntry) * dictionary->num_entries);
        dictionary->entries[dictionary->num_entries - 1].word = strdup(word);
        dictionary->entries[dictionary->num_entries - 1].definition = strdup(definition);

        asprintf(&response, "OK* The word %s has been added to the dictionary.\n", word);
        send_response(user->socket, response);
        free(response);

    } else if (request[0] == 'L') {
        // list all words
        asprintf(&response, "User %s has requested list words command.", user->username);
        ess_println(response);
        free(response);
        asprintf(&response, "L*%lu*", dictionary->num_entries);
        for (unsigned int i = 0; i < dictionary->num_entries; i++) {
            char *temp;
            asprintf(&temp, "%s\n", dictionary->entries[i].word);
            char *temp2 = realloc(response, strlen(response) + strlen(temp) + 1);
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
    } else if (request[0] == 'U') {
        // new user
        char *word = request + 2;
        word[strlen(word) - 1] = '\0';
        user->username = strdup(word);
        asprintf(&response, "New user connected: %s\n", word);
        ess_println(response);
        free(response);
        return;
    }
}

int main (const int argc, char *argv[]) {

    if (argc != 4) {
        ess_print_error("Number of args is not 2. It is necessary to provide the IP, port and filename.");
        ess_println("Usage: ./server <server_ip> <port> <dictionary_file>");
        exit(EXIT_FAILURE);
    }

    const int dictionary_file = open(argv[3], O_RDONLY);
    if (dictionary_file < 0) {
        ess_print_error("Couldn't open file");
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
    s_addr.sin_port = htons(atoi(argv[2])); //PORT
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

    fd_set active_fds, read_fds;
    FD_ZERO(&active_fds);
    FD_SET(socket_fd, &active_fds);

    struct User *users = NULL;
    int num_users = 0;

    while (1) {
        read_fds = active_fds;  // Copy active_fds to read_fds
        const int max_fd = get_max_fd(socket_fd, &active_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            ess_print_error("Error in select.");
            break;
        }

        // Check for new connection requests
        if (FD_ISSET(socket_fd, &read_fds)) {
            struct sockaddr_in c_addr;
            socklen_t c_len = sizeof(c_addr);
            const int new_fd = accept(socket_fd, (struct sockaddr *)&c_addr, &c_len);

            if (new_fd < 0) {
                ess_print_error("Couldn't accept connection.");
                continue;
            }

            FD_SET(new_fd, &active_fds);
            ess_println("New connection accepted.");

            users = realloc(users, sizeof(struct User) * (max_fd + 1));
            users[num_users].socket = new_fd;
            num_users++;
        }

        // Check for data on existing connections
        for (int client_fd = 0; client_fd <= max_fd; client_fd++) {
            if (client_fd != socket_fd && FD_ISSET(client_fd, &read_fds)) {
                char request[1024];
                const ssize_t bytes_read = read(client_fd, request, sizeof(request) - 1);

                struct User *temp = NULL;
                for (int i = 0; i < num_users; i++) {
                    if (users[i].socket == client_fd) {
                        temp = &users[i];
                    }
                }
                if (bytes_read <= 0) {
                    if (temp != NULL) {
                        char *buffer;
                        asprintf(&buffer, "New exit petition: %s has left the server.", temp->username);
                        ess_println(buffer);
                        if (temp->username) free(temp->username);
                        free(temp);
                    }

                    close(client_fd);
                    FD_CLR(client_fd, &active_fds);
                } else {
                    request[bytes_read] = '\0';

                    handle_client(&dictionary, temp, request);
                }
            }
        }
    }
    if (users != NULL) {
        for (int i = 0; i < num_users; i++) {
            close(users[i].socket);
            free(users[i].username);
        }
        free(users);
    }
    free_dictionary(dictionary);
    close(socket_fd);
    exit(EXIT_SUCCESS);
}
