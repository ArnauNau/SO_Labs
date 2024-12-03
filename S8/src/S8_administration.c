// S8_administration.c
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
#define MSG_TERMINATE 5 //-> ctrl+c

#define STARTING_TIME 9
#define MAX_SLOTS 8
#define MAX_APPOINTMENTS_PER_SLOT 2

typedef struct {
    int timeslot;
    int appointments;
} SlotStats;

typedef struct {
    int timeslot;
} Appointment;

typedef struct {
    long id;
    int header;
    int num_of_slots;
    SlotStats slot[MAX_SLOTS];
} Msg;

void read_appointments_file (const int file, SlotStats appointments[MAX_APPOINTMENTS_PER_SLOT]) {
    Appointment buffer;
    while (read(file, &buffer, sizeof(Appointment)) == sizeof(Appointment)) {
        appointments[buffer.timeslot - STARTING_TIME].appointments++;
        //appointments[buffer.timeslot - STARTING_TIME] = buffer;
    }
}

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

void save_appointments(const SlotStats *const appointments) {
    const int appointments_file = open("appointments.dat", O_WRONLY);
    if (appointments_file < 0) {
        ess_print("Couldn't open file"); //TODO: DELETE?
    } else {
        if (lseek(appointments_file, 0, SEEK_SET) < 0) {
            ess_print_error("lseek");
            return;
        }
        for (int i = 0; i < MAX_SLOTS; i++) {
            for (int j = 0; j < appointments[i].appointments; j++) {
                Appointment buffer = {i + STARTING_TIME};
                if (write(appointments_file, &buffer, sizeof(Appointment)) != sizeof(Appointment)) {
                    ess_print_error("write");
                    return;
                }
            }
        }
    }
}

void handleCtrlC(const int signal) {
    if (signal == SIGINT) {
        send_message(-1, getpid(), MSG_TERMINATE, 0, NULL);
    }
}

int main(const int argc, char *argv[] __attribute__((unused)) ) {
    signal(SIGINT, handleCtrlC);

    if (argc > 1) {
        ess_print_error("Invalid number of arguments.");
        exit(EXIT_FAILURE);
    }

    SlotStats slots[MAX_SLOTS] = {
        {9, 0},
        {10, 0},
        {11, 0},
        {12, 0},
        {13, 0},
        {14, 0},
        {15, 0},
        {16, 0}
    };

    const int appointments_file = open("appointments.dat", O_RDONLY);
    if (appointments_file < 0) {
        ess_print("Couldn't open file"); //TODO: DELETE?
    }
    else {
        read_appointments_file(appointments_file, slots);
        close(appointments_file);
    }

    const key_t key = ftok("S8_administration.c", MAX_SLOTS);
    const int msqid = msgget(key, 0666 | IPC_CREAT);
    if (msqid < 0) {
        ess_print_error("msgget");
        exit(EXIT_FAILURE);
    }

    while (1) {
        const Msg message = receive_message(msqid);

        if (message.id == getpid()) {
            if (message.header == MSG_TERMINATE) {
                if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                    ess_print_error("msgctl");
                }

                ess_println("Exiting...");
                exit(EXIT_SUCCESS);
            }
        }

        switch (message.header) {
            case MSG_REQUEST_TIMES: {
                SlotStats available_slots[MAX_SLOTS];
                int num_of_available_slots = 0;
                for (int i = 0; i < MAX_SLOTS; i++) {
                    if (slots[i].appointments < MAX_APPOINTMENTS_PER_SLOT) {
                        available_slots[num_of_available_slots++] = slots[i];
                    }
                }
                send_message(msqid, message.id, MSG_REQUEST_TIMES, num_of_available_slots, available_slots);
                break;
            }
            case MSG_RESERVE: {
                const int slot = message.slot[0].timeslot - STARTING_TIME;
                if (slots[slot].appointments < MAX_APPOINTMENTS_PER_SLOT) {
                    slots[slot].appointments++;
                    send_message(msqid, message.id, MSG_CONFIRMED, 1, &slots[slot]);
                    save_appointments(slots);
                }
                else {
                    send_message(msqid, message.id, MSG_NOT_AVAILABLE, 0, NULL);
                }
                break;
            }
            default: {
                ess_println("Invalid message header.");
            }
        }
    }
}