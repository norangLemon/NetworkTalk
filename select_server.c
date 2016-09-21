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
char message[PACKET_SIZE+1];

void resetBuff(){
    for (int i = 0; i < PACKET_SIZE+1; i++)
        message[i] = '\0';
}
void error_handling(char* message);
struct User_info;

int main(int argc, char** argv){
    int serv_sock;
    struct sockaddr_in serv_addr; // 주소 정보를 나타내는 구조체 변수

    fd_set reads, temps;
        int fd_max;

    int str_len;
    struct timeval timeout;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if (bind(serv_sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)))
    error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
    error_handling("listen() error");

    FD_ZERO(&reads);
    FD_SET(serv_sock, &reads);
    fd_max = serv_sock;

    while(1) {
        int fd, str_len;
        int clnt_sock, clnt_len;
        struct sockaddr_in clnt_addr;

        temps = reads;
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        if (select(fd_max+1, &temps, 0, 0, &timeout) == -1)
            error_handling("select() error");

        for (fd = 0; fd < fd_max+1; fd++) {
            if (FD_ISSET(fd, &temps)){
                if (fd == serv_sock){
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
                    str_len = recv(fd, message, PACKET_SIZE, 
                            MSG_PEEK|MSG_DONTWAIT);
                    printf("in buffer[%d](%d)\n", fd, str_len);
                    str_len = read(fd, message, PACKET_SIZE);
                    if (str_len == 0){
                        FD_CLR(fd, &reads);
                        close(fd);
                        printf("exit[%d]%s(%d)\n", fd, message, str_len);
                        printf("클라이언트 종료: fd-%d\n", fd);
                    } else {
                        printf("msg[%d]%s(%d)\n", fd, message, str_len);
                        write(fd, message, str_len);
                    }
                }
            } // if
            resetBuff(); // 한 번 읽어오고 난 후에는 버퍼를 초기화한다.
        } // for
    } // while
    return 0;
} // main

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}
