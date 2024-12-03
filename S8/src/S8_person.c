// S8_person.c
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
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctype.h>

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

#define MSG_REQUEST_TIMES 1 //-> devuelve libres
#define MSG_RESERVE 2 //-> agendar cita, apuntas appointment en timeslot IF available
#define MSG_CONFIRMED 3 //-> si appointment hecho correctamente
#define MSG_NOT_AVAILABLE 4 //-> si no hay hueco

#define STARTING_TIME 9
#define MAX_SLOTS 8
#define MAX_APPOINTMENTS_PER_SLOT 2
#define DNI_LETTERS "TRWAGMYFPDXBNJZSQVHLCKE"

typedef struct {
    int timeslot;
    int appointments;
} SlotStats;

typedef struct {
    long id;
    int header;
    int num_of_slots;
    SlotStats slot[MAX_SLOTS];
} Msg;

int send_message (const int msqid, const long id, const int header, const int num_of_slots, const SlotStats *const slot) {
    if (msqid < 0) {
        ess_print_error("Invalid message queue ID.");
        return 0;
    }

    Msg message;
    message.id = id;
    message.header = header;
    message.num_of_slots = num_of_slots;
    for (int i = 0; i < num_of_slots; i++) {
        message.slot[i] = slot[i];
    }

    if (msgsnd(msqid, &message, sizeof(Msg) - sizeof(long), 0) < 0) {
        ess_print_error("msgsnd");
        return 0;
    }
    return 1;
}

Msg receive_message(const int msqid) {
    if (msqid < 0) {
        ess_print_error("Invalid message queue ID.");
        exit(EXIT_FAILURE);
    }
    Msg message;
    if (msgrcv(msqid, &message, sizeof(Msg) - sizeof(long), 0, 0) < 0) {
        ess_print_error("msgrcv");
        exit(EXIT_FAILURE);
    }
    return message;
}

int get_valid_dni() {
    static char dni[10];
    while (1) {
        ess_print("Enter your DNI: ");
        char* input = ess_read_line(STDIN_FILENO);
        if (input == NULL) {
            ess_print_error("Invalid input.");
            continue;
        }
        if (strlen(input) != 9) {
            ess_println("Invalid DNI length.");
            free(input);
            continue;
        }
        strcpy(dni, input);

        char digits[9];
        strncpy(digits, dni, 8);
        digits[8] = '\0';
        char letter = dni[8];

          int valid_digits = 1;
        for (int i = 0; i < 8; i++) {
            if (!isdigit(digits[i])) {
                ess_println("Invalid DNI digits.");
                valid_digits = 0;
                break;
            }
        }

        if (!valid_digits) {

            continue;
        }

        int number = atoi(digits);

        int index = number % 23;
        char expected_letter = DNI_LETTERS[index];

        if (toupper(letter) == expected_letter) {
            ess_println("Valid DNI.");
            return number;
        } else {
            ess_println("Invalid DNI letter.");
        }
    }
}


int main(const int argc, char *argv[] __attribute__((unused))) {

    if (argc > 1) {
        ess_print_error("Invalid number of arguments.");
        exit(EXIT_FAILURE);
    }

    int dni = get_valid_dni();


    const key_t key = ftok("S8_administration.c", MAX_SLOTS);
    const int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid < 0) {
        ess_print_error("msgget");
        exit(EXIT_FAILURE);
    }

    SlotStats slot[MAX_SLOTS];
    if (!send_message(msqid, dni, MSG_REQUEST_TIMES, 0, slot)) {
        ess_print_error("send_message");
        exit(EXIT_FAILURE);
    }

    while (1) {
        Msg message = receive_message(msqid);
        char *buffer;
        ess_println("Available time slots:");
        for (int i = 0; i < message.num_of_slots; i++) {
            asprintf(&buffer, "%d:00", message.slot[i].timeslot);
            ess_println(buffer);
            free(buffer);
        }

        ess_print("Enter the time slot you want to reserve: ");
        char *input = ess_read_line(STDIN_FILENO);
        if (input == NULL) {
            ess_print_error("Invalid input.");
            continue;
        }
        const int selected_slot = atoi(input) - STARTING_TIME;

        slot[0] = message.slot[selected_slot];
        if (!send_message(msqid, dni, MSG_RESERVE, 1, slot)) {
            ess_print_error("send_message");
            exit(EXIT_FAILURE);
        }

        switch (message.header) {
            case MSG_CONFIRMED: {
                if(message.num_of_slots != 1) {
                    ess_print_error("Invalid number of slots.");
                    exit(EXIT_FAILURE);
                }else{
                    char *buffer;
                    asprintf(&buffer, "Appointment confirmed for %d", message.slot[0].timeslot);
                    ess_println(buffer);
                    free(buffer);
                }
                break;
            }
            case MSG_NOT_AVAILABLE: {
                    ess_println("No available appointments.");
                    break;
            }
        }
    }

    return 0;
}