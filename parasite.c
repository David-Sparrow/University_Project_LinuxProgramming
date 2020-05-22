#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

struct LastRequest {
    float requestValue;
    int remindersCount;
    int answersCount;
    _Bool completed;
};

/**
 * Zmienne globalne
 */
float fRequestRegister = 0;
pid_t myPid = 0;
int completedRequests = 0;
int sentReminders = 0;
struct LastRequest lastRequest;


/**
 * Deklaracje funkcji
 */
void OnError(const char *message);

void SendRequest(pid_t myPid, float requestRegister);

void SendReminder(pid_t receiverPid);

void SetSignalHandling(void(*handlerFunction)(int, siginfo_t *, void *), int signalToHandle, sigset_t signalsToBlock,
                       int sigactionFlags);

void GoToSleep(time_t seconds, long nanoseconds);

void HandlerConfirmationOfRequest(int sig, siginfo_t *si, void *ucontext);

void HandlerAnswerToReminder(int sig, siginfo_t *si, void *ucontext);

void HandlerSigPipe(int sig, siginfo_t *si, void *ucontext);

/**
 * Funkcja main
 */
int main(int argc, char *argv[]) {

    int nSignal = 0;
    pid_t procPid = 0;
    float fTimeGap = 0;
    myPid = getpid();

    srand(time(NULL));

    /**
     * Wczytywanie parametrow programu
     */
    int opt;
    while ((opt = getopt(argc, argv, "s:p:d:v:")) != -1) {
        switch (opt) {
            case 's':
                nSignal = (int) strtol(optarg, NULL, 10);
                break;

            case 'p':
                procPid = (int) strtol(optarg, NULL, 10);
                break;

            case 'd':
                fTimeGap = strtof(optarg, NULL);
                break;

            case 'v':
                fRequestRegister = strtof(optarg, NULL);
                break;
            default:
                OnError("Pasozyt: Blad we wczytywaniu wartosci parametrow!");
                break;
        }
    }

    long lNanoTimeGap = (long) (fTimeGap * 100000000.0);

    /**
     * Ustanowienie obslugi potwierdzenia spelnienia zadania
     */
    sigset_t signalsToBlock;
    sigemptyset(&signalsToBlock);
    sigaddset(&signalsToBlock, nSignal);
    SetSignalHandling(HandlerConfirmationOfRequest, nSignal, signalsToBlock, SA_RESTART);

    /**
     * Ustanowienie obslugi SIGPIPE
     */
    sigfillset(&signalsToBlock);
    SetSignalHandling(HandlerSigPipe, SIGPIPE, signalsToBlock, 0);

    /**
     * Glowna petla programu
     */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        int tempCompletedRequests = completedRequests;
        SendRequest(myPid, fRequestRegister);
        GoToSleep(0, lNanoTimeGap);
        if (completedRequests == tempCompletedRequests)//Jesli sa rowne tzn, ze odpowiedz nie nadeszla
        {
            SendReminder(procPid);
        } else//TODO: Usunac debug
        {
            printf("Debug: Nie wyslalem ponaglenia!\n");
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

void SendRequest(pid_t myPid, float requestRegister) {
    char requestMessage[20] = {0};
    sprintf(requestMessage, "%d %.2f\n", myPid, requestRegister);
    if (write(STDOUT_FILENO, requestMessage, sizeof(requestMessage)) == -1) {
        OnError("Pasozyt: Blad podczas wypisywania zadania!");
    }
    lastRequest.requestValue = requestRegister;
    lastRequest.remindersCount = 0;
    lastRequest.answersCount = 0;
    lastRequest.completed = 0;
}

void SendReminder(pid_t receiverPid) {
    union sigval sv;
    sv.sival_int = 0;
    sv.sival_ptr = NULL;

    int range = SIGRTMAX - SIGRTMIN;
    int value = rand() % (range + 1);

    /**
     * Ustanowienie obslugi ewentualnej odpowiedzi na ponaglenie
     */
    sigset_t signalsToBlock;
    sigemptyset(&signalsToBlock);//TODO: Usunac debug linijke nizej
    SetSignalHandling(HandlerAnswerToReminder, SIGRTMIN + 3, signalsToBlock, SA_RESTART | SA_SIGINFO);
//TODO: Usunac debug linijke nizej
    if (sigqueue(receiverPid, SIGRTMIN + 3, sv) == -1) {
        OnError("Pasozyt: Blad podczas wysylania ponaglenia!");
    }
    sentReminders++;
    lastRequest.remindersCount++;
}

void HandlerConfirmationOfRequest(int sig, siginfo_t *si, void *ucontext) {
    fRequestRegister += fRequestRegister * (float) 0.25;
    completedRequests++;
    lastRequest.completed = 1;
}

void HandlerAnswerToReminder(int sig, siginfo_t *si, void *ucontext) {
    fRequestRegister -= fRequestRegister * (float) 0.2;
    lastRequest.answersCount++;
}

void SetSignalHandling(void(*handlerFunction)(int, siginfo_t *, void *), int signalToHandle, sigset_t signalsToBlock,
                       int sigactionFlags) {
    struct sigaction sa;
    sa.sa_sigaction = handlerFunction;
    sa.sa_mask = signalsToBlock;
    sa.sa_flags = sigactionFlags;

    if (sigaction(signalToHandle, &sa, NULL) == -1) {
        OnError("Pasozyt: Blad w funkcji SIGACTION!");
    }
}

void HandlerSigPipe(int sig, siginfo_t *si, void *ucontext) {
    char preMortalReport[512] = {0};
    sprintf(preMortalReport, "PID:   %d\nSuma spelnionych zadan:   %d\nIlosc wyslanych ponaglen:   %d\n"
                             "Wysokosc zadania:   %.2f\nIlosc ponaglen:   %d\nIlosc odpowiedzi na ponaglenia:   %d\nSpelnione:   %d\n",
            myPid, completedRequests, sentReminders, lastRequest.requestValue, lastRequest.remindersCount,
            lastRequest.answersCount, lastRequest.completed);

    if (write(STDERR_FILENO, preMortalReport, sizeof(preMortalReport)) == -1) {
        OnError("Pasozyt: Blad podczas wypisywania raportu przedsmiertnego!");
    }

    exit(128 + SIGPIPE);
}

void GoToSleep(time_t seconds, long nanoseconds) {
    struct timespec ts;
    while (nanoseconds > 999999999L) {
        nanoseconds -= 1000000000L;
        seconds++;
    }
    ts.tv_sec = seconds;
    ts.tv_nsec = nanoseconds;
    nanosleep(&ts, NULL);
}

