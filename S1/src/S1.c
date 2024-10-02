// S1.c
// jeancarlo.saman@students.salle.url.edu - Jean Carlo Saman
// arnau.sf@students.salle.url.edu - Arnau Sanz Froiz
#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
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

#define STATE_NORMAL 1
#define STATE_CRITICAL_OXYGEN 2
#define STATE_ENERGY_FAILURE 4

struct Status {
    int code;
    time_t time;
};

struct Status current_status;
time_t sigusr2_actioned;

void report_status() {
    char *status_buffer;
    switch (current_status.code) {
        case STATE_NORMAL:
            asprintf(&status_buffer, "Normal state.");
            break;
        case STATE_CRITICAL_OXYGEN:
            asprintf(&status_buffer, "Critical oxygen level.");
            break;
        case STATE_ENERGY_FAILURE:
            asprintf(&status_buffer, "Energy failure.");
            break;
    }

    const int report_fd = open("drone_state.txt", O_WRONLY | O_TRUNC | O_CREAT);

    char *final_buffer;
    asprintf(&final_buffer, "Time: %s Status: Report: %s\n", ctime(&current_status.time), status_buffer);

    ess_print("\033[34mReport: ");
    ess_print(status_buffer);
    ess_println("\033[m");
    write(report_fd, final_buffer, strlen(final_buffer));

    close(report_fd);
    free(status_buffer);
    free(final_buffer);
}

void main_signal_handler(const int s);

void second_sigusr2_handler(const int s) {
    switch (s) {
        case SIGUSR2:
            ess_println("\033[31mEnergy failure detected.\033[m");
            current_status.code = STATE_ENERGY_FAILURE;
            current_status.time = time(NULL);
            break;
        case SIGUSR1:
            if (difftime(time(NULL), sigusr2_actioned) > 5) {
                ess_println("\033[31mOxygen state stabilized.\033[m");
            } else {
                ess_println("\033[31mCritical oxygen state detected.\033[m");
                current_status.code = STATE_CRITICAL_OXYGEN;
                current_status.time = time(NULL);
            }
            break;
            //signal(SIGUSR2, main_signal_handler);
        default:
            //any other signal
            break;
    }
    signal(SIGUSR2, main_signal_handler);
}

void rescue_successful() {
    ess_println("Rescue mission successful.");
    //exit(0);
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void ten_seconds() {
    ess_println("End of solar storm. All systems operational. Signals unblocked.");

    for (int i = 1; i <= 64; i++) {
        signal(i, main_signal_handler);
    }
}

void ignore_signal() {
    ess_println("\033[33mSolar storm detected.\033[m");
}

void solar_interference() {
    alarm(10);
    for (int i = 1; i <= 64; i++) {
        signal(i, ignore_signal);
    }
    signal(SIGALRM, ten_seconds);
    ess_println("Solar storm detected. All systems paused.");
}

void main_signal_handler(const int s) {
    switch (s) {
        case SIGUSR2:
            sigusr2_actioned = time(NULL);
            signal(SIGUSR1, second_sigusr2_handler);
            signal(SIGUSR2, second_sigusr2_handler);
            break;
        case SIGHUP:
            report_status();
            break;
        case SIGALRM:
            solar_interference();
            break;
        case SIGINT:
            rescue_successful();
            break;
    }
}

int main(void)
{
    for (int i = 1; i <= 64; i++) {
        signal(i, main_signal_handler);
    }

    char *buffer;
    asprintf(&buffer, "PID: %d\n", getpid());
    ess_print(buffer);
    free(buffer);
    ess_println("\033[35mRescue Drone AION initialized, waiting for signals...\033[m");

    current_status.code = STATE_NORMAL;
    current_status.time = time(NULL);

    while(1) {
        pause();
    }

    return 0;
}
