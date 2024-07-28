#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<stdbool.h>
#define MAX_CLIENTS 3
#define BUFFER_SIZE 1024
const char delim[]=":";
bool flag=false;
int temp=0;
struct keyVal{
    int key;
    char val[100];
};

struct Map{
    struct keyVal data[BUFFER_SIZE];
    int size;
};

void init_map(struct Map* map){
    map->size=0;
}

void put(struct Map *map,int key, char * val){
    if(map->size<BUFFER_SIZE){
        strcpy(map->data[map->size].val, val);
        map->data[map->size].key=key;
        map->size++;
    }
}


void delete(struct Map* map, int key){
    int idx=0;
    while(idx<map->size && map->data[idx].key!=key){
        idx++;
    }
    for(int j=idx;j<map->size-1;j++){
                map->data[j].key=map->data[j+1].key;
                strcpy(map->data[j].val, map->data[j+1].val);
            }
            map->size--;    
    
}

int main(int argc, char *argv[]){

    struct Map map;
    init_map(&map);

    char server_IP[INET6_ADDRSTRLEN];    
    strcpy(server_IP, argv[1]);
     
    unsigned int server_port;                
    server_port = atoi(argv[2]);

    int server_sockfd;
    struct sockaddr_in server_addrinfo;
    int yes = 1;

    if ((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("server: socket");
        exit(1);
    }

    // SockOptions
    if (setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        close(server_sockfd);
        exit(1);
    }

    server_addrinfo.sin_family = AF_INET;
    server_addrinfo.sin_port = htons(server_port);
    server_addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);
    if (inet_pton(AF_INET, server_IP, &server_addrinfo.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        close(server_sockfd);
        exit(1);
    }
    if (bind(server_sockfd, (struct sockaddr*) &server_addrinfo, sizeof(server_addrinfo)) == -1) {
        perror("server: bind");
        close(server_sockfd);
        exit(1);
    }
    
    // 3. LISTEN
    if (listen(server_sockfd, 3) == -1) {
        perror("listen");
        close(server_sockfd);
        exit(1);
    }

    server_port = ntohs(server_addrinfo.sin_port);
    int new_server_sockfd;
    struct sockaddr_in client_addrinfo;
    socklen_t sin_size = sizeof(client_addrinfo);
    
    //connecting all clients
        for(int i=0;i<MAX_CLIENTS;i++){
          new_server_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addrinfo, &sin_size);
          if (new_server_sockfd == -1) {
            perror("accept");
            close(new_server_sockfd);
            exit(1);
           }
           char msg[BUFFER_SIZE] = {0};
           memset(msg, 0, sizeof(msg));
           strcpy(msg, "NAME\n");
           int send_count;
           if ((send_count = send(new_server_sockfd, msg, sizeof(msg), 0)) == -1) {
            perror("send");
            close(new_server_sockfd);
            exit(1);
           }
           char reply[BUFFER_SIZE] = {0};
           memset(reply, 0, sizeof(reply));
           int recv_count;
           if ((recv_count = recv(new_server_sockfd, reply, sizeof(msg), 0)) == -1) {
            perror("recv");
            close(new_server_sockfd);
            exit(1);
          }
            int k=strlen(reply);
            reply[k-1]='\0';
            //printf("name is %s\n", reply);
            put(&map, new_server_sockfd, reply);

        }
    //polling starts
    int k=map.size;
    while(map.size>0){
         for(int i=0; i<map.size; i++){
            int client_sockfd=map.data[i].key;
            char msg[BUFFER_SIZE] = {0};
            memset(msg, 0, sizeof(msg));
            sprintf(msg,"%s", "POLL\n");
            int send_count;
            
            if ((send_count = send(client_sockfd, msg, sizeof(msg), 0)) == -1) {
               perror("send");
               close(client_sockfd);
               exit(1);
           }
           int recv_count;
           char reply[BUFFER_SIZE]={0};
           memset(reply, sizeof(reply), 0);
           if ((recv_count = recv(client_sockfd, reply, sizeof(msg), 0)) == -1) {
            perror("recv");
            close(new_server_sockfd);
            exit(1);
          }
           reply[recv_count]='\0'; 
            

            if(strncmp(reply, "NOOP", 4)==0){
                continue;
            }
            else if (strncmp(reply, "MESG", 4) == 0) {
               printf("%s:%s", map.data[i].val, reply + 5);
               continue;
}
            else if(strncmp(reply, "EXIT", 4)==0){
                //printf("%s", "EXIT FOUND\n");
                delete(&map, client_sockfd);
                
                --i;
                continue;
            }
            else if(strncmp(reply, "LIST", 4)==0){
                char ans[1024]="";
                for(int l=0;l<map.size;l++){
                    strcat(ans, map.data[l].val);
                    if(l!=map.size-1){
                        strcat(ans, ":");
                    }
                }
                int k=strlen(ans);
                ans[k]='\n';
                ans[k+1]='\0';
                int send_count=0;
                if ((send_count = send(client_sockfd, ans, sizeof(ans), 0)) == -1) {
                     perror("send");
                     close(client_sockfd);
                     continue;
                }
                continue;
            }
            
            else{
                printf("%s", "INVALID CMD\n");
                
                continue;
            }

        }
        continue;
    }
    printf("%s", "SERVER TERMINATED: EXITING......\n");
    return 0;
    
}