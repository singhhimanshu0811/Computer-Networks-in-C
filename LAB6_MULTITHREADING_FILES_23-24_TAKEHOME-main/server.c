#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include<errno.h>
#include <arpa/inet.h>
#include<stdbool.h>
#include <pthread.h>
#include<semaphore.h>
#include<sys/stat.h>


#define BUFFER_SIZE 1024
#define MAX_CLIENT 1024
sem_t map_mutex;
sem_t hist_mutex;
sem_t sem_grps;
char file_array[10*BUFFER_SIZE][10*BUFFER_SIZE]={0};
int file_idx;

int min(int a, int b){
    if(a<b)return a;
    return b;
}

struct keyVal{
    int key;
    char val[100];
    char group[100];
    char type[20];
};

struct Map{
    struct keyVal data[BUFFER_SIZE];
    int size;
};

void init_map(struct Map* map){
    map->size=0;
}

void put(struct Map *map,int key, char * val, char* group, char* type){
    //pthread_mutex_lock(&map_mutex);
    sem_wait(&map_mutex);
    if(map->size<BUFFER_SIZE){
        strcpy(map->data[map->size].val, val);
        map->data[map->size].key=key;
        strcpy(map->data[map->size].group, group);
        strcpy(map->data[map->size].type, type);
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
                    strcpy(map->data[j].type, map->data[j+1].type);
        }
        map->size--;
    }
    //pthread_mutex_unlock(&map_mutex);   
    sem_post(&map_mutex);
}
char* get_type(struct Map* map, int socket_id){
    int idx=0;
    while(idx<map->size && map->data[idx].key!=socket_id){
        idx++;
    }
    return map->data[idx].type;
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
bool file_exists(char *file_name){
    for(int i=0;i<file_idx;i++){
        if(!strcmp(file_name, file_array[i])){
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

void send_file(FILE* file, char *filename, int clinet_sockfd){
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
     
    char *contents;
    contents = (char *)malloc(file_size + 1);
    fread(contents, 1, file_size, file);
    contents[file_size] = '\0';

    char send_buffer[1024*1024]={0};
    sprintf(send_buffer, ";%s~%s;", filename, contents);

    if(send(clinet_sockfd, send_buffer, strlen(send_buffer), 0)==-1){
        perror("file send error\n");
        close(clinet_sockfd);
    }
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
        if(strncmp(op_buffer,"LIST", 4)==0){
            //LIST-<name1>|<authority1>:<name2>|<authority2>:<name3>|<authority3>\n
            char ans[1024]="";
            strcat(ans, "LIST-");
            for(int l=0;l<map.size;l++){
                    if(map.data[l].key!=-100){
                        strcat(ans, map.data[l].val);
                        strcat(ans, "|");
                        strcat(ans, map.data[l].type);
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
            char ans[1024]="";
            for(int l=0;l<map.size;l++){
                    if(map.data[l].key!=-100){
                        strcat(ans, map.data[l].val);
                        strcat(ans, "|");
                        strcat(ans, map.data[l].type);
                        if(l!=map.size-1){
                            strcat(ans, ":");
                        }
                }
            }
            close(client_sockfd);
            delete(&map, client_sockfd);
            
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
                char *new_client_name=map.data[new_client_idx].val;
                char op1[1000]={0}; char op2[1000]={0};
                sprintf(op1, "01_%s-%s.txt", og_str, new_client_name);
                sprintf(op2, "01_%s-%s.txt", new_client_name, og_str);
                strcpy(send_buffer,og_str);
                strcat(send_buffer,":");
                strcat(send_buffer, client_message);
                if ((send_count = send(map.data[new_client_idx].key, send_buffer, sizeof(send_buffer), 0)) == -1) {
                    perror("send");
                    close(client_sockfd);
                    continue;
                }
                bool one=file_exists(op1);
                bool two=file_exists(op2);
                printf("%d-%d-> one and two\n", one, two);
                char file_to_be_sent[BUFFER_SIZE]={0};
                char filepath[BUFFER_SIZE]={0};
                strcat(filepath, "server_files/");
                if(one){
                    strcpy(file_to_be_sent, op1);
                    strcat(filepath, op1);
                    FILE *fileptr=fopen(filepath, "a");
                    fprintf(fileptr, "%s\n", client_message);
                    fflush(fileptr);
                    fclose(fileptr);
                }
                else if(two){
                    strcpy(file_to_be_sent, op2);
                    strcat(filepath, op2);
                    FILE *fileptr=fopen(filepath, "a");
                    fprintf(fileptr, "%s\n", client_message);
                    fflush(fileptr);
                    fclose(fileptr);
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
            char filepath[BUFFER_SIZE]={0};
            memset(filepath, 0, sizeof(filepath));
            strcat(filepath, "server_files/");
            char filenam[1000]={0};
            sprintf(filenam, "02_%s.txt", group_name);
            strcat(filepath, filenam);
            FILE *fileptr=fopen(filepath, "a");
            fclose(fileptr);
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
                char filepath[BUFFER_SIZE]={0};
                memset(filepath, 0, sizeof(filepath));
                strcat(filepath, "server_files/");
                char filenam[1000]={0};
                sprintf(filenam, "02_%s.txt", group_name);
                strcat(filepath, filenam);
                FILE *fileptr=fopen(filepath, "a");
                fprintf(fileptr, "%s\n", send_buffer);
                fflush(fileptr);
                fclose(fileptr);
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
            char filepath[BUFFER_SIZE]={0};
            strcat(filepath, "server_files/");
            strcat(filepath, "03_bcst.txt");
            FILE *fileptr=fopen(filepath, "a");
            fprintf(fileptr, "%s:%s\n", og_str, message);
            fflush(fileptr);
            fclose(fileptr);
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
        else if(strncmp(op_buffer, "CAST:", 5)==0){
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
            int k=strlen(message);
            message[k-1]='\0';
            //printf("bcst continued\n");
            int indx=0;
            while(map.data[indx].key !=client_sockfd){
                indx++;
            }
            
            for(int i=0;i<map.size;i++){
                if(map.data[indx].key==map.data[i].key)continue;
                else if(strncmp(map.data[indx].type, map.data[i].type, min(strlen(map.data[indx].type), strlen(map.data[i].type)))==0){
                    int client_sockfd1 = map.data[i].key;
                    int send_count = send(client_sockfd1, msg,1024, 0);            
                }
            }
        }

        else if(strncmp(op_buffer, "HISF:", 5)==0){
            char* type=get_type(&map, client_sockfd);
            if(strncmp(type, "r",1)){
                int send_count=0;
                char msg[1024]={0};
                strcpy(msg, "EROR:UNAUTHORIZED\n");
                send_count=send(client_sockfd, msg, strlen(msg)+2, 0);
            }
            else{
                 
                char filepath[BUFFER_SIZE]={0};
                strcat(filepath, "server_files/");
                    
                char op_copy[1000];
                strcpy(op_copy, op_buffer);
                char const delim1[]={":"};
                char const delim2[]={"|"};
                char *dump=strtok(op_copy, delim1);
                char *hisf_type=strtok(NULL, delim2);
                char *message=strtok(NULL, delim2);
                message=message+3;
                if(strcmp(hisf_type, "-t 01")==0){
                    int new_client_idx=get_socket(&map, message);
                    char* og_str=get_name(&map, client_sockfd);
                    char *new_client_name=map.data[new_client_idx].val;
                    char op1[1000]={0}; char op2[1000]={0};
                    sprintf(op1, "01_%s-%s.txt", og_str, new_client_name);
                    sprintf(op2, "01_%s-%s.txt", new_client_name, og_str);
                    
                    bool one=file_exists(op1);
                    bool two=file_exists(op2);
                    if(one){
                        strcat(filepath, op1);
                        FILE* file=fopen(filepath, "r");
                        send_file(file, op1, client_sockfd);
                    }
                    else if(two){
                        strcat(filepath, op2);
                        FILE* file=fopen(filepath, "r");
                        send_file(file, op2, client_sockfd);
                    }
                }
                else if(strcmp(hisf_type, "-t 03")==0){
                    strcat(filepath, "03_bcst.txt");
                    FILE* fileptr=fopen(filepath, "r");
                    if(fileptr==NULL){
                        printf("error in bcst file name -server side\n");
                        fflush(stdout);
                    }
                    send_file(fileptr, "03_bcst.txt", client_sockfd);
                }
                else if(strcmp(hisf_type, "-t 02")==0){
                    int idx=0;
                    while(map.data[idx].key!=client_sockfd){
                        idx++;
                    }
                    if(strcmp(map.data[idx].group, message)){
                        printf("%s~%s\n", map.data[idx].group, message);
                        char msg_wrong[BUFFER_SIZE]={0};
                        memset(msg_wrong, 0, sizeof(msg_wrong));
                        strcpy(msg_wrong, "EROR:UNAUTHORISED\n");
                        int send_count=send(client_sockfd, msg_wrong, strlen(msg_wrong), 0);
                        continue;
                    }
                    else{
                        char filepath[BUFFER_SIZE]={0};
                        strcat(filepath, "server_files/");
                        strcat(filepath, "02_");
                        strcat(filepath, message);
                        strcat(filepath, ".txt");
                        FILE *fileptr=fopen(filepath, "r");
                        printf("%s\n", filepath);
                        if(fileptr==NULL){
                            printf("Error in mcst file open-server side");
                        }
                        char filenam[1000]={0};
                        sprintf(filenam, "02_%s.txt",message);
                        send_file(fileptr,filenam, client_sockfd);
                    }
                }
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
    file_idx=0;

    if (mkdir("server_files", 0777) == -1 && errno != EEXIST) {
        perror("mkdir");
    }

    if(argc!=4){
       printf("%s\n", "Give 3 args");
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

    char password[BUFFER_SIZE];
    strcpy(password, argv[3]);

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
    int k=-1;
    while(1){
        //printf("Accepting clients\n");
            new_server_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addrinfo, &sin_size);
            printf("%s", "client accepted");
            fflush(stdout);

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
            char const name_delim[]="|";
            char *name=strtok(name_buffer, name_delim);
            char *type=strtok(NULL, name_delim);
            printf("%s->%s\n", name, type);
            fflush(stdout);
           
            int *client_sockfd_ptr = malloc(sizeof(int));
            *client_sockfd_ptr = new_server_sockfd;
            char cli_password[BUFFER_SIZE];
            int g=strlen(type);
            printf("%s is type\n", type);
            if(!strcmp(type, "r")){
                int recv_count;
                if ((recv_count = recv(new_server_sockfd, cli_password, 1024, 0)) == -1) {
                    perror("recv");
                    close(new_server_sockfd);
                    exit(1);
                }
                int k=strlen(cli_password);
                //cli_password[k-1]='\0';
                if(strcmp(password, cli_password)!=0){
                    char msg_wrong[BUFFER_SIZE]={0};
                    strcpy(msg_wrong, "EROR:PASSWORD REJECTED\n");
                    int send_count=0;
                    if ((send_count = send(new_server_sockfd, msg_wrong, sizeof(msg_wrong), 0)) == -1) {
                        perror("send");
                        close(new_server_sockfd);
                        continue;
                    }
                    close(new_server_sockfd);
                    continue;
                }
                else{
                    char msg_right[BUFFER_SIZE];
                    // sprintf(msg_wrong, "%s", "PASSWORD REJECTED");
                    int send_count;
                    strcpy(msg_right, "PASSWORD ACCEPTED\n");
                    
                    if ((send_count = send(new_server_sockfd, msg_right, sizeof(msg_right), 0)) == -1) {
                        perror("send");
                        close(new_server_sockfd);
                        continue;
                    }
                }
            }
            for(int i=0;i<map.size;i++){
                char *old_client=map.data[i].val;
                char file_name[1000]={0};
                strcat(file_name, "01_");
                strcat(file_name, old_client);
                strcat(file_name, "-");
                strcat(file_name, name);
                strcat(file_name, ".txt");
                strcpy(file_array[file_idx], file_name);
                file_idx++;
                
            }
            put(&map, new_server_sockfd, name, "", type);
            if(pthread_create(&threads[new_server_sockfd%MAX_CLIENT], NULL, handle_client, client_sockfd_ptr)!=0){
                perror("pthread_create failed");
                close(new_server_sockfd);
                continue;
            }
            printf("ok\n");
            
            if(pthread_detach(threads[new_server_sockfd%MAX_CLIENT])){
                perror("pthread_detach_failed\n");
                continue;
            }
       }
       
    return 0;
}