#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include<stdbool.h>
#include <pthread.h>
#include<semaphore.h>
#define BUFFER_SIZE 1024
#define MAX_CLIENT 1024
sem_t map_mutex;
sem_t hist_mutex;
sem_t sem_grps;

// pthread_mutex_t map_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t hist_mutex = PTHREAD_MUTEX_INITIALIZER;

struct keyVal{
    int key;
    char val[100];
    char group[100];
};

struct Map{
    struct keyVal data[BUFFER_SIZE];
    int size;
};

void init_map(struct Map* map){
    map->size=0;
}

void put(struct Map *map,int key, char * val, char* group){
    //pthread_mutex_lock(&map_mutex);
    sem_wait(&map_mutex);
    if(map->size<BUFFER_SIZE){
        strcpy(map->data[map->size].val, val);
        map->data[map->size].key=key;
        strcpy(map->data[map->size].group, group);
        map->size++;
    }
    //pthread_mutex_unlock(&map_mutex);
    sem_post(&map_mutex);
}


void delete(struct Map* map, int key){
    //pthread_mutex_lock(&map_mutex);
    sem_wait(&map_mutex);
    int idx=0;
    while(idx<map->size && map->data[idx].key!=key){
        idx++;
    }
    if(idx<map->size){
        for(int j=idx;j<map->size-1;j++){
                    map->data[j].key=map->data[j+1].key;
                    strcpy(map->data[j].val, map->data[j+1].val);
                    strcpy(map->data[j].group, map->data[j+1].group);
                   
        }
        map->size--;
    }
    //pthread_mutex_unlock(&map_mutex);   
    sem_post(&map_mutex);
}

int get_socket(struct Map* map, char* key){
    int idx=0;
    while(idx<map->size && strncmp(map->data[idx].val, key, strlen(key))!=0){
        idx++;
    }
    if(idx>=map->size){
        return -1;
    }
    else return idx;
}
char* get_name(struct Map* map, int socket){
    int idx=0;
    while(idx<map->size && map->data[idx].key!=socket){
        idx++;
    }
    return map->data[idx].val;
}
int put_group(struct Map* map, char *client, char *group_name){
    int idx=0;
    while(idx<map->size && strcmp(map->data[idx].val, client)!=0){
        idx++;
    }
    if(idx>=map->size)return -1;
    strcpy(map->data[idx].group, group_name);
    return 0;
}
bool group_exists(struct Map* map, char *group){
    for (int idx = 0; idx < map->size; ++idx) {
        if (strcmp(map->data[idx].group, group) == 0) {
            return true;
        }
        
    }
    return false;
}
struct Map map;//an instance of map
char all_history[100000]={0};//maintains all communication from server_end
void put_history(char*client_name, char*op_buffer){
    //pthread_mutex_lock(&hist_mutex);
    sem_wait(&hist_mutex);
    strcat(all_history, client_name);
    strcat(all_history, "-");
    strcat(all_history, op_buffer);
    sem_post(&hist_mutex);
    //pthread_mutex_unlock(&hist_mutex);
}

void *handle_client(void *args){
    //printf("hello from server\n");
    int client_sockfd=*(int*)args;
    char op_buffer[BUFFER_SIZE];
    memset(op_buffer, 0, sizeof(op_buffer));
    while(true) {
        memset(op_buffer, 0, sizeof(op_buffer));
        int recv_count=recv(client_sockfd, op_buffer, 1024, 0);
        if(recv_count==-1 ||recv_count==0){
            delete(&map, client_sockfd);
            close(client_sockfd);
            exit(1);
        }
        char command[5]={0};
        strncpy(command, op_buffer, 4);
        command[4]='\0';
       
        char *client_name=get_name(&map, client_sockfd);
        put_history(client_name, op_buffer);
        if(strncmp(op_buffer,"LIST\n", 5)==0){
            
            char ans[1024]="";
            for(int l=0;l<map.size;l++){
                    if(map.data[l].key!=-100){
                        strcat(ans, map.data[l].val);
                        if(l!=map.size-1){
                            strcat(ans, ":");
                        }
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
            
        }
        else if(strcmp(op_buffer, "EXIT\n")==0){
            //printf("exit found\n");
            
            close(client_sockfd);
            delete(&map, client_sockfd);
            // free(args);
            // pthread_exit(NULL);
            
            break;
        }

        else if(strncmp(op_buffer, "MSGC:", 5)==0){
            
            char op_copy[BUFFER_SIZE];
            memset(op_copy, 0, sizeof(op_copy));
            strcpy(op_copy, op_buffer);
            char const delim[]={":"};
            char *dump=strtok(op_copy, delim);
            char *client_name=strtok(NULL, delim);
            char *client_message=strtok(NULL, delim);
            //printf("%s", client_name);
            int new_client_idx=get_socket(&map, client_name);
            int send_count=0;
            char send_buffer[BUFFER_SIZE];
            if(new_client_idx==-1){
                memset(send_buffer, 0, sizeof(send_buffer));
                strcpy(send_buffer, "USER NOT FOUND\n");
            
                if ((send_count = send(client_sockfd, send_buffer, sizeof(send_buffer), 0)) == -1) {
                    perror("send");
                    close(client_sockfd);
                    continue;
                }
            }

            else{
                memset(send_buffer, 0, sizeof(send_buffer));
                char* og_str=get_name(&map, client_sockfd);
                strcpy(send_buffer,og_str);
                strcat(send_buffer,":");
                strcat(send_buffer, client_message);
                if ((send_count = send(map.data[new_client_idx].key, send_buffer, sizeof(send_buffer), 0)) == -1) {
                    perror("send");
                    close(client_sockfd);
                    continue;
                }
            }
            

        }
        else if(strncmp(op_buffer, "GRPS:", 5)==0){
            sem_wait(&sem_grps);

            char op_copy[BUFFER_SIZE];

            memset(op_copy, 0, sizeof(op_copy));
            strcpy(op_copy, op_buffer);

            char const delim[]=":";
            char *dump=strtok(op_copy, delim);
            char *clients_name=strtok(NULL, delim);
            char *group_name=strtok(NULL, delim);
            int k=strlen(group_name);
            group_name[k-1]='\0';//removing \n
            bool flag=false;
            
            char const delim2[]=",";
            char *token=strtok(clients_name, delim2);
            int j[100];
            int index=0;
            while(token!=NULL){
                j[index]=get_socket(&map, token);
               
                if(j[index]==-1){
                    char msg[1024];
                    memset(msg,0,sizeof(msg));
                    strcpy(msg, "INVALID USERS LIST\n");
                    int send_count=0;
                    if ((send_count = send(client_sockfd, msg, sizeof(msg), 0)) == -1) {
                        perror("send");
                        close(client_sockfd);
                        continue;
                    }
                    
                    flag=true;
                    break;
                }
                else{
                    token=strtok(NULL, delim2);
                }
                index++;
            }
           
            token=strtok(clients_name, delim2);
            if(!flag){
                if(group_exists(&map, group_name)==true){
                    for(int i=0;i<map.size;i++){
                        if(strcmp(map.data[i].group, group_name)==0){
                            strcpy(map.data[i].group, "");
                        }
                    }
                }
                // index=0;
                // while(token!=NULL){
                //     //printf("putting the group name\n");
                //     strcpy(map.data[j[index]].group, group_name);
                //     index++;
                //     token=strtok(NULL,delim);
                // }
                //dont use while loop ever and no need to use strtok here
                for (int idx = 0; idx < index; ++idx) {
                    strcpy(map.data[j[idx]].group, group_name);
                }
                char msg[1024];
                memset(msg,0,sizeof(msg));
                sprintf(msg, "GROUP %s CREATED\n", group_name);
                int send_count=0;
                if ((send_count = send(client_sockfd, msg, sizeof(msg), 0)) == -1) {
                    perror("send");
                    close(client_sockfd);
                    continue;
                }
                
            }
            sem_post(&sem_grps);
        }
        else if(strncmp(op_buffer, "MCST:", 5)==0){
            
            char op_copy[BUFFER_SIZE];
            memset(op_copy, 0, sizeof(op_copy));
            strcpy(op_copy, op_buffer);
            const char delim[]=":";
            char *dump=strtok(op_copy, delim);
            char *group_name=strtok(NULL, delim);
            char *message=strtok(NULL, delim);
            if(group_exists(&map, group_name)==false){
                int send_count=0;
                char send_buffer[BUFFER_SIZE];
                sprintf(send_buffer, "GROUP %s NOT FOUND\n", group_name);

                if ((send_count = send(client_sockfd, send_buffer, sizeof(send_buffer), 0)) == -1) {
                    perror("send");
                    close(client_sockfd);
                    continue;
                }
            }
            
            else{
                char send_buffer[1024];
                memset(send_buffer, 0, 1024);

                char* og_str=get_name(&map, client_sockfd);
                strcpy(send_buffer,og_str);
                strcat(send_buffer,":");
                strcat(send_buffer, message);

                for (int idx = 0; idx < map.size; ++idx) {
                    //printf("%s->%s", map.data[idx].group,group_name);
                    if (strncmp(map.data[idx].group, group_name, strlen(group_name))==0){
                        int client_sockfd = map.data[idx].key;
                        int send_count = send(client_sockfd, send_buffer,1024, 0);
                        if (send_count == -1) {
                            perror("send");
                            continue; 
                        }
                    }
                }
            }
            
        
        }
        else if(strncmp(op_buffer, "BCST:", 5)==0){
            
            //printf("bcst entered\n");
            char* og_str=get_name(&map, client_sockfd);
            char op_copy[BUFFER_SIZE];
            memset(op_copy, 0, sizeof(op_copy));
            strcpy(op_copy, op_buffer);
            const char delim[]=":";

            char *dump=strtok(op_copy, delim);
            char *message=strtok(NULL, delim);

            char msg[1024];
            memset(msg, 0, 1024);
            // og_str[strlen(og_str)-1]='\0';
            strcat(msg, og_str);
            strcat(msg, ":");
            strcat(msg, message);
            //printf("bcst continued\n");
            int idx=0;
            for (int idx = 0; idx < map.size; idx++) {
                if (map.data[idx].key != client_sockfd){
                    int client_sockfd1 = map.data[idx].key;
                    int send_count = send(client_sockfd1, msg,1024, 0);
                    if (send_count == -1) {
                        perror("send");
                        continue; // Skip to the next client
                    }
                }
            }
            
        }
        else if(strncmp(op_buffer, "HIST", 4)==0){
            
            //printf("%s", all_history);
            int send_count=0;
            if ((send_count = send(client_sockfd, all_history, sizeof(all_history), 0)) == -1) {
                perror("send");
                close(client_sockfd);
                continue;
            }
            
        }
        
        else{
            
            int send_count=0;
            char message[1024];
            memset(message, 0, sizeof(message));
            strcpy(message, "INVALID COMMAND\n");
            if ((send_count = send(client_sockfd, message, sizeof(message), 0)) == -1) {
                perror("send");
                close(client_sockfd);
                continue;
            }
            
            continue;
        }
        
        memset(op_buffer, 0, strlen(op_buffer));
    }
    
    pthread_exit(NULL);

}
int main(int argc, char* argv[]){

    init_map(&map);

    if(argc!=3){
       printf("%s\n", "Give 2 args");
       return 2;
    }

    sem_init(&map_mutex, 0,1);
    sem_init(&hist_mutex, 0,1);
    sem_init(&sem_grps, 0,1);
    
    memset(all_history, 0, sizeof(all_history));

    char server_IP[INET6_ADDRSTRLEN];    
    strcpy(server_IP, argv[1]);
     
    unsigned int server_port;                
    server_port = atoi(argv[2]);

    pthread_t threads[MAX_CLIENT];

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
    if (listen(server_sockfd, 1024) == -1) {
        perror("listen");
        close(server_sockfd);
        exit(1);
    }
    

    server_port = ntohs(server_addrinfo.sin_port);
    int new_server_sockfd;
    struct sockaddr_in client_addrinfo;
    socklen_t sin_size = sizeof(client_addrinfo);

    while(1){
        //printf("Accepting clients\n");
            new_server_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addrinfo, &sin_size);
            

            if (new_server_sockfd == -1) {
                perror("accept");
                close(new_server_sockfd);
                exit(1);
            }
            char name_buffer[BUFFER_SIZE];
            memset(name_buffer, 0, sizeof(name_buffer));
            int recv_count;
            if ((recv_count = recv(new_server_sockfd, name_buffer, 1024, 0)) == -1) {
                perror("recv");
                close(new_server_sockfd);
                exit(1);
            }
            //strcat(all_history, name_buffer);
            int k=strlen(name_buffer);
            name_buffer[k]='\0';
           
            put(&map, new_server_sockfd, name_buffer, "");
            int *client_sockfd_ptr = malloc(sizeof(int));
            *client_sockfd_ptr = new_server_sockfd;
            
            if(pthread_create(&threads[new_server_sockfd%MAX_CLIENT], NULL, handle_client, client_sockfd_ptr)!=0){
                perror("pthread_create failed");
                close(new_server_sockfd);
                continue;
            }
            
            if(pthread_detach(threads[new_server_sockfd%MAX_CLIENT])){
                perror("pthread_detach_failed\n");
                continue;
            }
       }
       
    return 0;
}