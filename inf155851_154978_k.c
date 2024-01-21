#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "inf155851_154978_mqipc.h"

int *cmsgid = 0;  // client message queue id
int shared;       // shared memory id
char username[32];
char blocklist[32][32];  // list of blocked users

void printError(char *msg) {
    fprintf(stderr, "%s\nError: %s\n", msg, strerror(errno));
}

void blockUser() {
    // user input
    char user[32];
    printf("User to block: ");
    while (1) {
        // read 32-1 characters from stdin (31 characters + null terminator)
        if (scanf(" %31s", user) > 0) {
            break;
        }
        printf("Invalid username.\n");
    }
    // check if user is already blocked
    for (int i = 0; i < 32; i++) {
        if (!strcmp(blocklist[i], user)) {
            printf("User is already blocked.\n");
            return;
        }
    }
    // add user to blocklist
    for (int i = 0; i < 32; i++) {
        if (!strcmp(blocklist[i], "")) {
            strcpy(blocklist[i], user);
            printf("User blocked.\n");
            return;
        }
    }
    printf("Blocklist is full.\n");
}

int isBlocked(char *user) {
    for (int i = 0; i < 32; i++) {
        if (!strcmp(blocklist[i], user)) {
            return 1;
        }
    }
    return 0;
}

int connectToServer() {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
    }
    return msgid;
}

int login() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return 0;
    }
    // create login message
    msg_login login;
    login.mtype = M_LOGIN;
    // wait for user input
    printf("Username: ");
    while (1) {
        // read 32-1 characters from stdin (31 characters + null terminator)
        if (scanf("%31s", login.username) > 0) {
            break;
        }
        printf("Invalid username.\n");
    }
    login.cmsgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT);
    if (login.cmsgid == -1) {
        printError("Failed to create message queue.");
        return 0;
    }
    // send login message
    msgsnd(msgid, &login, sizeof(msg_login), 0);
    // wait for server to respond
    msg_response response;
    msgrcv(login.cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
    if (response.status == M_SUCCESS) {
        strcpy(username, login.username);
        return login.cmsgid;
    } else {
        msgctl(login.cmsgid, IPC_RMID, NULL);
    }
    return 0;
}

void logout() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return;
    }
    // create logout message
    msg_logout logout;
    logout.mtype = M_LOGOUT;
    logout.cmsgid = *cmsgid;
    // send logout message
    msgsnd(msgid, &logout, sizeof(msg_logout), 0);
    // delete user queue
    msgctl(*cmsgid, IPC_RMID, NULL);
    return;
}

void createRoom() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return;
    }
    // create new room message
    msg_create_room create_room;
    create_room.mtype = M_CREATE_ROOM;
    create_room.cmsgid = *cmsgid;
    // wait for user input
    printf("Room name: ");
    while (1) {
        // read 32-1 characters from stdin (31 characters + null terminator)
        if (scanf(" %31s", create_room.room_name) > 0) {
            break;
        }
        printf("Invalid room name.\n");
    }
    // send create room message
    msgsnd(msgid, &create_room, sizeof(msg_create_room), 0);
    // wait for server to respond
    msg_response response;
    msgrcv(*cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
}

void listRooms() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return;
    }
    // create list rooms message
    msg_list_rooms list_rooms;
    list_rooms.mtype = M_LIST_ROOMS;
    list_rooms.cmsgid = *cmsgid;
    // send list rooms message
    msgsnd(msgid, &list_rooms, sizeof(msg_list_rooms), 0);
    // wait for server to respond
    msg_response response;
    while (1) {
        msgrcv(*cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
        printf("Server response: %s\n", response.message);
        if (response.status != M_MORE) {
            break;
        }
    }
}

void joinRoom() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return;
    }
    // create join room message
    msg_join_room join_room;
    join_room.mtype = M_JOIN_ROOM;
    join_room.cmsgid = *cmsgid;
    strcpy(join_room.username, username);
    // wait for user input
    printf("Room name: ");
    while (1) {
        // read 32-1 characters from stdin (31 characters + null terminator)
        if (scanf(" %31s", join_room.room_name) > 0) {
            break;
        }
        printf("Invalid room name.\n");
    }
    printf("Type of subscribtion (-1 infinite, >0 number of messages): ");
    while (1) {
        // read integer from stdin
        if (scanf("%d", &join_room.subscribtion) != 1) {
            printf("Invalid subscribtion type.\n");
            while (getchar() != '\n' && getchar() != EOF)
                ;
            continue;
        }
        if (join_room.subscribtion < -1) {
            printf("Invalid subscribtion type.\n");
            continue;
        }
        if (join_room.subscribtion == 0) {
            printf("Invalid subscribtion type.\n");
            continue;
        }
        break;
    }
    join_room.subscribtion++;
    // send join room message
    msgsnd(msgid, &join_room, sizeof(msg_join_room), 0);
    // wait for server to respond
    msg_response response;
    msgrcv(*cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
}

void sendMessage() {
    // connect to server
    int msgid = connectToServer();
    if (msgid == -1) {
        return;
    }
    msg_send_message msg;
    msg.mtype = M_SEND_MESSAGE;
    strcpy(msg.author, username);
    msg.cmsgid = *cmsgid;
    // wait for user input
    printf("Room name: ");
    while (1) {
        // read 32-1 characters from stdin (31 characters + null terminator)
        if (scanf(" %31s", msg.room_name) > 0) {
            break;
        }
        printf("Invalid room name.\n");
    }
    printf("Message: ");
    while (1) {
        // read 256-1 characters from stdin (255 characters + null terminator)
        if (scanf(" %255[^\n]", msg.message) > 0) {
            break;
        }
        printf("Invalid message.\n");
    }
    printf("Priority (1-10): ");
    while (1) {
        // read integer from stdin
        if (scanf("%d", &msg.priority) != 1) {
            printf("Invalid priority.\n");
            while (getchar() != '\n' && getchar() != EOF)
                ;
            continue;
        }
        if (msg.priority < 1 || msg.priority > 10) {
            printf("Invalid priority.\n");
            continue;
        }
        break;
    }
    // send message
    msgsnd(msgid, &msg, sizeof(msg_send_message), 0);
    // wait for server to respond
    msg_response response;
    msgrcv(*cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
}

void readMessage() {
    msg_send_message msg;
    do {
        msgrcv(*cmsgid, &msg, sizeof(msg_send_message), -10, 0);
    } while (isBlocked(msg.author));
    printf("> %s@%s said: %s\n", msg.author, msg.room_name, msg.message);
}

void exitHandler(int sig) {
    if (*cmsgid)
        logout();
    if (shared != -1)
        shmctl(shared, IPC_RMID, NULL);
    exit(0);
}

void exitChildHandler(int sig) {
    if (shmdt(cmsgid) == -1)
        printError("Failed to detach shared memory.");
    exit(0);
}

void asyncRead() {
    msg_send_message msg;
    while (1) {
        if (*cmsgid > 0) {
            if (msgrcv(*cmsgid, &msg, sizeof(msg_send_message), -10, IPC_NOWAIT) != -1) {
                if (isBlocked(msg.author))
                    continue;
                printf("> %s@%s said: %s\n", msg.author, msg.room_name, msg.message);
            }
        }
    }
    shmdt(cmsgid);
}

pid_t turnAsyncRead() {
    pid_t pid = fork();
    if (pid == 0) {
        signal(SIGKILL, exitChildHandler);
        printf("Async read started.\n");
        asyncRead();
        exit(0);
    }
    return pid;
}

int main(int argc, char const *argv[]) {
    printf("Welcome to Message Queue IPC Client\n");
    signal(SIGINT, exitHandler);
    shared = shmget(IPC_PRIVATE, sizeof(cmsgid), 0666 | IPC_CREAT);
    if (shared == -1) {
        printError("Failed to create shared memory.");
        return 1;
    }
    cmsgid = (int *)shmat(shared, NULL, 0);
    if (cmsgid == (void *)-1) {
        printError("Failed to attach shared memory.");
        return 1;
    }
    pid_t pid = 0;
    while (1) {
        if (*cmsgid)
            printf("User channel: %d\n", *cmsgid);
        else
            printf("Plase login to connect to server.\n");
        printf("[l] login\t[s] send message\t[r] recive message(sync)\t[t] toogle read(async)\t[m] list rooms\t[j] join room\t[n] create new room\t[b] block user\t[o] logout\t[e] exit\n");
        char input;
        scanf(" %c", &input);
        switch (input) {
            case 'l':
                if (*cmsgid) {
                    printf("You are already logged in.\n");
                    break;
                }
                *cmsgid = login();
                break;
            case 'r':
                if (!*cmsgid) {
                    printf("You must be logged in to recive messages.\n");
                    break;
                }
                readMessage();
                break;
            case 't':
                if (pid) {
                    kill(pid, SIGKILL);
                    pid = 0;
                    printf("Async read stopped.\n");
                } else {
                    pid = turnAsyncRead();
                }
                break;
            case 'o':
                if (!*cmsgid) {
                    printf("You must be logged in to logout.\n");
                    break;
                }
                logout();
                *cmsgid = 0;
                break;
            case 's':
                if (!*cmsgid) {
                    printf("You must be logged in to send a message.\n");
                    break;
                }
                sendMessage();
                break;
            case 'j':
                if (!*cmsgid) {
                    printf("You must be logged in to join a room.\n");
                    break;
                }
                joinRoom();
                break;
            case 'n':
                if (!*cmsgid) {
                    printf("You must be logged in to create a room.\n");
                    break;
                }
                createRoom();
                break;
            case 'b':
                if (!*cmsgid) {
                    printf("You must be logged in to block a user.\n");
                    break;
                }
                blockUser();
                break;
            case 'm':
                if (!*cmsgid) {
                    printf("You must be logged in to list rooms.\n");
                    break;
                }
                listRooms();
                break;
            case 'e':
                if (pid)
                    kill(pid, SIGKILL);
                exitHandler(0);
                return 0;
            default:
                printf("Invalid input.\n");
                break;
        }
    }
    return 0;
}
