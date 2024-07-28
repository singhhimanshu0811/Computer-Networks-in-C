#include "header.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdbool.h>
#include<sys/stat.h>
int department_id=-1;
int campus_id=-1;
char *pilaniClients[1024];
char *goaClients[1024];
char *hydClients[1024];
int pilaniIdx=0; int goaIdx=0; int hydIdx=0;
int min(int a, int b){
    if(a<b)return a;
    return b;
}

void fillList(char* clientList, int campus){
	const char *delim="=;";
	int x=1;
	char name[5];
	if(campus==1){
		memset(name, 0, sizeof(name));
		char*token;
		token=strtok(clientList, ":");
		while(token!=NULL){
			if(strcmp(token, "P")){
				if(x==1){
            		printf("%s~%s\n", token, name);
					strcpy(pilaniClients[atoi(token)], name);
            		x=0;
        		}
				else{
					strcpy(name, token);
            		x=1;
				}
			}
			token=strtok(NULL, delim);
		}
	}
	else if(campus==2){
		memset(name, 0, sizeof(name));
		char*token;
		token=strtok(clientList, ":");
		while(token!=NULL){
			if(strcmp(token, "G")){
				if(x==1){
            		printf("%s~%s\n", token, name);
					strcpy(goaClients[atoi(token)], name);
            		x=0;
        		}
				else{
					strcpy(name, token);
            		x=1;
				}
			}
			token=strtok(NULL, delim);
		}
	}
	else if(campus==3){
		memset(name, 0, sizeof(name));
		char*token;
		token=strtok(clientList, ":");
		while(token!=NULL){
			if(strcmp(token, "H")){
				if(x==1){
            		printf("%s~%s\n", token, name);
					strcpy(hydClients[atoi(token)], name);
            		x=0;
        		}
				else{
					strcpy(name, token);
            		x=1;
				}
			}
			token=strtok(NULL, delim);
		}
	}
}

//input in the below functions are the strings obtained from the 
//user through stdin in its entirety. 
int send_function(int client_sockid, char*msg){
	char message[1024];
	strcpy(message, msg);
	int send_count;
    if((send_count = send(client_sockid, message, sizeof(message), 0)) == -1){
        perror("send");
        fflush(stdout);
        exit(1);
    }
    return send_count;
}
pthread_t threads[6];
int create_connection(char *addr, int port, struct sockaddr_in server_addrinfo){
	int client_sockfd;
    	// 1. SOCKET
    if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("client: socket");
        fflush(stdout);
        exit(1);
        }
       // NO BIND NECESSARY IN CLIENT!!!		
    server_addrinfo.sin_family = AF_INET;		
    server_addrinfo.sin_port = htons(port);	
     
    if(inet_pton(AF_INET, addr, &server_addrinfo.sin_addr) <= 0){
        printf("\nInvalid address/ Address not supported \n");
        close(client_sockfd);
        exit(1);
    }
        // 2. CONNECT
    if(connect(client_sockfd, (struct sockaddr*)&server_addrinfo, sizeof(server_addrinfo)) == -1){
    /// client connects if server port has started listen()ing and queue is non-full; 
    //however server connects to client only when it accept()s
    	printf("Could not find server");	
        close(client_sockfd);
        exit(1);
    }
    return client_sockfd;
}
void* send_data(void* args) {
    while(true){
        char msg[1024];
        memset(msg, 0, 1024);
        int socket_id=*(int*)args;
        
        fgets(msg, 1024, stdin);
        int k=strlen(msg);
        msg[k-1]='\0';
            //printf("msg is %s", msg);		
        int send_count;
        if((send_count = send(socket_id, msg, sizeof(msg), 0)) == -1){
            perror("send");
            fflush(stdout);
            exit(1);
    	}

        if(strcmp(msg, "EXIT\n")==0){
            break;
        }
    }
    pthread_exit(NULL);
}
void* recv_data(void *args) {
    while(true){
        char reply[1024];
        memset(reply, 0, 1024);
        int socket_id=*(int*)args;
        
        int recv_count;
        if((recv_count = recv(socket_id, reply, 1024, 0)) == -1){
            perror("recv");
            fflush(stdout);
            exit(1);
        }
        if(recv_count == 0){
            break;
        }
        printf("%s", reply);
    }
    pthread_exit(NULL);
}

void thread_maker(int socket_id, int idx){
	int *client_sockfd_ptr = malloc(sizeof(int));;
    *client_sockfd_ptr = socket_id;	
        
    if(pthread_create(&threads[idx], NULL, send_data, client_sockfd_ptr)!=0){
        perror("pthread_create failed");
        fflush(stdout);
        close(socket_id);
    }
    if(pthread_create(&threads[idx+1], NULL, recv_data, client_sockfd_ptr)!=0){
        perror("pthread_create failed");
        fflush(stdout);
        close(socket_id);

    }
    for (int i = 0; i < 2; i++) {
        if(pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failed");
            fflush(stdout);
            return;
        }
    }
}

int destDeptId(char* deptt, char* campus){
	int dest_dept_id=-1;
	if(strncmp(campus, "P", 1)==0){
		for(int i=0;i<1024;i++){
			if(!strncmp(deptt, pilaniClients[i], min(strlen(deptt), strlen(pilaniClients[i])))){
				dest_dept_id=i;
				break;
			}
		}
	}
	else if(strncmp(campus, "G", 1)==0){
		for(int i=0;i<1024;i++){
			if(!strncmp(deptt, goaClients[i], min(strlen(deptt), strlen(goaClients[i])))){
				dest_dept_id=i;
				break;
			}
		}
	}
	else if(strncmp(campus, "H", 1)==0){
		for(int i=0; i<1024;i++){
			if(!strncmp(deptt, hydClients[i], min(strlen(deptt), strlen(hydClients[i])))){
				dest_dept_id=i;
				break;
			}
		}
	}
   return dest_dept_id;
}
int destCampusId(char* campus){
	if(!strncmp(campus, "P", 1)){
		return 1;
	}
	else if(!strncmp(campus, "G", 1)){
		return 2;
	}
	else if(!strncmp(campus, "H", 1)){
		return 3;
	}
	return -1;
}

struct packet* generateUnicastPacket(char* input)
{
	//code fore generating Unicast packets
	//Use for inputs starting from 1 and 2
	struct packet* unicast;
	if(input[0]=='1'){
		char *dump1=strtok(input, ".");
		char *campus=strtok(NULL, ":");
		char *deptt=strtok(NULL, ":");
		char *payload=strtok(NULL, ":");
		int dest_dept_id=destDeptId(deptt, campus);
		int dest_campus_id=destCampusId(campus);

		unicast=generatePacket(2, 6, 6+strlen(payload), department_id, dest_dept_id, 0, 0, 0, 100, campus_id, dest_campus_id, payload);
    }
	else if(input[0]=='2'){
		char *dump=strtok(input, ".");
		char *deptt=strtok(NULL, ":");
		char *payload=strtok(NULL, ":");
        int dest_dept_id=-1;
		if(campus_id==1){
			dest_dept_id=destDeptId(deptt, "P");
		}
		else if(campus_id==2){
			dest_dept_id=destDeptId(deptt, "G");
		}
		else if(campus_id==3){
			dest_dept_id=destDeptId(deptt, "H");
		}

		unicast=generatePacket(2, 5, 5+strlen(payload), department_id, dest_dept_id, 0, 0, 0, 100, campus_id, campus_id, payload);
	}
	
    return unicast;
}

struct packet* generateBroadcastPacket(char* input)
{
	//code for generating broadcast packets
	//Use for unputs starting with 3 and 4
	struct packet* broadcast;
	if(input[0]=='3'){
		char *payload=input+2;
		broadcast=generatePacket(2,5,5+strlen(payload), department_id, 111, 0, 0, 0, 100, campus_id, campus_id, payload);
	}
	else if(input[0]=='4'){
		char *payload=input+2;
		broadcast=generatePacket(2, 6, 6+strlen(payload), department_id, 1111, 0, 0, 0, 100, campus_id, -1, payload);
	}
	return broadcast;
}

struct packet* generateControlPacket(char* input)
{
	//code for generating control packets
	//Use for inputs starting with 5

}

int main(int argc, char *argv[])
{
    if (argc != 5)
	{
		printf("Refer Qn for arguments\n");
        fflush(stdout);
		exit(1);
	}
    
	// extract the address and port from the command line arguments
	char* addr = argv[1];
	int port = atoi(argv[2]);
    int campus = atoi(argv[3]);
	campus_id=campus;
    char* department = argv[4];
	struct sockaddr_in hyd_addrinfo, pilani_addrinfo, goa_addrinfo, client_addrinfo;
	int client_sockfd;
	struct packet* init_packet;
	char *serialised_init;
	char reply[1024];
    if(campus==1){
		client_sockfd=create_connection(addr, port, pilani_addrinfo);
		
		init_packet=generatePacket(2, 5, 5+strlen(department), 7, 0, 0, 0, 0, 100, 1, 1, department);
		//ack=100 in normal packets, arbitrary
		serialised_init=serialize(init_packet);
        int send_count=send_function(client_sockfd, serialised_init);
    
	    memset(reply, 0, sizeof(reply));
		int recv_count;
		recv_count=recv(client_sockfd, reply, sizeof(reply), 0);
		if(recv_count==0 || recv_count<-1){
			perror("recv");
            fflush(stdout);
            exit(1);
		}
		struct packet *ackNackReply=deserialize(reply);
        department_id=ackNackReply->destDept;

        memset(reply, 0, sizeof(reply));
        recv_count=recv(client_sockfd, reply, sizeof(reply), 0);
		if(recv_count==0 || recv_count<-1){
			perror("recv");
            fflush(stdout);
            exit(1);
		}
		char *pilaniList=strtok(reply, "|");
		char *goaList=strtok(NULL, "|");
		char *hydList=strtok(NULL, "|");
		if(pilaniList!=NULL)fillList(pilaniList, 1);
		if(goaList!=NULL)fillList(goaList, 2);
		if(hydList!=NULL)fillList(hydList, 3);
		
		thread_maker(client_sockfd, 0);
	}
	else if(campus==2){
		client_sockfd=create_connection(addr, port, goa_addrinfo);
		
		init_packet=generatePacket(2, 5, 5+strlen(department), 7, 0, 0, 0, 0, 100, 2, 2, department);
		//ack=100 in normal packets, arbitrary
		serialised_init=serialize(init_packet);
        int send_count=send_function(client_sockfd, serialised_init);
		memset(reply, 0, sizeof(reply));
		int recv_count;
		recv_count=recv(client_sockfd, reply, sizeof(reply), 0);
		if(recv_count==0 || recv_count<-1){
			perror("recv");
            fflush(stdout);
            exit(1);
		}
		struct packet *ackNackReply=deserialize(reply);
        department_id=ackNackReply->destDept;

		memset(reply, 0, sizeof(reply));
        recv_count=recv(client_sockfd, reply, sizeof(reply), 0);
		if(recv_count==0 || recv_count<-1){
			perror("recv");
            fflush(stdout);
            exit(1);
		}
		char *pilaniList=strtok(reply, "|");
		char *goaList=strtok(NULL, "|");
		char *hydList=strtok(NULL, "|");
		if(pilaniList!=NULL)fillList(pilaniList, 1);
		if(goaList!=NULL)fillList(goaList, 2);
		if(hydList!=NULL)fillList(hydList, 3);

		thread_maker(client_sockfd, 1);
		
        
	}
	else if(campus==3){
		client_sockfd=create_connection(addr, port, hyd_addrinfo);
		
		init_packet=generatePacket(2, 5, 5+strlen(department), 7, 0,  0, 0, 0, 100, 2, 2, department);
		//ack=100 in normal packets, arbitrary
		serialised_init=serialize(init_packet);
        int send_count=send_function(client_sockfd, serialised_init);
		memset(reply, 0, sizeof(reply));
		int recv_count;
		recv_count=recv(client_sockfd, reply, sizeof(reply), 0);
		if(recv_count==0 || recv_count<-1){
			perror("recv");
            fflush(stdout);
            exit(1);
		}
		struct packet *ackNackReply=deserialize(reply);
        department_id=ackNackReply->destDept;

		char *pilaniList=strtok(reply, "|");
		char *goaList=strtok(NULL, "|");
		char *hydList=strtok(NULL, "|");
		if(pilaniList!=NULL)fillList(pilaniList, 1);
		if(goaList!=NULL)fillList(goaList, 2);
		if(hydList!=NULL)fillList(hydList, 3);

		thread_maker(client_sockfd, 1);
		
		
	}
   
    return 0;
}