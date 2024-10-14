// S2.c
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

enum Status{
    WAITING,
    START_STRING,
    START_WIND,
    START_PERCUSSION,
    END
};

enum Status flag = WAITING;

#define COLOR_DEFAULT "\033[m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BLUE "\033[34m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_GREEN "\033[32m"
#define COLOR_RED "\033[31m"

void ignore_signal() {
}

void start_percussion() {
    signal(SIGUSR1, ignore_signal);
    flag = START_PERCUSSION;
}

void start_percussion_group(){

    const pid_t child_percussion = fork();
    if (child_percussion > 0) {
        //parent - Conductor
        const pid_t status = waitpid(child_percussion, NULL, 0);
        if (status != child_percussion) {
            ess_print_error("Error on waiting for Percussion group.");
            exit(-1);
        }

        signal(SIGUSR1, start_percussion);
        ess_println("Director: Percussion section completed.");
        ess_println("\nConcert finished successfully.");
        exit(0);

    } else if (child_percussion == 0) {
        //Wind
        const pid_t child_flute = fork();
        if (child_flute > 0) {
            //Wind
            const pid_t child_clarinet1 = fork();
            if (child_clarinet1 > 0) {
                //Wind

                    //String
                    pid_t status = waitpid(child_flute, NULL, 0);
                    if (status != child_flute) {
                        ess_print_error("Error on waiting for Percussion instrument.");
                        _exit(-1);
                    }
                    status = waitpid(child_clarinet1, NULL, 0);
                    if (status != child_clarinet1) {
                        ess_print_error("Error on waiting for Percussion instrument.");
                        _exit(-1);
                    }

                    _exit(0);

            } else if (child_clarinet1 == 0) {
                //triangle
                ess_println(COLOR_MAGENTA "Percussion Triangle are playing: Do Re Re Mi" COLOR_DEFAULT);
                sleep(2);
                _exit(0);
            } else {
                ess_print_error("Error creating Percussion instrument.");
                _exit(-1);
            }
        } else if (child_flute == 0) {
            //vibraphone
            ess_println(COLOR_MAGENTA "Percussion Vibraphone is playing: Do Re Mi" COLOR_DEFAULT);
            sleep(2);
            _exit(0);
        } else {
            ess_print_error("Error creating Percussion instrument.");
            _exit(-1);
        }
    } else {
        ess_print_error("Error creating Percussion child.");
        exit(-1);
    }
}

void start_wind() {
    signal(SIGUSR1, ignore_signal);
    flag = START_WIND;
}

void start_wind_group() {
    const pid_t child_wind = fork();
    if (child_wind > 0) {
        //parent - Conductor
        const pid_t status = waitpid(child_wind, NULL, 0);
        if (status != child_wind) {
            ess_print_error("Error on waiting for Wind group.");
            exit(-1);
        }

        signal(SIGUSR1, start_percussion);
        ess_println("Director: Wind section completed.");
        ess_println(COLOR_RED " Waiting to start Percussion section." COLOR_DEFAULT);
        ess_println("Section is ready to start.");

    } else if (child_wind == 0) {
        //Wind
        const pid_t child_flute = fork();
        if (child_flute > 0) {
            //Wind
            const pid_t child_clarinet1 = fork();
            if (child_clarinet1 > 0) {
                //Wind

                    //String
                    pid_t status = waitpid(child_flute, NULL, 0);
                    if (status != child_flute) {
                        ess_print_error("Error on waiting for Wind instrument.");
                        _exit(-1);
                    }
                    status = waitpid(child_clarinet1, NULL, 0);
                    if (status != child_clarinet1) {
                        ess_print_error("Error on waiting for Wind instrument.");
                        _exit(-1);
                    }

                    _exit(0);

            } else if (child_clarinet1 == 0) {
                //clarinet 1
                ess_println(COLOR_BLUE "Wind Clarinet 1 is playing: Re Re" COLOR_DEFAULT);
                sleep(2);
                _exit(0);
            } else {
                ess_print_error("Error creating Wind instrument.");
                _exit(-1);
            }
        } else if (child_flute == 0) {
            //viola
            ess_println(COLOR_BLUE "Wind Flute is playing: Do Do" COLOR_DEFAULT);
            sleep(2);
            _exit(0);
        } else {
            ess_print_error("Error creating Wind instrument.");
            _exit(-1);
        }
    } else {
        ess_print_error("Error creating Wind child.");
        exit(-1);
    }
}

void start_string () {
    signal(SIGUSR1, ignore_signal);
    flag = START_STRING;
}

void start_string_group() {

    const pid_t child_string = fork();
    if (child_string > 0) {
        //parent - Conductor
        const pid_t status = waitpid(child_string, NULL, 0);
        if (status != child_string) {
            ess_print_error("Error on waiting for String group.");
            exit(-1);
        }

        signal(SIGUSR1, start_wind);
        ess_println("Director: Strings section completed.");
        ess_println(COLOR_RED " Waiting to start Wind section." COLOR_DEFAULT);
        ess_println("Section is ready to start.");
    } else if (child_string == 0) {
        //String
        const pid_t child_viola = fork();
        if (child_viola > 0) {
            //String
            const pid_t child_violin1 = fork();
            if (child_violin1 > 0) {
                //String
                const pid_t child_violin2 = fork();
                if (child_violin2 > 0) {
                    //String
                    pid_t status = waitpid(child_viola, NULL, 0);
                    if (status != child_viola) {
                        ess_print_error("Error on waiting for String instrument.");
                        _exit(-1);
                    }
                    status = waitpid(child_violin1, NULL, 0);
                    if (status != child_violin1) {
                        ess_print_error("Error on waiting for String instrument.");
                        _exit(-1);
                    }
                    status = waitpid(child_violin2, NULL, 0);
                    if (status != child_violin2) {
                        ess_print_error("Error on waiting for String instrument.");
                        _exit(-1);
                    }
                    _exit(0);

                } else if (child_violin2 == 0) {
                    //violin 2
                    ess_println(COLOR_GREEN "Strings Violin 2 is playing: Re" COLOR_DEFAULT);
                    sleep(2);
                    _exit(0);
                } else {
                    ess_print_error("Error creating String instrument.");
                    _exit(-1);
                }
            } else if (child_violin1 == 0) {
                //violin 1
                ess_println(COLOR_GREEN "Strings Violin 1 is playing: Re" COLOR_DEFAULT);
                sleep(2);
                _exit(0);
            } else {
                ess_print_error("Error creating String instrument.");
                _exit(-1);
            }
        } else if (child_viola == 0) {
            //viola
            ess_println(COLOR_GREEN "Strings Viola is playing: Do" COLOR_DEFAULT);
            sleep(2);
            _exit(0);
        } else {
            ess_print_error("Error creating String instrument.");
            _exit(-1);
        }
    } else {
        ess_print_error("Error creating String child.");
        exit(-1);
    }
}

int main(const int argc, char *argv[]) {
    if (argc > 1) {
        ess_print("The following arguments were given: ");
        ess_println(argv[1]);
        ess_print_error("No arguments are accepted by this program.");
    }

    char *buffer;
    asprintf(&buffer, COLOR_YELLOW "Director (PID %d) starting the concert. Use 'kill -SIGUSR1 PID' to start sections." COLOR_DEFAULT, getpid());
    ess_println(buffer);
    free(buffer);

    signal(SIGUSR1, start_string);
    signal(SIGINT, ignore_signal);
    ess_println(COLOR_YELLOW "Section is ready to start." COLOR_DEFAULT);

    while(1) {
        pause();

        switch (flag) {
            case WAITING:
                break;
            case START_STRING:
                start_string_group();
                break;
            case START_WIND:
                start_wind_group();
                break;
            case START_PERCUSSION:
                start_percussion_group();
                break;
            default:
                ess_print_error("Unknown status.");
                break;
        }
    }

    return 0;
}
