#include <stdio.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX_LENGTH 1024

typedef struct message
{
    long mtype;
    char text[MAX_LENGTH];
} message;

int main(int argc, char const *argv[])
{
    int msgid = msgget(atoi(argv[1]), 0666 | IPC_CREAT);
    message m;
    msgrcv(msgid, &m, MAX_LENGTH + 1, 1, 0);
    printf("%s\n", m.text);
    return 0;
}