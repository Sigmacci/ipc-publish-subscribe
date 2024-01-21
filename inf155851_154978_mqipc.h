#ifndef MQIPC_H
#define MQIPC_H

#define MQIPC_SERVER 1337
#define MQIPC_MESSAGE_SIZE 256

typedef struct msg_response {
    long mtype;
    char message[MQIPC_MESSAGE_SIZE];
    int status;
} msg_response;

typedef struct msg_login {
    long mtype;
    int cmsgid;         // client cmsgid
    char username[32];  // client username
} msg_login;

typedef struct msg_logout {
    long mtype;
    int cmsgid;  // client cmsgid
} msg_logout;

typedef struct msg_create_room {
    long mtype;
    int cmsgid;          // client cmsgid
    char room_name[32];  // room name
} msg_create_room;

typedef struct msg_list_rooms {
    long mtype;
    int cmsgid;  // client cmsgid
} msg_list_rooms;

typedef struct msg_join_room {
    long mtype;
    int cmsgid;          // client cmsgid
    char username[32];   // client username
    char room_name[32];  // room name
    int subscribtion;    // -1 infinite, 0 none, >0 number of messages
} msg_join_room;

typedef struct msg_send_message {
    long mtype;
    int cmsgid;                        // client cmsgid
    char author[32];                   // author    (username)
    char room_name[32];                // room name
    char message[MQIPC_MESSAGE_SIZE];  // message
    int priority;                      // priority
} msg_send_message;

enum msg_response_status {
    M_SUCCESS = 0,
    M_FAIL = 1,
    M_MORE = 2,
};

enum msg_type {
    M_RESPONSE = 1,
    M_LOGIN = 2,
    M_LOGOUT = 3,
    M_CREATE_ROOM = 4,
    M_LIST_ROOMS = 5,
    M_JOIN_ROOM = 6,
    M_SEND_MESSAGE = 7,
    M_RECIEVE_MESSAGE = 8,
};

#endif  // !MQIPC_H