#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mqipc.h"

void printError(char *msg) {
    fprintf(stderr, "%s\nError: %s\n", msg, strerror(errno));
}

int login() {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
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
        return login.cmsgid;
    } else {
        msgctl(login.cmsgid, IPC_RMID, NULL);
    }
    return 0;
}

void logout(int cmsgid) {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
        return;
    }
    // create logout message
    msg_logout logout;
    logout.mtype = M_LOGOUT;
    logout.cmsgid = cmsgid;
    // send logout message
    msgsnd(msgid, &logout, sizeof(msg_logout), 0);
    // delete user queue
    msgctl(cmsgid, IPC_RMID, NULL);
    return;
}

void createRoom(int cmsgid) {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
        return;
    }
    // create new room message
    msg_create_room create_room;
    create_room.mtype = M_CREATE_ROOM;
    create_room.cmsgid = cmsgid;
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
    msgrcv(cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
}

void listRooms(int cmsgid) {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
        return;
    }
    // create list rooms message
    msg_list_rooms list_rooms;
    list_rooms.mtype = M_LIST_ROOMS;
    list_rooms.cmsgid = cmsgid;
    // send list rooms message
    msgsnd(msgid, &list_rooms, sizeof(msg_list_rooms), 0);
    // wait for server to respond
    msg_response response;
    while (1) {
        msgrcv(cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
        printf("Server response: %s\n", response.message);
        if (response.status != M_MORE) {
            break;
        }
    }
}

void joinRoom(int cmsgid) {
    // connect to server
    int msgid = msgget(MQIPC_SERVER, 0666);
    // check if connection was successful
    if (msgid == -1) {
        if (errno == ENOENT)
            printError("Server is not running.");
        else
            printError("Failed to connect to server.");
        return;
    }
    // create join room message
    msg_join_room join_room;
    join_room.mtype = M_JOIN_ROOM;
    join_room.cmsgid = cmsgid;
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
    // send join room message
    msgsnd(msgid, &join_room, sizeof(msg_join_room), 0);
    // wait for server to respond
    msg_response response;
    msgrcv(cmsgid, &response, sizeof(msg_response), M_RESPONSE, 0);
    // print server response
    printf("Server response: %s\n", response.message);
}

int cmsgid = 0;
void exitHandler(int sig) {
    if (cmsgid)
        logout(cmsgid);
    exit(0);
}

int main(int argc, char const *argv[]) {
    printf("Welcome to Message Queue IPC Client\n");
    signal(SIGINT, exitHandler);
    while (1) {
        if (cmsgid)
            printf("User channel: %d\n", cmsgid);
        else
            printf("Plase login to connect to server.\n");
        printf("[l] login\t[m] list rooms\t[j] join room\t[n] create new room\t[o] logout\t[e] exit\n");
        char input;
        scanf(" %c", &input);
        switch (input) {
            case 'l':
                if (cmsgid) {
                    printf("You are already logged in.\n");
                    break;
                }
                cmsgid = login();
                break;
            case 'o':
                if (!cmsgid) {
                    printf("You must be logged in to logout.\n");
                    break;
                }
                logout(cmsgid);
                cmsgid = 0;
                break;
            case 'j':
                if (!cmsgid) {
                    printf("You must be logged in to join a room.\n");
                    break;
                }
                joinRoom(cmsgid);
                break;
            case 'n':
                if (!cmsgid) {
                    printf("You must be logged in to create a room.\n");
                    break;
                }
                createRoom(cmsgid);
                break;
            case 'm':
                if (!cmsgid) {
                    printf("You must be logged in to list rooms.\n");
                    break;
                }
                listRooms(cmsgid);
                break;
            case 'e':
                if (cmsgid)
                    logout(cmsgid);
                return 0;
                break;
            default:
                printf("Invalid input.\n");
                break;
        }
    }
    return 0;
}
