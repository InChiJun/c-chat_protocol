#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFSIZE 100
#define NAMESIZE 20

int StoHEX(char fi, char sc);
void error_handling(char* msg);
void* send_message(void* arg);
void* recv_message(void* arg);

char message[BUFFSIZE];

void* rcv(void* arg){
    printf("rcv thread created!\n");
    
    int sock = (int)arg;
    char buff[500];
    int len;

    while(1){
        len = read(sock, buff, sizeof(buff));

        if(len == -1){
            printf("sock close\n");
            break;
        }

        printf("%s", buff);
    }

    pthread_exit(0);
    return 0;
}

int main(int argc, char **argv){
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread; // **snd_thread는 따로 만들어야 함
    void* thread_result;

    char id[100];
    printf("argc: %d\n", argc); // 처음에 ID값 줬는지 확인하는 라인
    if (argc < 2){
        printf("you have to enter ID\n");
        return 0;
    }

    strcpy(id, argv[1]);
    printf("id: %s\n", id);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        printf("socked error\n");
    } else {
        printf("socket ok\n");
    }

    // Set N bytes of S to C.
    memset(&serv_addr, 0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serv_addr.sin_port=htons(7090);

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        printf("connect error\n");
    } else{
        printf("connection success\n");
    }

    pthread_create(&rcv_thread, NULL, rcv, (void*)sock);

    char chat[100];
    char msg[1000];

    // send 기능 시작 부분. 아래의 내용을 snd thread 만들어야 함
    printf("while before\n");
    while(1){
        printf("채팅 입력: "); // 덮어써지는 오류로 인해 해당 라인 삭제 예정
        gets(chat);

        sprintf(msg, "[%s]: %s\n", id, chat);
        printf("send: %s", msg);

        write(sock, msg, strlen(msg)+1);
        sleep(1);
    }
    printf("while end");

    close(sock);
    return 1;    
}//end main
