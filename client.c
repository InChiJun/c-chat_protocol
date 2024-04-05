#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFSIZE 200
#define NAMESIZE 20

void error_handling(char* msg);
void* send_message(void* arg);
void* recv_message(void* arg);

char message[BUFFSIZE];
char name[NAMESIZE];

void* recv_message(void* arg){
    printf("rcv thread created!\n");
    
    int sock = *((int*)arg);
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

void* send_message(void* arg)
{
    int sock = *((int *)arg);
    char chat[BUFFSIZE];

    printf("while before\n");
    while(1){
        fgets(chat, BUFFSIZE, stdin);

        printf("send: %s", chat);

        write(sock, chat, strlen(chat)+1);
        sleep(1);
    }
    printf("while end");
    return NULL;
}

void error_handling(char *msg)
{
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char **argv){
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void* thread_result;

    // printf("argc: %d\n", argc); // 처음에 name값 줬는지 확인하는 라인
    // if (argc < 2){
    //     printf("you have to enter name\n");
    //     return 0;
    // }
    do
    {
        printf("Please enter your name: ");
        fgets(name, NAMESIZE, stdin);
        name[strlen(name)-1] = '\0';
    } while(strstr(name, " ") != NULL);

    // strcpy(name, argv[1]);
    printf("USER name: %s\n", name);

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        error_handling("socked error");
    } else {
        printf("socket ok\n");
    }

    // Set N bytes of S to C.
    memset(&serv_addr, 0,sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    serv_addr.sin_port=htons(7090);

    if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect error");
    } else{
        printf("connection success\n");
        // char msg[NAMESIZE+4];
        // sprintf(msg, "[Entrance]: %s", name);
        write(sock, name, strlen(name)+1);
    }

    pthread_create(&rcv_thread, NULL, recv_message, (void*) &sock);
    pthread_create(&snd_thread, NULL, send_message, (void*) &sock);

    pthread_join(rcv_thread, NULL);
    pthread_join(snd_thread, NULL);

    close(sock);
    return 1;    
}//end main//