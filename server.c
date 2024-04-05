#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

// 전처리 문자
#define CLNT_MAX 10     // 클라이언트 최대 개수
#define BUFFSIZE 200    // 버퍼 최대 사이즈
#define NAMESIZE 20     // 이름 최대 사이즈
#define ROOM_MAX 5      // 1:1 채팅방 최대 개수

// 전역 변수
int g_clnt_socks[CLNT_MAX]; // client 소켓을 모아놓은 배열
int g_clnt_count = 0; // client 수
pthread_mutex_t g_mutex;    // mutex 키
int g_room_count = 0;   // room 개수
int g_room_state[ROOM_MAX] = {1,0,0,0,0};   // room의 사용자가 있는지 여부 확인용 배열 | 0 : 없음 | 1 : 있음 |
int g_room_person_count[ROOM_MAX] = {0, 0, 0, 0, 0};   // 방의 사람 수

// USER 정보 구조체
typedef struct _USER
{
    char name[NAMESIZE];    // USER 이름
    int room;               // USER 위치(현재 위치한 방 번호)
}USER;

//chatroom 정보 구조체
// typedef struct _ROOM
// {
//     int person_count;
// }ROOM;

// 구조체 배열들
USER user[CLNT_MAX];    // user 정보 구조체 배열

// 함수들
int findIndexBySocket(int my_sock){  // 자신의 인덱스를 socket 값으로 찾는 함수
    int index = -1;
    pthread_mutex_lock(&g_mutex);
    for(int i = 0; i < g_clnt_count; i++){
        if(my_sock == g_clnt_socks[i]){ // 클라이언트 소켓에 저장 되어있는 소켓이 자신의 소켓과 같은 소켓인지 찾아서 인덱스 알아내기
            index = i;
            break;
        }//end if
    }//end for
    pthread_mutex_unlock(&g_mutex);
    return index;  // 자신의 인덱스를 리턴
}

int findIndexByName(char* my_name){ // 자신의 인덱스를 name 값으로 찾는 함수
    int index = -1;
    pthread_mutex_lock(&g_mutex);
    for(int i = 0; i < g_clnt_count; i++){
        if(strncmp(my_name, user[i].name, strlen(user[i].name)) == 0){ // user.name에서 자신의 이름과 같은 이름을 찾아서 인덱스 알아내기
            index = i;
            break;
        }//end if
    }//end for
    pthread_mutex_unlock(&g_mutex);
    return index;    // 자신의 인덱스를 리턴
}

char* findNameBySocket(int my_sock){ // 자신의 이름을 socket 값으로 찾는 함수
    int i = findIndexBySocket(my_sock);
    return user[i].name;
}

// void send_all_clnt(char* msg, int my_sock){ // 자신을 제외한 모든 사용자에게 메시지를 전달하는 함수
//     char sendmsg[BUFFSIZE]; // 보낼 메시지 배열
//     sprintf(sendmsg, "[%s]: %s", findNameBySocket(my_sock), msg);    // 보낼 형식으로 sendmsg에 저장
    
//     pthread_mutex_lock(&g_mutex);
//     for(int i = 0; i < g_clnt_count; i++){ // g_clnt_count만큼 반복하기 때문에 send_all이 되는 것.
//         if(g_clnt_socks[i] != my_sock){ // 자신에게는 보내지 않기 위한 조건문
//             printf("send msg: %s", sendmsg);
//             write(g_clnt_socks[i], sendmsg, strlen(sendmsg)+1); // **send가 가능하면 send도 사용해보자.
//         }
//     }
//     pthread_mutex_unlock(&g_mutex);
// }

void send_clnt(char* msg, int my_sock, int my_room){ // 자신을 제외한 같은방 사용자에게 메시지를 전달하는 함수
    char sendmsg[BUFFSIZE]; // 보낼 메시지 배열
    sprintf(sendmsg, "[%s]: %s", findNameBySocket(my_sock), msg);    // 보낼 형식으로 sendmsg에 저장

    pthread_mutex_lock(&g_mutex);
    for(int i = 0; i < g_clnt_count; i++){ // g_clnt_count만큼 반복하기 때문에 send_all이 되는 것.
        if((g_clnt_socks[i] != my_sock) && (user[i].room == my_room)){ // 자신에게 보내지 않으면서 다른방 사람에게 보내지 않기 위한 조건문
            printf("send msg: %s", sendmsg);
            write(g_clnt_socks[i], sendmsg, strlen(sendmsg)+1); // **send가 가능하면 send도 사용해보자.
        }
    }
    pthread_mutex_unlock(&g_mutex);
}

void send_spec_clnt(char* msg, int my_sock){    // 귀속말 함수(특정 사용자에게만 전달하는 함수)
    char * tok[3];  // 보낸 메시지를 나누어서 저장할 배열 포인터 [0]: 명령어, [1]: 받을 사람, [2] : 보낼 내용

    tok[0] = strtok(msg, " ");
    for(int i = 1; i < 3; ++i){
        tok[i] = strtok(NULL, " ");
    }

    char sendmsg[BUFFSIZE]; // 보낼 메시지
    sprintf(sendmsg, "[%s]: %s", findNameBySocket(my_sock), tok[2]); // 보낼 형식으로 sendmsg에 저장

    pthread_mutex_lock(&g_mutex);
    for(int i = 0; i < g_clnt_count; i++){
        if(strcmp(user[i].name, tok[1]) == 0){
            printf("send spec msg: %s", msg);
            write(g_clnt_socks[i], sendmsg, strlen(sendmsg)+1);
        }
    }
    pthread_mutex_unlock(&g_mutex);
}

int roomCheck(){    // room 비어있는지 확인해서 비어있는 room 번호 알려주는 함수
    int room = -1;

    pthread_mutex_lock(&g_mutex);
    for(int i = 1; i < ROOM_MAX; ++i){
        if(g_room_state[i] == 0)
        {
            room = i;
            break;
        }
    }
    pthread_mutex_unlock(&g_mutex);
    return room;
}

void createRoom(char* msg, int my_sock){
    // 룸 번호 확인
    int room = roomCheck();
    if(room == -1)  // 빈 방이 없으면
    {
        write(my_sock, "There are no empty rooms", strlen("There are no empty rooms") + 1);
        return;
    }

    // msg에서 이름만 뽑아내기
    strtok(msg, " ");   // 명령어 부분이라서 버림
    char* name = strtok(NULL, " ");

    // 유저1과 유저2의 방 번호를 새로운 방번호로 교체
    pthread_mutex_lock(&g_mutex);
    g_room_state[room] = 1;
    ++g_room_count;
    g_room_person_count[room] += 2;
    pthread_mutex_unlock(&g_mutex);

    user[findIndexByName(name)].room = room;
    user[findIndexBySocket(my_sock)].room = room;
}

void exitRoom(int my_sock){
    int index = findIndexBySocket(my_sock);

    pthread_mutex_lock(&g_mutex);
    if(--g_room_person_count[user[index].room] == 0){   // 방의 마지막 사람이 나가면
        // 방 빼기
        g_room_state[user[index].room] = 0;
    }
    pthread_mutex_unlock(&g_mutex);

    user[index].room = 0;   // 단체 채팅방으로 이동
}

void handle_command(char* msg, int my_sock){    // 명령어를 다루는 함수
    char* msg_copy = (char*)malloc(strlen(msg) + 1);
    strcpy(msg_copy, msg);  // 내용 복사
    char* cmd;   // 명령어 저장 포인터
    cmd = strtok(msg_copy, " ");  // 명령어 저장

    if(strcmp(cmd, "/msg") == 0)    // 귓속말 명령어 형식: /msg [받는 사람] [보낼 문장]
    {
        send_spec_clnt(msg, my_sock);
    }
    else if(strcmp(cmd, "/privateChat") == 0)   // 1:1 채팅방을 만드는 명령어 형식: /privateRoom [초대 받는 사람]
    {
        createRoom(msg, my_sock);
    }
    else if(strncmp(cmd, "/exitroom", strlen("/exitroom")) == 0)  // 채팅방을 나가는 명령어 형식: /exitroom
    {
        exitRoom(my_sock);
    }
    else write(my_sock, "errorcode", strlen("errorcode") + 1);
    free(msg_copy); // 메모리 해제
}

void* clnt_connection(void* arg){ // 클라이언트 connect하는 start_routine
    int clnt_sock = *((int*)arg);
    int str_len = 0;
    char msg[BUFFSIZE];
    int my_num;
    
    // 본인 인덱스 찾는 부분
    for(int i = 0; i < g_clnt_count; i++){
        if(clnt_sock == g_clnt_socks[i]){
            my_num = i;
            break;
        }//end if
    }//end for

    while(1){
        str_len = read(clnt_sock, msg, sizeof(msg));
        if(str_len <= 0){
            printf("clnt[%d] close\n", clnt_sock);
            break;
        }
        if(strncmp(msg, "/", 1) == 0)
        {
            handle_command(msg, clnt_sock); // 명령어를 처리하는 함수
        }
        else send_clnt(msg, clnt_sock, user[my_num].room); // **소켓을 넣어서 보내는 이유를 파악해보자.
        printf("%s\n", msg); // **사용자 아이디도 있어야 하므로 버그 가능(수정 필요)
    }
    
    pthread_mutex_lock(&g_mutex);
    for(; my_num < g_clnt_count - 1; my_num++){
                g_clnt_socks[my_num] = g_clnt_socks[my_num+1];
    }
    g_clnt_count--;

    /*
    for(i = 0; i < g_clnt_count; i++){
        if(clnt_sock == g_clnt_socks[i]){
            for(; i < g_clnt_count - 1; i++){
                g_clnt_socks[i] = g_clnt_socks[i+1];
                break;
            }//end for
        }//end if
    }//end for
    */
    pthread_mutex_unlock(&g_mutex);
    close(clnt_sock);
    pthread_exit(0);
    
    return NULL;
}

/*
void* handle_chatroom(void* arg){
    ROOM room = *((ROOM*)arg);  // 유저 이름 2개 있다.
    int max_sd;
    fd_set readfds;
    char buffer[BUFFSIZE];

    int cli

    // 두 소켓 중 더 큰 값을 계산하여 select의 첫 번째 인자로 사용
    max_sd = max(client_socket1, client_socket2) + 1;
    
    while(1) {
        FD_ZERO(&readfds);
        
        // 소켓을 fd_set에 추가
        FD_SET(client_socket1, &readfds);
        FD_SET(client_socket2, &readfds);
        
        // 대기
        if(select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }
        
        // 클라이언트 1로부터 메시지 수신
        if(FD_ISSET(client_socket1, &readfds)) {
            memset(buffer, 0, BUF_SIZE);
            if(read(client_socket1, buffer, BUF_SIZE) > 0) {
                // 클라이언트 2로 메시지 전송
                write(client_socket2, buffer, strlen(buffer));
            }
        }
        
        // 클라이언트 2로부터 메시지 수신
        if(FD_ISSET(client_socket2, &readfds)) {
            memset(buffer, 0, BUF_SIZE);
            if(read(client_socket2, buffer, BUF_SIZE) > 0) {
                // 클라이언트 1로 메시지 전송
                write(client_socket1, buffer, strlen(buffer));
            }
        }
    }

    return NULL;
}
*/

// void create_chatroom(pthread_t* t_thread, char* user1_name, char* user2_name){   // 개인 채팅방 만드는 함수
//     pthread_mutex_lock(&g_mutex);
//     // 개인 채팅방 생성 로직을 구현하기
//     if(g_room_count == ROOM_MAX) return;
//     pthread_create(g_t_thread[g_room_count], NULL, handle_chatroom, (void*) &room[g_room_count]);
//     ++g_room_count;
//     pthread_mutex_unlock(&g_mutex);
//     return NULL;
// }

int main(int argc, char ** argv){
    int serv_sock;
    int clnt_sock;

    pthread_t t_thread;
    
    struct sockaddr_in clnt_addr;
    int clnt_addr_size;

    pthread_mutex_init(&g_mutex, NULL);

    struct sockaddr_in serv_addr;
    serv_sock = socket(PF_INET, SOCK_STREAM, 0); // socket 생성
    // PF_INET=IPv4, SOCK_STREAM=TCP, 0=1,2번째 인덱스에 따라 프로토콜이 결정됨(TCP)

    serv_addr.sin_family = AF_INET; // 소켓 bind 설정
    // AF_INET=address IPv4로 설정할 때 사용
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 소켓 bind할 때 어떤 주소로 할 지 설정
    // htonl=호스트의 오더 방식을 네트워크오더 (long)방식으로 변경, INADDR_ANY=현재 PC의 IP주소
    serv_addr.sin_port=htons(7090);
    // 7989=임의의 포트 번호

    // bind 함수는 소켓의 정보를 구별하기 위해 소켓의 정보를 담는다.
    if(bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1){
        printf("bind error\n");
    }

    if(listen(serv_sock, 5) == -1){ // 5=대기할 수 있는 유저의 큐 크기
        printf("listen error\n");
    }

    char buff[200]; // client 메시지가 담긴 배열
    int recv_len = 0; // 메시지 길이
    while(1){
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
                    // accept로 client의 정보(IP주소)를 넘김.
                    // 여기에 담긴 IP로 사용자를 관리할 수 있음
        char clnt_name[NAMESIZE];
        read(clnt_sock, clnt_name, sizeof(clnt_name));            
        pthread_mutex_lock(&g_mutex);
        strcpy(user[g_clnt_count].name, clnt_name); // 받아온 유저의 이름을 유저 구조체에 저장
        user[g_clnt_count].room = 0;    // 유저의 위치를 전체 채팅방으로 초기 설정
        ++g_room_person_count[0];   // 채팅방 인원수 증가
        g_clnt_socks[g_clnt_count++] = clnt_sock;
        pthread_mutex_unlock(&g_mutex);

        pthread_create(&t_thread, NULL, clnt_connection, (void*) &clnt_sock);
        pthread_detach(t_thread);
    }

    close(serv_sock);

    return 1;
}