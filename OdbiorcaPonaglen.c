#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

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
        OnError("OdbiorcaPonaglen: Blad w funkcji SIGACTION!");
    }
}

void Handler(int sig, siginfo_t *si, void *ucontext) {
    printf("OdbiorcaPonaglen: Otrzymalem ponaglenie!\n");
}

int main(int argc, char *argv[]) {
    printf("Tu odbiorca ustanawiam obsluge SIGRTMIN+3 !\n");
    sigset_t signalsToBlock;
    sigemptyset(&signalsToBlock);
    SetSignalHandling(Handler, SIGRTMIN + 3, signalsToBlock, SA_RESTART);

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while (1) {
        sleep(5);
    }
#pragma clang diagnostic pop
    return 0;
}

