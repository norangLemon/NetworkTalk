#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

/* server
 * – Message delivery: Receiving a message, the server should do either
 * • If the recipient is active, then send the message to the client immediately
 * • If not, store the message for later delivery
 * – Manage status (Activated or deactivated) of clients
 * • Also, should manage binding information of client ID and (IP address + Port number)
 * – Manage message queue for deactivated clients
 */

char IDs[20][100];
int IDcnt = 0;

int main (int argc, char **argv) {
    int sock_fd_server = 0;
    int sock_fd_client = 0;

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;

    int client_addr_size;

    char message[] = "Hello Client!\n";

    // 서버 소켓 생성 (TDP)
    sock_fd_server = socket(PF_INET, SOCK_STREAM, 0);

    memset(&server_addr, 0, sizeof(struct sockaddr_in));

    // TCP/IP 스택의 주소 집합으로 설정
    server_addr.sin_family = AF_INET;
    // 어떤 클라이언트 주소도 접근할 수 있음
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    // 서버의 포트를 20403번으로 설정
    server_addr.sin_port = htons(20403);

    // 서버 소켓에 주소 할당    
    bind(sock_fd_server, (struct sockaddr*) &server_addr,
        sizeof(struct sockaddr_in));
    // 서버 대기 상태로 설정
    listen(sock_fd_server, 5);

    client_addr_size = sizeof(struct sockaddr_in);

    // 클라이언트 요청을 수락하고, 클라이언트 소켓 할당
    sock_fd_client = accept(sock_fd_server,
        (struct sockaddr *) &client_addr,
        (socklen_t *) &client_addr_size);

    // 클라이언트에 데이터 전송
    write(sock_fd_client, message, sizeof(message));

    // read test
    int message_len = read(sock_fd_client, message, 1024 - 1);

    printf("Message from client: %s\n", message);
    // 클라이언트 소켓 종료
    close(sock_fd_client);
    // 서버 소켓 종료
    close(sock_fd_server);

    while (1) {  // main accept() loop
        client_addr_size = sizeof(struct sockaddr_in);
        // 클라이언트 요청을 수락하고, 클라이언트 소켓 할당
        sock_fd_client = accept(sock_fd_server, 
                (struct sockaddr *)&client_addr, 
                (socklen_t *)&client_addr_size);

        if (!fork()) { // this is the child process
            close(sock_fd_server); // child doesn't need the listener
            // 클라이언트에 데이터 전송
            send(sock_fd_client, "Hello, world!", 13, 0);

            close(sock_fd_client);
            exit(0);
        }
        close(sock_fd_client);  // parent doesn't need this
    }
    return 0;
}

