#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <float.h>
#include <math.h>
#include <ctype.h>
#include <sys/time.h>
#include <errno.h>
#include <values.h>

/**
 * NOTATKI:
 * Obsługa żądań realizowana jest w głównej pętli programu w main. Czytanie z STDIN wykonywane jest po jednym znaku,
 * aż do napotkania znaku nowej linii. Następnie odczytywana jest informacja z żądania i wykonywana obsługa.
 * Zbiory realizowane są jako reakcja na sygnał SIGALRM pochodzący z budzika.
 */

/**
 * Zmienne globalne
 */
float resourceValue = 0.0;
pid_t parasitePid = 0;
float requestValue = 0.0;
int signalInput = 0; //Sygnał potwierdzający spełnienie żądania
float increaseValue = 0.0;

/**
 * Deklaracje funkcji
 */
void OnError(const char *message);

void SetSignalHandling(void(*handlerFunction)(int, siginfo_t *, void *), int signalToHandle, sigset_t signalsToBlock,
                       int sigactionFlags);

void GoToSleep(time_t seconds, long nanoseconds);

void SendConfirmationOfRequest(pid_t pid, int signal);

void ReadRequestFromStdin(char* buffer, int sizeOfBuffer);

void HandlerSendResponseForReminder(int sig, siginfo_t *si, void *ucontext);

void SetRandomRealTimeHandling(float nonDeathPercentage, float responsePercentage);

int ValidateRequest(char* request, int sizeOfRequest);

void HandlerTimer(int sig, siginfo_t *si, void *ucontext);

void SetTimer(time_t seconds, long nanoseconds);


/**
 * Funkcja main
 */
int main(int argc, char* argv[])
{
    char* cIncreaseRate = NULL;
    float gapBetweenCollections = 0.0; //Czas w sekundach pomiędzy zbiorami
    float gapBetweenCollectionsMicro = 0.0; //Ułamkowa część wczytanego czasu w mikrosekundach

    char* cResistanceRate = NULL;
    float deathResistance = 0.0; //Wartość w % - jaki procent sygnałów RT nie zabija
    float responseRate = 0.0; //Wartość w % - jaki procent przyjętych sygnałóœ RT zasługuje na odpowiedź

    char* endptr = NULL;
    long strtolReturn = 0L;

    srand(time(NULL));

    /**
     * Wczytywanie parametrów programu
     */
    int opt;
    while ((opt = getopt(argc, argv, "s:h:")) != -1) {
        switch (opt) {
            case 's':
                errno = 0;
                endptr = NULL;
                strtolReturn = strtol(optarg, &endptr, 10);
                signalInput = (int) strtolReturn;
                if ((errno == ERANGE && (strtolReturn == LONG_MAX || strtolReturn == LONG_MIN)) || (errno != 0 && strtolReturn == 0) || (endptr == optarg)) {
                    OnError("Provider: Blad strtol przy wczytywaniu parametru -s");
                }
                break;
            case 'h':
                cIncreaseRate = optarg;
                break;
            default:
                OnError("Provider: Blad we wczytywaniu wartosci parametrow!");
                break;
        }
    }
    cResistanceRate = argv[optind]; //Wczytanie parametru, który nie jest oznaczony flagą

    char* afterSlash = NULL;
    errno = 0;
    increaseValue = strtof(cIncreaseRate, &afterSlash);
    if ((errno == ERANGE && (increaseValue == HUGE_VALF || increaseValue == -HUGE_VALF)) || (errno != 0 && increaseValue == 0) || (afterSlash == cIncreaseRate)) {
        OnError("Provider: Blad strtof przy wczytywaniu parametru -h");
    }
    errno = 0;
    endptr = NULL;
    gapBetweenCollections = strtof(afterSlash+1, &endptr);
    if ((errno == ERANGE && (gapBetweenCollections == HUGE_VALF || gapBetweenCollections == -HUGE_VALF)) || (errno != 0 && gapBetweenCollections == 0) || (afterSlash+1 == endptr)) {
        OnError("Provider: Blad strtof przy wczytywaniu parametru -h");
    }
    gapBetweenCollectionsMicro = gapBetweenCollections - floorf(gapBetweenCollections);
    gapBetweenCollections = floorf(gapBetweenCollections);
    gapBetweenCollectionsMicro *= 1000000;
    errno = 0;
    deathResistance = strtof(cResistanceRate, &afterSlash);
    if ((errno == ERANGE && (deathResistance == HUGE_VALF || deathResistance == -HUGE_VALF)) || (errno != 0 && deathResistance == 0) || (afterSlash == cResistanceRate)) {
        OnError("Provider: Blad strtof przy wczytywaniu parametru pozycyjnego");
    }
    errno = 0;
    endptr = NULL;
    responseRate = strtof(afterSlash+1, &endptr);
    if ((errno == ERANGE && (responseRate == HUGE_VALF || responseRate == -HUGE_VALF)) || (errno != 0 && responseRate == 0) || (afterSlash+1 == endptr)) {
        OnError("Provider: Blad strtof przy wczytywaniu parametru pozycyjnego");
    }

    /**
     * Ustanowienie losowej obsługi sygnałów RT - ponagleń
     */
     SetRandomRealTimeHandling(deathResistance, responseRate);

    /**
    * Ustanowienie obsługi sygnału SIGALRM - sygnał budzika taktującego przyrost zasobu
    */
    sigset_t signalsToBlock;
    sigfillset(&signalsToBlock);
    SetSignalHandling(HandlerTimer, SIGALRM, signalsToBlock, SA_RESTART);

    /**
     * Włączenie timer'a
     */
    SetTimer((time_t) gapBetweenCollections, (long) gapBetweenCollectionsMicro);

    /**
     * Główna pętla programu
     */
    char buffer[512] = {0};
    char* spacesBetweenNums = NULL;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1)
    {
        ReadRequestFromStdin(buffer, 512);
        if (ValidateRequest(buffer, 512) == -1)
        {
            printf("Provider: Bledny format zadania! Ignoruje zadanie!\n");
            memset(buffer, 0, sizeof(buffer));
            continue;
        }
        fprintf(stderr, "Provider DEBUG: Po pozytywnej walidacji!\n"); //TODO: DEBUG
        parasitePid = strtol(buffer, &spacesBetweenNums, 10);
        requestValue = strtof(spacesBetweenNums, NULL);
        memset(buffer, 0, sizeof(buffer));
        fprintf(stderr, "Provider DEBUG: Odczytane wartosci: %d\t%f\n", parasitePid, requestValue); //TODO: DEBUG
        if (requestValue <= resourceValue && (resourceValue - requestValue) <= FLT_MAX)//Warunki spełnienia żądania
        {
            resourceValue -= requestValue;
            SendConfirmationOfRequest(parasitePid, signalInput);
        }
    }
#pragma clang diagnostic pop
    return 0;
}

/**
 * Definicje funkcji
 */
void OnError(const char *message) {
    perror(message);
    exit(666);
}

void SetSignalHandling(void(*handlerFunction)(int, siginfo_t *, void *), int signalToHandle, sigset_t signalsToBlock,
                       int sigactionFlags) {
    struct sigaction sa;
    sa.sa_sigaction = handlerFunction;
    sa.sa_mask = signalsToBlock;
    sa.sa_flags = sigactionFlags;

    if (sigaction(signalToHandle, &sa, NULL) == -1) {
        OnError("Provider: Blad w funkcji SIGACTION!");
    }
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

void SendConfirmationOfRequest(pid_t pid, int signal) {
    if (kill(pid, signal) == -1)
    {
        OnError("Provider: Blad wysylania potwierdzenia spelnienia zadania!");
    }
    fprintf(stderr,"Provider: DEBUG: Wyslalem potwierdzenie wykonania zadania na: %d\n", pid); //TODO: DEBUG
}

void ReadRequestFromStdin(char* buffer, int sizeOfBuffer) {
    int res = 66;
    res = read(STDIN_FILENO, buffer, 20);
    if (res == 0)
    {
        fprintf(stderr, "Provider: Wszystkie pasozyty nie zyja. Koncze dzialanie!\n");
        exit(0);
    }
}

void HandlerSendResponseForReminder(int sig, siginfo_t *si, void *ucontext) {
    union sigval sv;
    sv.sival_int = 0;
    sv.sival_ptr = NULL;

    if (sigqueue(si->si_pid, si->si_signo, sv) == -1) {
        OnError("Provider: Blad podczas wysylania odpowiedzi na ponaglenie!");
    }
}

void SetRandomRealTimeHandling(float nonDeathPercentage, float responsePercentage) {
    int range = SIGRTMAX - SIGRTMIN;
    int nonMortalSignals = (int) (nonDeathPercentage * (float) (range + 1) / 100.0);
    int responsiveSignals = (int) (responsePercentage * (float) nonMortalSignals / 100.0);

    int *randomSignals = (int *) calloc(nonMortalSignals, sizeof(int));
    memset(randomSignals, -1, sizeof(int) * nonMortalSignals);

    int signalsFound = 0;
    _Bool isDuplicate = 0;
    while (signalsFound < nonMortalSignals) {
        int randomNum = rand() % (range + 1);
        for (int i = 0; i < nonMortalSignals && !isDuplicate; ++i) {
            if (randomSignals[i] == randomNum) {
                isDuplicate = 1;
            }
        }
        if (isDuplicate) {
            isDuplicate = 0;
            continue;
        } else {
            randomSignals[signalsFound++] = randomNum;
        }
    }

    for (int i = 0; i < nonMortalSignals; ++i) {
        if (i < responsiveSignals) {
            sigset_t signalsToBlock;
            sigfillset(&signalsToBlock);
            SetSignalHandling(HandlerSendResponseForReminder, SIGRTMIN + randomSignals[i], signalsToBlock,
                              SA_RESTART | SA_SIGINFO);
            //fprintf(stderr,"Provider DEBUG: Ustanowiono obsluge SIGRTMIN+%d\n", randomSignals[i]); //TODO: DEBUG
        } else {
            if (signal(SIGRTMIN + randomSignals[i], SIG_IGN) == SIG_ERR) {
                OnError("Provider: Blad podczas ustawiania ignorowania sygnalu RT!");
            }
            //fprintf(stderr,"Provider DEBUG: Ustanowiono ignorowanie SIGRTMIN+%d\n", randomSignals[i]); //TODO: DEBUG
        }
    }
    free(randomSignals);
}

int ValidateRequest(char *request, int sizeOfRequest) {
    int numsCounter = 0;
    _Bool encounteredDot = 0;
    _Bool encounteredNewLine = 0;
    _Bool isValid = 0;
    fprintf(stderr,"Provider DEBUG: Tu funkcja ValidateRequest: Odczytuje ciag znakow: "); //TODO: DEBUG
    for (int i = 0; i < sizeOfRequest - 1; ++i) {
        fprintf(stderr,"%c", request[i]); //TODO: DEBUG
        if (isdigit(request[i]) || isblank(request[i]) || request[i] == '.' || request[i] == '\n')
        {
            if (isdigit(request[i]) && (isblank(request[i+1]) || request[i+1] == '\n'))
            {
                numsCounter++;
                if (numsCounter == 2)
                {
                    isValid = 1;
                }
            }

            if (request[i] == '.' && !encounteredDot)
            {
                encounteredDot = 1;
            }

            if (request[i] == '\n' && !encounteredNewLine)
            {
                encounteredNewLine = 1;
            }

            if ((request[i+1] == '\n' && encounteredNewLine) || (request[i+1] == '.' && encounteredDot) || numsCounter > 2)
            {
                return -1;
            }
        } else if (request[i] == '\0' && isValid)
        {
            return 1;
        } else
        {
            return -1;
        }
    }
    return 1;
}

void HandlerTimer(int sig, siginfo_t *si, void *ucontext) {
    resourceValue += increaseValue;
    fprintf(stderr,"Provider DEBUG: Wartosc zasobu: %f\n", resourceValue); //TODO: DEBUG
}

void SetTimer(time_t seconds, long microseconds) {
    struct itimerval itv;

    while (microseconds > 999999L) {
        microseconds -= 1000000L;
        seconds++;
    }
    itv.it_value.tv_sec = seconds;
    itv.it_value.tv_usec = microseconds;

    itv.it_interval.tv_sec = seconds;
    itv.it_interval.tv_usec = microseconds;

    if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
        OnError("Provider: Blad przy ustawianiu timer-a!");
    }
}