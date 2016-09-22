#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PACKET_SIZE 100
#define SERV_PORT 20403

#define ID_LEN 10
#define MSG_LEN 75
#define USER_NUM 100

#define CMTYPE_LOGIN_REQUEST '0'
#define CMTYPE_SEND_MESSAGE '1'

#define M_LOGIN_REJECT "0 0"
#define M_LOGIN_SUCCESS "0 1"
#define M_ACK_FAIL "2 0"
#define M_ACK_SUCCESS "2 1"

#define MAX_UNREAD 100

typedef enum {false, true} bool;
typedef struct {
    bool isActive;
    int fd;
    int port;
    int numUnread;
    char IP[12];
    char ID[ID_LEN+1];
    char unread[MAX_UNREAD][MSG_LEN+1];
} UserInfo;

void resetBuff();
void errLog(char *message);
int getIdxByID(char* ID);
int getIdxByFd(int fd);
void clearMsgAt(int userIdx, int msgIdx);
void setUserInfo(int idx, int fd, struct sockaddr_in clnt_addr);
bool connClose(int idx);
void sendUnread(int idx);
bool pushUnread(int idx, char* message);

char message[PACKET_SIZE*2];
UserInfo uInfoList[USER_NUM];

int main(int argc, char** argv){
    // 유저 정보 리스트를 초기화한다
    // 편의상 계정 100개를 미리 할당한다.
    for (int i = 0; i < USER_NUM; i++) {
        UserInfo* u = &uInfoList[i];
        u->isActive = false;
        u->fd = -1;
        u->port = -1;
        u->numUnread = 0;
        strncpy(u->IP, "", 0);
        // ID 형태는 u0, u1, ..., u100
        snprintf(u->ID, ID_LEN-1, "u%d", i);

        for (int j = 0; j < 100; j++){
            clearMsgAt(i, j);
        }
    }
    
    int serv_sock;
    struct sockaddr_in serv_addr;

    fd_set reads, temps;
    int fd_max;

    int str_len;
    struct timeval timeout;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
        errLog("bind() error");
    if (listen(serv_sock, 5) == -1)
        errLog("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while(1) {
        int idx;
        char type;

        int fd, str_len;
        int clnt_sock, clnt_len;
        struct sockaddr_in clnt_addr;

        temps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        if (select(fd_max+1, &temps, 0, 0, &timeout) == -1)
            errLog("select() error");

        for (fd = 0; fd < fd_max+1; fd++) {
            // 파일 디스크립터를 순회하며 수신할 데이터가 있는지 확인한다.
            if (FD_ISSET(fd, &temps)){
                if (fd == serv_sock){
                    // 연결 시도가 있으면 일단 연결만을 한다.
                    // 로그인 요청 메시지를 받기 전까지는 active되지 않은 것으로 본다.
                    clnt_len = sizeof(clnt_addr);
                    clnt_sock = accept(serv_sock,
                    (struct sockaddr *)&clnt_addr, 
                    (socklen_t *)&clnt_len);
                    FD_SET(clnt_sock, &reads);
                    if(fd_max < clnt_sock)
                        fd_max = clnt_sock;
                    printf("클라이언트 연결: fd-%d | IP-%s | port-%d\n", 
                    clnt_sock, inet_ntoa(clnt_addr.sin_addr), clnt_addr.sin_port);
                } else {
                    // 메시지가 도착하면 0 또는 PACKET_SIZE만큼의 메시지인지 확인한다.
                    // 이외의 경우 패킷이 잘려 도착한 것으로,
                    // 나머지 패킷이 도착할 때까지 읽지 않고 기다린다.
                    str_len = recv(fd, message, PACKET_SIZE, 
                            MSG_PEEK|MSG_DONTWAIT);
                    printf("in buffer[%d](%d): ", fd, str_len);
                    if (str_len != 0 && str_len < PACKET_SIZE){
                        printf("패킷의 남은 부분을 기다립니다\n");
                        continue;
                    }
                    printf("패킷을 성공적으로 받아왔습니다\n");
                    str_len = read(fd, message, PACKET_SIZE);
                    if (str_len == 0){
                        // 길이가 0인 경우 종료를 요청하는 메시지이다.
                        FD_CLR(fd, &reads);
                        close(fd);
                        
                        // 종료된 클라이언트의 정보를 리셋해준다.
                        idx = getIdxByFd(fd);
                        if (connClose(idx))
                            printf("exit[%d] %s\n", fd, uInfoList[idx].ID);
                    } else {
                        // 통상의 메시지 PACKET 처리
                        // 패킷의 맨 앞 숫자를 읽어서 어떤 종류의 패킷인지 확인한다.
                        type = message[0];
                        printf("[type%c] '%s'\n", type, message);
                        switch (type) {
                            case (CMTYPE_LOGIN_REQUEST):
                                // 1. 로그인 요청 처리
                                // 1-1. 유저 정보 조회
                                idx = getIdxByID(&message[2]);
                                // 1-2. 로그인 허가/거부
                                if (idx < 0) {
                                   write(fd, M_LOGIN_REJECT, PACKET_SIZE);
                                   printf("login reject: %s\n", &message[2]);
                                   break;
                                } else {
                                   write(fd, M_LOGIN_SUCCESS, PACKET_SIZE); 
                                   printf("login success: %s\n", &message[2]);
                                }
                                // 1-3. 유저 정보 갱신
                                setUserInfo(idx, fd, clnt_addr);
                                // 1-4. 부재중 매시지 전송
                                sendUnread(idx);

                               break;
                            case (CMTYPE_SEND_MESSAGE):
                                // 2. 메시지 전달 요청 처리
                                // 2-1. 수신자 확인 후 ACK
                                idx = getIdxByID(&message[2]);
                                printf("idx: %d, to: %s, act: %d\n", idx, uInfoList[idx].ID, uInfoList[idx].isActive);
                                if (idx < 0){
                                    // 없는 유저에게 보낸 경우
                                    write(fd, M_ACK_FAIL, PACKET_SIZE);
                                    break;
                                } else {
                                    // 올바른 유저에게 보낸 경우
                                    write(fd, M_ACK_SUCCESS, PACKET_SIZE);
                                }
                                if (uInfoList[idx].isActive){
                                    // 2-2a. 수신자 Active: 즉시 전송
                                    // fd 번호 찾아다가 -> 바로 write()
                                    printf("send msg immediately\n");
                                    int receiver_fd = uInfoList[idx].fd;
                                    write(receiver_fd, message, PACKET_SIZE);
                                } else {
                                    // 2-2b. 수신자 Deactive: unread에 추가/numUnread + 1
                                    printf("pushUnread\n");
                                    pushUnread(idx, message);
                                }
                                break;
                            default:
                                errLog("wrong type");
                        }
                    }
                }
            } // if
            resetBuff(); // 한 번 읽어오고 난 후에는 버퍼를 초기화한다.
        } // for
    } // while
    return 0;
} // main

void resetBuff(){
    for (int i = 0; i <= PACKET_SIZE; i++)
        message[i] = '\0';
}

void errLog(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int getIdxByID(char* ID){
    printf("ID: '%s'\n", ID);
    // 없는 아이디인 경우 -1 리턴함

    for (int i = 0; i < USER_NUM; i++) {
        if (strncmp(ID, uInfoList[i].ID, strlen(uInfoList[i].ID)) == 0) {
            return i;
        }
    }
    return -1;
}

int getIdxByFd(int fd){
    int ret = -1;
    // 없는 fd 번호인 경우 -1 리턴함

    for (int i = 0; i < USER_NUM; i++) {
        if (uInfoList[i].fd == fd) {
            ret = i;
            break;
        }
    }
    return ret;

}

void clearMsgAt(int userIdx, int msgIdx){
    for (int i = 0; i <= MSG_LEN; i++)
        uInfoList[userIdx].unread[msgIdx][i] = '\0';
}
void setUserInfo(int idx, int fd, struct sockaddr_in clnt_addr) {
    uInfoList[idx].isActive = true;
    uInfoList[idx].fd = fd;
    uInfoList[idx].port = clnt_addr.sin_port;
    strcpy(uInfoList[idx].IP, inet_ntoa(clnt_addr.sin_addr));
}

bool connClose(int idx){
    // 접속을 종료한 유저의 정보를 갱신한다.
    if (idx < 0)
        return false;
    uInfoList[idx].fd = -1;
    uInfoList[idx].isActive = false;
    return true;
}

void sendUnread(int idx) {
    int num = uInfoList[idx].numUnread;
    for (int i = 0; i < num; i++) {
        write(uInfoList[idx].fd, uInfoList[idx].unread[i], PACKET_SIZE);
        printf("send unread: '%s'", uInfoList[idx].unread[i]);
    }

}

bool pushUnread(int idx, char* message) {
    if(uInfoList[idx].numUnread > MAX_UNREAD-1)
        return false;
    UserInfo* u = &uInfoList[idx];
    strcpy(u->unread[u->numUnread], message);
    printf("push unread: '%s'", u->unread[u->numUnread]);
    u->numUnread = u->numUnread + 1;
    return true;
}
