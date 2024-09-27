#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#define USERS_FILE "include/users.instagram"

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
    size_t buffer_size = 8;
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
        if (size <= 0) {
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

int read_username_string (const char * string, char ** username, const int starting_index) {
    size_t username_size = 9;
    *username = (char *) malloc(sizeof(char) * username_size);
    if (*username == NULL) {
        ess_print_error("malloc failed.");
        return -1;
    }

    int i = starting_index;
    int j = 0;
    while (string[i] != '#' && string[i] !='\0') {
        if (j + 1 >= username_size) {
            username_size = username_size + 8;
            char *temp = realloc(*username, username_size);
            if (temp == NULL) {
                free(*username);
                ess_print_error("realloc failed.");
                return -1;
            }
            *username = temp;
        }
        //write(STDOUT_FILENO, string+i, 1);
        (*username)[j++] = string[i++];
    }
    (*username)[j] = '\0';
    //ess_print("\n");
    return i;
}

int read_user_info(const int fd) {
    char *line_buffer = ess_read_until(fd, '&');
    if (line_buffer == NULL) {
        return -1;
    }
    ess_println(line_buffer);
    int line_index = 0;
    char *word_buffer = NULL;
    line_index = read_username_string(line_buffer, &word_buffer, line_index);
    if (line_index == -1) {
        free(line_buffer);
        return -1;
    }
    ess_println("pre:");
    ess_println(word_buffer);
    ess_println("post:");

    char trash[4];
    read(fd, trash, 4);

    //read followers
    do {
        free(word_buffer);
        line_index = read_username_string(line_buffer, &word_buffer, line_index);
        if (line_index == -1) {
            ess_println("line_index=-1");
            break;
        }
        ess_println(word_buffer);
        if (word_buffer[0] == '&') {
            ess_println("'&' found");
            break;
        }
    } while (0); //FIXME
    //all followers have been read from this user

    free(word_buffer);
    free(line_buffer);
    return 0;
}

int main(void)
{
    ess_println("Hello, World!\n");

    //reading inclue/users.instagram file:
    //format:
    //username#[4 bytes]follower_username#follower2_username#&
    const int users_fd = open(USERS_FILE, O_RDONLY);
    if (users_fd < 0) {
        ess_print_error("Failed to open users file.");
        return 1;
    }

    if (read_user_info(users_fd) != 0) {
        ess_print_error("Problem reading user info.");
    }

    close(users_fd);
    ess_println("\nBye!\n");

    return 0;
}
