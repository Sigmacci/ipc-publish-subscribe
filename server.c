#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mqipc.h"

// Message Queue
#define RESPONSE_QUEUE_SIZE 16
// Database
#define DATABASE_DIR "database"
#define USERS_DB "database/users.db"
#define ROOMS_DB "database/rooms.db"
#define KEYS_DB "database/keys.db"
#define TEMP_DB "database/temp.db"

void printError(char *msg) {
    fprintf(stderr, "%s\nError: %s\n", msg, strerror(errno));
}

void dbInit() {
    // check if database directory exists
    struct stat st = {0};
    if (stat(DATABASE_DIR, &st) == -1) {
        // create database directory
        mkdir(DATABASE_DIR, 0755);
    }
    // create users database
    FILE *users = fopen(USERS_DB, "a");
    if (!users) {
        printError("Failed to create users database.");
        exit(1);
    }
    fclose(users);
    // crate rooms database
    FILE *rooms = fopen(ROOMS_DB, "a");
    if (!rooms) {
        printError("Failed to create rooms database.");
        exit(1);
    }
    fclose(rooms);
    // create keys database
    FILE *keys = fopen(KEYS_DB, "a");
    if (!keys) {
        printError("Failed to create keys database.");
        exit(1);
    }
    fclose(keys);
}

int dbUserExists(char *username) {
    // open users database
    FILE *users = fopen(USERS_DB, "r");
    if (!users) {
        printError("Failed to open users database.");
        exit(1);
    }
    // read database line by line
    char name[32];
    int id = 0, cmsgid;
    while (fscanf(users, "%d %s %d", &id, name, &cmsgid) == 3) {
        if (strcmp(name, username) == 0) {
            fclose(users);
            return cmsgid;  // User exists
        }
    }
    fclose(users);
    // username does not exist
    return -(++id);
}

int dbEditUser(char *username, int cmsgid) {
    // open users database
    FILE *users = fopen(USERS_DB, "r");
    if (!users) {
        printError("Failed to open users database.");
        exit(1);
    }
    // open temp database
    FILE *temp = fopen(TEMP_DB, "w");
    if (!temp) {
        printError("Failed to open temp database.");
        exit(1);
    }
    // read database line by line
    char name[32];
    int tmp, key, edit = 0;
    while (fscanf(users, "%d %s %d", &tmp, name, &key) == 3) {
        if (strcmp(name, username) == 0) {
            fprintf(temp, "%d %s %d\n", tmp, name, cmsgid);
            edit = 1;
        } else {
            fprintf(temp, "%d %s %d\n", tmp, name, key);
        }
    }
    fclose(users);
    fclose(temp);
    if (edit) {
        // remove users database
        remove(USERS_DB);
        // rename temp database
        rename(TEMP_DB, USERS_DB);
        return 0;  // User exists
    }
    return 1;  // User does not exist
}

int dbAddUser(char *username, int cmsgid) {
    int tmp = dbUserExists(username);
    if (tmp > 0) {
        return tmp;
    }
    if (tmp == 0) {
        return dbEditUser(username, cmsgid);
    }
    // open users database
    FILE *users = fopen(USERS_DB, "a");
    if (!users) {
        printError("Failed to open users database.");
        exit(1);
    }
    fprintf(users, "%d %s %d\n", -tmp, username, cmsgid);
    // close users database
    fclose(users);
    return 0;
}

void dbRemoveUser(int cmsgid) {
    // open users database
    FILE *users = fopen(USERS_DB, "r");
    if (!users) {
        printError("Failed to open users database.");
        exit(1);
    }
    // open temp database
    FILE *temp = fopen(TEMP_DB, "w");
    if (!temp) {
        printError("Failed to open temp database.");
        exit(1);
    }
    // read database line by line
    char name[32];
    int id, key, userExists = 0;
    while (fscanf(users, "%d %s %d", &id, name, &key) == 3) {
        if (key == cmsgid) {
            fprintf(temp, "%d %s %d\n", id, name, 0);
            userExists = 1;
        } else
            fprintf(temp, "%d %s %d\n", id, name, key);
    }
    fclose(users);
    fclose(temp);
    if (userExists) {
        // remove users database
        remove(USERS_DB);
        // rename temp database
        rename(TEMP_DB, USERS_DB);
    }
}

int dbRoomExists(char *room_name) {
    // open rooms database
    FILE *rooms = fopen(ROOMS_DB, "r");
    if (!rooms) {
        printError("Failed to open rooms database.");
        exit(1);
    }
    // read database line by line
    char name[32];
    int id = 0;
    while (fscanf(rooms, "%d %s", &id, name) == 2) {
        if (strcmp(name, room_name) == 0) {
            fclose(rooms);
            return id;  // Room exists
        }
    }
    // close rooms database
    fclose(rooms);
    return -(++id);
}

int dbAddRoom(char *room_name) {
    int tmp = dbRoomExists(room_name);
    if (tmp > 0) {
        return tmp;
    }
    // open rooms database
    FILE *rooms = fopen(ROOMS_DB, "a");
    if (!rooms) {
        printError("Failed to open rooms database.");
        exit(1);
    }
    fprintf(rooms, "%d %s\n", -tmp, room_name);
    // close rooms database
    fclose(rooms);
    return 0;
}

int dbEditRoom(int roomid, int cmsgid, int sub) {
    // open keys database
    FILE *keys = fopen(KEYS_DB, "r");
    if (!keys) {
        printError("Failed to open keys database.");
        exit(1);
    }
    // open temp database
    FILE *temp = fopen(TEMP_DB, "w");
    if (!temp) {
        printError("Failed to open temp database.");
        exit(1);
    }
    // read database line by line
    int id, msgid, subscribtion, edit = 0;
    while (fscanf(keys, "%d %d %d", &id, &msgid, &subscribtion) == 3) {
        if (id == roomid && msgid == cmsgid) {
            fprintf(temp, "%d %d %d\n", id, msgid, sub);
            edit = 1;
        } else {
            fprintf(temp, "%d %d %d\n", id, msgid, subscribtion);
        }
    }
    fclose(keys);
    fclose(temp);
    if (edit) {
        // remove keys database
        remove(KEYS_DB);
        // rename temp database
        rename(TEMP_DB, KEYS_DB);
        return 0;  // User is in room
    }
    return 1;  // User is not in room
}

int dbJoinRoom(char *room_name, int cmsgid, int subscribtion) {
    // find room id
    int key = dbRoomExists(room_name);
    if (key < 0) {
        return 1;  // Room does not exist
    }
    // edit room if user is already in it
    if (!dbEditRoom(key, cmsgid, subscribtion)) {
        return 2;  // User is in room
    }
    // open keys database
    FILE *keys = fopen(KEYS_DB, "a");
    if (!keys) {
        printError("Failed to open keys database.");
        exit(1);
    }
    fprintf(keys, "%d %d %d\n", key, cmsgid, subscribtion);
    // close keys database
    fclose(keys);
    return 0;
}

int gid = 0;
void exitHandler(int sig) {
    printf("Server shutting down.\n");
    msgctl(gid, IPC_RMID, 0);
    exit(0);
}

int main(int argc, char const *argv[]) {
    printf("Welcome to Message Queue IPC Server\n");
    printf("CTRL+C to exit.\n");
    // initialize database
    dbInit();
    // create server queue
    int msgid = msgget(MQIPC_SERVER, 0666 | IPC_CREAT);
    // register exit handler
    signal(SIGINT, exitHandler);
    gid = msgid;
    // check for errors
    if (msgid == -1) {
        printError("Failed to create server queue.");
        return 1;
    }
    printf("Server started at %d\n", msgid);
    // listen for messages
    int listen = 1;
    msg_login msg_temp_login;
    msg_create_room msg_temp_create_room;
    msg_logout msg_temp_logout;
    msg_list_rooms msg_temp_list_rooms;
    msg_join_room msg_temp_join_room;
    while (listen) {
        // check for logout messages
        if (msgrcv(msgid, &msg_temp_logout, sizeof(msg_temp_logout), M_LOGOUT, IPC_NOWAIT) != -1) {
            printf("Received logout message from user: #%d\n", msg_temp_logout.cmsgid);
            // delete user from database
            dbRemoveUser(msg_temp_logout.cmsgid);
        }
        // check for login messages
        if (msgrcv(msgid, &msg_temp_login, sizeof(msg_temp_login), M_LOGIN, IPC_NOWAIT) != -1) {
            printf("Received login message from user: %s #%d\n", msg_temp_login.username, msg_temp_login.cmsgid);
            // add user to database
            int id = dbAddUser(msg_temp_login.username, msg_temp_login.cmsgid);
            // define response
            msg_response response;
            // check if user exists
            if (id != msg_temp_login.cmsgid && id != 0) {
                response.status = M_FAIL;
                response.mtype = M_RESPONSE;
                strcpy(response.message, "Username is taken.");
            } else {
                response.status = M_SUCCESS;
                response.mtype = M_RESPONSE;
                strcpy(response.message, "Login successful.");
            }
            printf("Sending response to: %d\n", msg_temp_login.cmsgid);
            if (msgsnd(msg_temp_login.cmsgid, &response, sizeof(msg_response), IPC_NOWAIT) == -1) {
                printError("Failed to send login response.");
            }
        }
        // check for create room messages
        if (msgrcv(msgid, &msg_temp_create_room, sizeof(msg_temp_create_room), M_CREATE_ROOM, IPC_NOWAIT) != -1) {
            printf("Received create room message from user: #%d\n", msg_temp_create_room.cmsgid);
            int id = dbAddRoom(msg_temp_create_room.room_name);
            msg_response response;
            if (id) {
                // room exists
                response.status = M_FAIL;
                response.mtype = M_RESPONSE;
                strcpy(response.message, "Room name is taken.");
            } else {
                // room created
                response.status = M_SUCCESS;
                response.mtype = M_RESPONSE;
                strcpy(response.message, "Room created.");
            }
            printf("Sending response to: %d\n", msg_temp_create_room.cmsgid);
            if (msgsnd(msg_temp_create_room.cmsgid, &response, sizeof(msg_response), IPC_NOWAIT) == -1) {
                printError("Failed to send create room response.");
            }
        }
        // check for list rooms messages
        if (msgrcv(msgid, &msg_temp_list_rooms, sizeof(msg_temp_list_rooms), M_LIST_ROOMS, IPC_NOWAIT) != -1) {
            printf("Received list rooms message from user: #%d\n", msg_temp_list_rooms.cmsgid);
            // open rooms database
            FILE *rooms = fopen(ROOMS_DB, "r");
            if (!rooms) {
                printError("Failed to open rooms database.");
                exit(1);
            }
            // read database line by line
            char name[32];
            int id = 0, key = MQIPC_RESPONSE_TEXT_SIZE / 32;
            // define response
            msg_response response;
            response.status = M_SUCCESS;
            response.mtype = M_RESPONSE;
            strcpy(response.message, "");
            while (fscanf(rooms, "%d %s", &id, name) == 2) {
                strcat(response.message, name);
                if (!(--key)) {
                    response.status = M_MORE;
                    printf("Sending response to: %d\n", msg_temp_list_rooms.cmsgid);
                    if (msgsnd(msg_temp_list_rooms.cmsgid, &response, sizeof(msg_response), IPC_NOWAIT) == -1) {
                        printError("Failed to send list rooms response.");
                    }
                    key = MQIPC_RESPONSE_TEXT_SIZE / 32;
                    strcpy(response.message, "");
                    continue;
                }
                strcat(response.message, " ");
            }
            response.status = M_SUCCESS;
            printf("Sending response to: %d\n", msg_temp_list_rooms.cmsgid);
            if (msgsnd(msg_temp_list_rooms.cmsgid, &response, sizeof(msg_response), IPC_NOWAIT) == -1) {
                printError("Failed to send list rooms response.");
            }
            // close rooms database
            fclose(rooms);
        }
        // check for join room messages
        if (msgrcv(msgid, &msg_temp_join_room, sizeof(msg_temp_join_room), M_JOIN_ROOM, IPC_NOWAIT) != -1) {
            printf("Received join room message from user: #%d\n", msg_temp_join_room.cmsgid);
            int id = dbJoinRoom(msg_temp_join_room.room_name, msg_temp_join_room.cmsgid, msg_temp_join_room.subscribtion);
            msg_response response;
            switch (id) {
                case 1:
                    response.status = M_FAIL;
                    response.mtype = M_RESPONSE;
                    strcpy(response.message, "Room does not exist.");
                    break;
                case 2:
                    response.status = M_SUCCESS;
                    response.mtype = M_RESPONSE;
                    strcpy(response.message, "Changed room subscribtion.");
                    break;
                default:
                    response.status = M_SUCCESS;
                    response.mtype = M_RESPONSE;
                    strcpy(response.message, "Room joined.");
                    break;
            }
            printf("Sending response to: %d\n", msg_temp_join_room.cmsgid);
            if (msgsnd(msg_temp_join_room.cmsgid, &response, sizeof(msg_response), IPC_NOWAIT) == -1) {
                printError("Failed to send join room response.");
            }
        }
    }
    return 0;
}
