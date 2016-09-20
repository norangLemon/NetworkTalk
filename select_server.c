#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 100

void error_handling(char* message);

int main(int argc, char** argv){
   int serv_sock;
   struct sockaddr_in serv_addr;
   
   fd_set reads, temps;
   int fd_max;

   char message[BUFSIZE];
   int str_len;
   struct timeval timeout;
   if (argc!= 2){
      printf("Usage: %s <port>\n", argv[0]);
      exit(1);
   }

   serv_sock = socket(PF_INET, SOCK_STREAM, 0);
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   serv_addr.sin_port = htons(atoi(argv[1]));

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

      if(select(fd_max+1, &temps, 0, 0, &timeout) == -1)
         error_handling("select() error");

      for(fd = 0; fd < fd_max+1; fd++){
         if (FD_ISSET(fd, &temps)){
            if (fd == serv_sock){
               clnt_len = sizeof(clnt_addr);
               clnt_sock = accept(serv_sock,
                  (struct sockaddr *)&clnt_addr, 
                  (socklen_t *)&clnt_len);
               FD_SET(clnt_sock, &reads);
               if(fd_max < clnt_sock)
                  fd_max = clnt_sock;
               printf("클라이언트 연결: 파일 디스크립터 %d\n", clnt_sock);
            } else {
               str_len = read(fd, message, BUFSIZE);
               if (str_len == 0){
                  FD_CLR(fd, &reads);
                  close(fd);
                  printf("클라이언트 종료: 파일 디스크립터 %d\n", fd);
               } else {
                  write(fd, message, str_len);
               }
            }
         } // if
      } // for
   } // while
   return 0;
} // main

void error_handling(char *message){
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}
