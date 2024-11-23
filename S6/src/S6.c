// S6.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz

#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <time.h>

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

#define NUM_OF_STATIONS 3
#define NUM_OF_TRAINS 10

#define ARRIVAL_REPORT 0x01
#define DEPARTURE_REPORT 0x02
#define TERMINATE_EVENT 0x04

void manageCentralNode (const int station_pipes[NUM_OF_STATIONS][2], volatile int *const num_of_passengers) {
    fd_set pipe_set;
    int max_fd = 0;

    for (int i = 0; i < NUM_OF_STATIONS; i++) {
        if (station_pipes[i][0] > max_fd) {
            max_fd = station_pipes[i][0];
        }
    }

    int terminated_stations = 0;
    while (terminated_stations < NUM_OF_STATIONS) {
        FD_ZERO(&pipe_set);
        for (int i = 0; i < NUM_OF_STATIONS; i++) {
            FD_SET(station_pipes[i][0], &pipe_set);
        }

        const int active_pipe = select(max_fd + 1, &pipe_set, NULL, NULL, NULL);
        if (active_pipe < 0) {
            ess_print_error("Error checking pipe activity (select).");
            exit(EXIT_FAILURE);
        }

        for (int station = 0; station < NUM_OF_STATIONS; station++) {
            if (FD_ISSET(station_pipes[station][0], &pipe_set)) {
                int report = 0x00;
                if (read(station_pipes[station][0], &report, sizeof(int)) > 0) {
                    if (report & ARRIVAL_REPORT) {
                        char *buffer;
                        asprintf(&buffer, "[Control Center] Station %d - Train arrival. Station %d now has %d passengers.", station, station, num_of_passengers[station]);
                        ess_println(buffer);
                        free(buffer);
                    }
                    else if (report & DEPARTURE_REPORT) {
                        char *buffer;
                        asprintf(&buffer, "[Control Center] Station %d - Train departure. Station %d now has %d passengers.", station, station, num_of_passengers[station]);
                        ess_println(buffer);
                        free(buffer);
                    }
                    else if (report & TERMINATE_EVENT) {
                        terminated_stations++;
                    }
                    else {
                        ess_print_error("Unknown report value.");
                    }
                }
            }
        }
    }
}

void manageStation (const int station_pipes[2], const int station_id, volatile int *const num_of_passengers) {
    close(station_pipes[0]);

    srand(time(NULL) ^ ((unsigned long)getpid() << 16));

    int passengers = 0;

    for (int train = 0; train < NUM_OF_TRAINS; train++) {
        const int random = rand();
        sleep((random % 10) + 1);

        const int event = (random % 2 == 0) ? ARRIVAL_REPORT : DEPARTURE_REPORT;

        if (event & ARRIVAL_REPORT) {
            passengers += random%40+10;
        }
        else if (event & DEPARTURE_REPORT) {
            passengers -= random%40+10;
            if (passengers < 0) passengers = 0;
        }
        write(station_pipes[1], &event, sizeof(int));
        *num_of_passengers = passengers;
    }

    const int event = TERMINATE_EVENT;
    write(station_pipes[1], &event, sizeof(int));
    close(station_pipes[1]);
    _exit(EXIT_SUCCESS);
}

int main (const int argc, const char * const argv[]) {

    if (argc != 1) {
        ess_print_error("Invalid number of arguments.");
        exit(EXIT_FAILURE);
    }

    int station_pipes[NUM_OF_STATIONS][2];
    pid_t stations[NUM_OF_STATIONS];
    for (int i = 0; i < NUM_OF_STATIONS; i++) {
        if (pipe(station_pipes[i]) == -1) {
            ess_print_error("Error creating pipe.");
            exit(EXIT_FAILURE);
        }
    }

    const int shmid = shmget(IPC_PRIVATE, sizeof(int) * NUM_OF_STATIONS, IPC_CREAT | IPC_EXCL | 0600);
    if (shmid <= 0) {
        ess_print_error("Error getting shared mem region.");
        exit(EXIT_FAILURE);
    }

    int *const num_of_passengers = shmat(shmid, NULL, 0);
    if (num_of_passengers == (int *) -1) {
        ess_print_error("Error linking shared mem.");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < NUM_OF_STATIONS; i++) {
        num_of_passengers[i] = 0;
    }

    for (int station_id = 0; station_id < NUM_OF_STATIONS; station_id++) {
        stations[station_id] = fork();
        if (stations[station_id] == -1) {
            ess_print_error("Error creating Station.");
        }
        else if (stations[station_id] == 0) {
            //is child - Station
            manageStation(station_pipes[station_id], station_id, num_of_passengers);
        }
    }

    //parent - Central node
    for (int i = 0; i < NUM_OF_STATIONS; i++) {
        close(station_pipes[i][1]);
    }

    manageCentralNode(station_pipes, num_of_passengers);

    //terminating...
    for (int i = 0; i < NUM_OF_STATIONS; i++) {
        close(station_pipes[i][0]);
    }

    shmdt(num_of_passengers);
    shmctl(shmid, IPC_RMID, NULL);

    ess_println("Control Center: All stations have finished.");

    exit(EXIT_SUCCESS);
}
