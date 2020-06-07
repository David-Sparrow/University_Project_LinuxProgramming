#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <values.h>
#include <time.h>


/**
 * Deklaracje funkcji
 */
void OnError(const char *message);

void GoToSleep(time_t seconds, long nanoseconds);


/**
 * Funkcja main
 */
int main(int argc, char* argv[])
{
    int signalInput = SIGHUP;
    char* cIncreaseRate = NULL;
    char* cResistanceRate = NULL;

    char* endptr = NULL;
    long strtolReturn = 0L;

    int parasitesCounter = argc - 5;

    int grandpaPid = 0;

    /**
     * Wczytywanie parametrów programu
     */
    int opt;
    while ((opt = getopt(argc, argv, "s:h:r:")) != -1) {
        switch (opt) {
            case 's':
                errno = 0;
                endptr = NULL;
                strtolReturn = strtol(optarg, &endptr, 10);
                signalInput = (int) strtolReturn;
                if ((errno == ERANGE && (strtolReturn == LONG_MAX || strtolReturn == LONG_MIN)) || (errno != 0 && strtolReturn == 0) || (endptr == optarg)) {
                    OnError("Familiada: Blad strtol przy wczytywaniu parametru -s");
                }
                parasitesCounter -= 2;
                break;
            case 'h':
                cIncreaseRate = optarg;
                break;
            case 'r':
                cResistanceRate = optarg;
                break;
            default:
                OnError("Familiada: Blad we wczytywaniu wartosci parametrow!");
                break;
        }
    }

    /**
     * Tworzenie pipe'a
     */
    int pipeFd[2];
    if (pipe(pipeFd) == -1){
        OnError("Familiada: Blad w tworzeniu pipe'a!");
    }

    /**
     * Tworzenie procesu dziadka
     */
    switch (grandpaPid = fork()) {
        case -1:
            OnError("Familiada: Blad w fork przy tworzeniu dziadka!");
            break;
        case 0:
            if (close(pipeFd[1]) == -1)
            {
                OnError("Familiada: Blad zamkniecia koncowki do pisania w procesie dziadka!");
            }
            if (pipeFd[0] != STDIN_FILENO)
            {
                if (dup2(pipeFd[0], STDIN_FILENO) == -1)
                    OnError("Familiada: Blad dupa w procesie dziadka!");
                if (close(pipeFd[0]) == -1)
                    OnError("Familiada: Blad zamkniecia koncowki do czytania w procesie dziadka!");
            }
            char signal[30] = {0};
            sprintf(signal, "%d", signalInput);
            execl("./provider", "Provider", "-s", signal, "-h", cIncreaseRate, cResistanceRate,(char*)NULL);
            OnError("Familiada: Blad funkcji execl w procesie dziadka!");
            break;
        default:
            break;
    }

    /**
     * Tworzenie procesu rodzica, który zrodzi pasożyty i popełni samobójstwo
     */
    switch (fork()) {
        case -1:
            OnError("Familiada: Blad w fork przy tworzeniu rodzica!");
            break;
        case 0:
            if (setpgid(0, 0) == -1)
                OnError("Familiada: Blad setpgid w rodzicu!");

            for (int i = 0; i < parasitesCounter; ++i) {
                switch (fork()) {
                    case -1:
                        OnError("Familiada: Blad stworzenia pasozyta!");
                        break;
                    case 0:
                        if (close(pipeFd[0]) == -1)
                        {
                            OnError("Familiada: Blad zamkniecia koncowki do czytania w procesie pasozyta!");
                        }
                        if (pipeFd[1] != STDOUT_FILENO)
                        {
                            if (dup2(pipeFd[1], STDOUT_FILENO) == -1)
                                OnError("Familiada: Blad dupa w procesie pasozyta!");
                            if (close(pipeFd[1]) == -1)
                                OnError("Familiada: Blad zamkniecia koncowki do pisania w procesie pasozyta!");
                        }
                        char signal[30] = {0};
                        sprintf(signal, "%d", signalInput);
                        char pid[30] = {0};
                        sprintf(pid, "%d", grandpaPid);
                        char* arg = argv[optind];
                        char timeGap[30] = {0};
                        char registerValue[30] = {0};
                        for (int j = 0; j < 30; ++j) {
                            if (*arg != ':')
                            {
                                timeGap[j] = *arg;
                            } else
                            {
                                arg++;
                                break;
                            }
                            arg++;
                        }
                        for (int k = 0; k < 30; ++k) {
                            registerValue[k] = *arg;
                            if (*arg == '\0')
                            {
                                break;
                            }
                            arg++;
                        }
                        execl("./parasite", "Pasozyt", "-s", signal, "-p", pid, "-d", timeGap, "-v", registerValue,(char*)NULL);
                        OnError("Familiada: Blad funkcji execl w procesie pasozyta!");
                        break;
                    default:
                        optind++;
                        break;
                }
            }
            kill(getpid(), SIGKILL);
        default:
            break;
    }

    /**
     * Familiada udaje się na długi spoczynek
     */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
     while (1)
     {
         GoToSleep(5,0L);
     }
#pragma clang diagnostic pop

}

/**
 * Definicje funkcji
 */
void OnError(const char *message) {
    perror(message);
    exit(666);
}

void GoToSleep(time_t seconds, long nanoseconds) {
    struct timespec ts;
    struct timespec timeLeft;
    _Bool sleepSuccess = 0;
    while (nanoseconds > 999999999L) {
        nanoseconds -= 1000000000L;
        seconds++;
    }
    ts.tv_sec = seconds;
    ts.tv_nsec = nanoseconds;
    if (nanosleep(&ts, &timeLeft) == 0) {
        sleepSuccess = 1;
    }
    while (sleepSuccess == 0) {
        if (nanosleep(&timeLeft, &timeLeft) == 0) {
            sleepSuccess = 1;
        }
    }
}