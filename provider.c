#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <float.h>
#include <pthread.h>
#include <math.h>

/**
 * NOTATKI:
 * Obsługa żądań realizowana jest w osobnym wątku. Czytanie z STDIN wykonywane jest po jednym znaku, aż do napotkania
 * znaku nowej linii. Następnie odczytywana jest informacja z żądania i wykonywana obsługa.
 * Zbiory realizowane są w nieskończonej pętli w main. *
 */

/**
 * Zmienne globalne
 */
float resourceValue = 0.0;
pid_t parasitePid = 0;
float requestValue = 0.0;
int signalInput = 0; //Sygnał potwierdzający spełnienie żądania


/**
 * Deklaracje funkcji
 */
void OnError(const char *message);

void SetSignalHandling(void(*handlerFunction)(int, siginfo_t *, void *), int signalToHandle, sigset_t signalsToBlock,
                       int sigactionFlags);

void GoToSleep(time_t seconds, long nanoseconds);

void SendConfirmationOfRequest(pid_t pid, int signal);

void ReadRequestFromStdin(char* buffer, int sizeOfBuffer);

static void* HandlingRequestsInThread(void* arg);

void HandlerSendResponseForReminder(int sig, siginfo_t *si, void *ucontext);

void SetRandomRealTimeHandling(float nonDeathPercentage, float responsePercentage);

/**
 * Funkcja main
 */
int main(int argc, char* argv[])
{
    char* cIncreaseRate = NULL;
    float increaseValue = 0.0;
    float gapBetweenCollections = 0.0; //Czas w sekundach pomiędzy zbiorami
    float gapBetweenCollectionsNano = 0.0; //Ułamkowa część wczytanego czasu w nanosekundach

    char* cResistanceRate = NULL;
    float deathResistance = 0.0; //Wartość w % - jaki procent sygnałów RT nie zabija
    float responseRate = 0.0; //Wartość w % - jaki procent przyjętych sygnałóœ RT zasługuje na odpowiedź

    srand(time(NULL));

    /**
     * Wczytywanie parametrów programu
     */
    int opt;
    while ((opt = getopt(argc, argv, "s:h:")) != -1) {
        switch (opt) {
            case 's':
                signalInput = (int) strtol(optarg, NULL, 10);
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
    increaseValue = strtof(cIncreaseRate, &afterSlash);
    gapBetweenCollections = strtof(afterSlash+1, &afterSlash);
    gapBetweenCollectionsNano = gapBetweenCollections - floorf(gapBetweenCollections);
    gapBetweenCollections = floorf(gapBetweenCollections);
    gapBetweenCollectionsNano *= 1000000000;
    deathResistance = strtof(cResistanceRate, &afterSlash);
    responseRate = strtof(afterSlash+1, NULL);

    /**
     * Ustanowienie losowej obsługi sygnałów RT - ponagleń
     */
     SetRandomRealTimeHandling(deathResistance, responseRate);

    /**
     * Uruchomienie obsługi żądań w wątku
     */
     pthread_t handlerOfRequests;
     if (pthread_create(&handlerOfRequests, NULL, HandlingRequestsInThread, NULL) != 0)
     {
         OnError("Provider: Blad uruchomienia watku do obslugi zadan!");
     }

     /**
      * Główna pętla programu
      */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
      while (1)
      {
          printf("Wartosc zasobu: %f\n", resourceValue); //TODO: Usuń debug!!
          GoToSleep((time_t)gapBetweenCollections,(long)gapBetweenCollectionsNano);
          resourceValue += increaseValue;
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
}

void ReadRequestFromStdin(char* buffer, int sizeOfBuffer) {
    char ch;
    int i = 0;
    while (read(STDIN_FILENO, &ch, 1) > 0)
    {
        buffer[i++] = ch;
        if (ch == '\n' || i >= sizeOfBuffer)
        {
            break;
        }
    }
}

void* HandlingRequestsInThread(void *arg) {
    char buffer[30] = {0};
    char* spacesBetweenNums = NULL;
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1)
    {
        ReadRequestFromStdin(buffer, 30);
        parasitePid = strtol(buffer, &spacesBetweenNums, 10);
        requestValue = strtof(spacesBetweenNums, NULL);
        memset(buffer, 0, sizeof(buffer));
        if (requestValue <= resourceValue && (resourceValue - requestValue) <= FLT_MAX)//Warunki spełnienia żądania
        {
            resourceValue -= requestValue;
            SendConfirmationOfRequest(parasitePid, signalInput);
        }
    }
#pragma clang diagnostic pop
    return NULL;
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
    for (int i = 0; i <= range; ++i) {
        int randomNum = (rand() % 100) + 1;
        if (randomNum <= nonDeathPercentage)
        {
            randomNum = (rand() % 100) + 1;
            if (randomNum <= responsePercentage)
            {
                sigset_t signalsToBlock;
                sigfillset(&signalsToBlock);
                SetSignalHandling(HandlerSendResponseForReminder, SIGRTMIN + i, signalsToBlock, SA_RESTART | SA_SIGINFO);
            } else
            {
                if (signal(SIGRTMIN + i, SIG_IGN) == SIG_ERR) {
                    OnError("Provider: Blad podczas ustawiania ignorowania sygnalu RT!");
                }
            }
        }
    }
}
