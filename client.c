#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#define PACKET_SIZE 100
#define SERV_IP "127.0.0.1"
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

#define MENU_READ 1
#define MENU_WRITE 2
#define MENU_QUIT 3

typedef enum {false, true} bool;

char ID[ID_LEN];
char message[PACKET_SIZE+1], buf[PACKET_SIZE+1];

void readline(char* line, int size);
void resetBuff(char* buf);
void getLoginPacket(char* buf);
void getMsgPacket(char* buf);
void printMainMenu();
void enqueue(char* msg);
void dequeue();
char* displayMsg(char* msg);

int queue_size;
char queue[50][PACKET_SIZE];


int main(int argc, char **argv) {

    int sock_fd = 0;
    struct sockaddr_in server_addr;
    int message_len = 0;

    sock_fd = socket(PF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    server_addr.sin_port = htons(20403);

    connect(sock_fd, (struct sockaddr*) &server_addr,
        sizeof(server_addr));
    
    while(1) {
        // 로그인을 하기 위한 루프
        getLoginPacket(message); // 로그인 화면을 출력하고 아이디를 받는다.
        write(sock_fd, message, PACKET_SIZE);
        resetBuff(message); // 서버에 패킷 전송 후

        read(sock_fd, message, PACKET_SIZE); // 서버의 확인 메시지를 받는다.
        if (!strncmp(message, M_LOGIN_SUCCESS, strlen(M_LOGIN_SUCCESS))){
            printf("logon success.\n");
            resetBuff(message);
            break;
        }
        printf("logon fail. try again.\n");
        resetBuff(message);
    }

    while(1) {
        resetBuff(message);
        int op;
        printMainMenu();
        scanf("%d", &op);
        while('\n'!=getchar());
        switch (op) {
            case MENU_READ:
                printf("\n< 도착한 메시지 목록 >\n");
                dequeue();
                while(recv(sock_fd, message, PACKET_SIZE, MSG_PEEK|MSG_DONTWAIT)>= PACKET_SIZE){
                    // 패킷 크기보다 더 큰 메시지가 쌓여있으면 메시지를 읽어온다.
                    read(sock_fd, message, PACKET_SIZE);
                    printf("돈다뿅%s\n", displayMsg(message));
                }
                printf("\n# 메인 메뉴로 돌아가려면 아무 키나 누르십시오:");
                if(getchar())
                    break;
                break;
            case MENU_WRITE:
                getMsgPacket(message); // 전송 화면을 출력하고, 입력을 받아 message로 패킷을 저장한다.
                write(sock_fd, message, PACKET_SIZE); // 서버에 전송한 뒤
                resetBuff(message);

                message_len = read(sock_fd, message, PACKET_SIZE); // ACK를 받아 전송 성공 여부를 받는다.
                if (!strncmp(message, M_ACK_SUCCESS, strlen(M_ACK_SUCCESS)))
                    printf("-- 전송에 성공하였습니다 --\n");
                else{
                    while(message[0] != '2'){
                       enqueue(message);
                       read(sock_fd, message, PACKET_SIZE); // ACK를 받아 전송 성공 여부를 받는다.
                    }
                    if (!strncmp(message, M_ACK_SUCCESS, strlen(M_ACK_SUCCESS)))
                        printf("-- 전송에 성공하였습니다 --\n");
                    else printf("-- [!]유효한 발신자가 아닙니다 --\n");
                }
                break;
            case MENU_QUIT:
                // 연결 종료
                close(sock_fd);
                return 0;
                break;
            default:
                printf("# 1, 2, 3 중 하나를 입력해주십시오.\n");
                break;
        }
    }
    return 0;
}

void readline(char* line, int size) {
    fgets(line, size, stdin);

    if (strlen(line) < size) {
        line[strlen(line)-1] = '\0';
    } else {
        line[size-1] = '\0';
        while('\n' == getchar());
    }
}

void resetBuff(char* buf) {
    for (int i = 0; i < PACKET_SIZE+1; i++)
        buf[i] = '\0';
}

void printMainMenu() {
    printf("--------------------\n");
    printf("1. Read Message\n");
    printf("2. Send Message\n");
    printf("3. Quit\n");
    printf("--------------------\n");
    printf("# 원하는 메뉴의 번호를 입력해주세요: ");
}

void getLoginPacket(char* buf) {
    printf("client ID: ");
    readline(ID, ID_LEN);

    buf[0] = CMTYPE_LOGIN_REQUEST;
    strncpy(buf+2, ID, ID_LEN);

    // 중간에 있는 '\0'는 모두 공백으로 대체한다.
    int end = 2 + strlen(ID);
    for (int i = 0; i < end; i++) {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }
}

void getMsgPacket(char* buf) {
    // buf에 msg를 보내기 위한 packet을 넣어준다.
    buf[0] = CMTYPE_SEND_MESSAGE;

    char temp[PACKET_SIZE];
    printf("R: ");
    readline(temp, ID_LEN);
    strncpy(buf+2, temp, ID_LEN);
    resetBuff(temp);

    strncpy(buf+(4+ID_LEN), ID, ID_LEN);
    
    printf("M: ");
    readline(temp, MSG_LEN);
    strncpy(buf+(6+ID_LEN*2), temp, MSG_LEN);


    // 중간에 있는 '\0'는 모두 공백으로 대체한다.
    int end = 6 + ID_LEN*2 + strlen(&buf[5+ID_LEN*2]);
    for (int i = 0; i < end; i++) {
        if (buf[i] == '\0')
            buf[i] = ' ';
    }
}
void enqueue(char* msg){
    strncpy(queue[queue_size++], msg, PACKET_SIZE);
}
void dequeue(){
    for(int i = 0; i < queue_size; i++) {
        printf("죽어라 젠장 디큐%s\n", displayMsg(queue[i]));
    }
    queue_size = 0;
}

char* displayMsg(char* msg){
    char* temp = (char*) malloc(PACKET_SIZE);

    /* 송신자가 본인임이 확실하기 때문에 띄워주지 않아도 된다

     메시지 패킷 형태:
     123 1234567890 1234567890 MSG (16 + MSG_LEN + ID_LEN)
     201 receiver   sender     MSG
    
     띄워줄 형태:
     12345678912345678901234567MSG (16 + MSG_LEN + ID_LEN)
     sender:  ID         msg: MSG
     */

    // temp에 띄울 메시지 만들기
    strcpy(temp, "sender:  ");
    strncpy(temp+9, msg+(5+ID_LEN), ID_LEN); // ID 복사
    strcpy(temp+9+ID_LEN, " msg:  ");
    strncpy(temp+16+ID_LEN, msg+(6+ID_LEN*2), MSG_LEN);

    // msg에 temp를 덮어씌우기
    strncpy(msg, temp, PACKET_SIZE);
    free(temp);
    return msg;
}
