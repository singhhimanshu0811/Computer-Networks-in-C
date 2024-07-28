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

#define MAX_CLIENTS 8
#define PILANI 1
#define GOA 2
#define HYDERABAD 3

int min(int a, int b){
    if(a<b)return a;
    return b;
}

struct ClientMap{
    int slot;
    char name[40];
};

int pilaniClientSlots[MAX_CLIENTS]={0};
int goaClientSlots[MAX_CLIENTS]={0};
int hydClientSlots[MAX_CLIENTS]={0};

struct ClientMap pilaniClientMap[MAX_CLIENTS];
struct ClientMap goaClientMap[MAX_CLIENTS];
struct ClientMap hydClientMap[MAX_CLIENTS];

int put(struct ClientMap* map, char *deptt, int campus){
    int slot=-1;
    if(campus==1){
        for(int i=1;i<MAX_CLIENTS;i++){
            if(pilaniClientSlots[i]==0){
                slot=i;
                break;
            }
        }
        if(slot==-1){
            //no slots remaining
            return -1;
        }
        pilaniClientSlots[slot]=1;
        pilaniClientMap[slot].slot=slot;
        strcpy(pilaniClientMap[slot].name, deptt);
        //client added to pilani map, on lowest possible idx
    }
    else if(campus==2){
        for(int i=1;i<MAX_CLIENTS;i++){
            if(goaClientSlots[i]==0){
                slot=i;
                break;
            }
        }
        if(slot==-1){
            //no slots remaining
            return -1;
        }
        goaClientSlots[slot]=1;
        goaClientMap[slot].slot=slot;
        strcpy(goaClientMap[slot].name, deptt);
        //client added to goa map, on lowest possible idx
    }
    else if(campus==3){
        for(int i=1;i<MAX_CLIENTS;i++){
            if(hydClientSlots[i]==0){
                slot=i;
                break;
            }
        }
        if(slot==-1){
            //no slots remaining
            return -1;
        }
        hydClientSlots[slot]=1;
        hydClientMap[slot].slot=slot;
        strcpy(hydClientMap[slot].name, deptt);
        //client added to pilani map, on lowest possible idx
    }
    else{
        //invalid campus, no map available, gand marao
        return -1;
    }
    return slot;
}

void delete(struct ClientMap* map, char *deptt, int campus){
    if(campus==1){
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (pilaniClientSlots[i] == 1 && strcmp(pilaniClientMap[i].name, deptt) == 0) {
                slot = i;
                break;
            }
        }
        if(slot==-1){
            //no slot found;
            return;
        }
        memset(pilaniClientMap[slot].name, 0, sizeof(pilaniClientMap[slot].name)); // Free allocated memory for client name
        pilaniClientSlots[slot] = 0;
    }
    else if(campus==2){
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (goaClientSlots[i] == 1 && strcmp(goaClientMap[i].name, deptt) == 0) {
                slot = i;
                break;
            }
        }
        if(slot==-1){
            //no slot found;
            return;
        }
        memset(goaClientMap[slot].name, 0, sizeof(goaClientMap[slot].name)); // Free allocated memory for client name
        goaClientSlots[slot] = 0;
    }
    else if(campus==3){
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (hydClientSlots[i] == 1 && strcmp(hydClientMap[i].name, deptt) == 0) {
                slot = i;
                break;
            }
        }
        if(slot==-1){
            //no slot found;
            return;
        }
        memset(hydClientMap[slot].name, 0, sizeof(hydClientMap[slot].name)); // Free allocated memory for client name
        hydClientSlots[slot] = 0;
    }
    else{
        //invalid campus
        return;
    }
}

bool existsMap(struct ClientMap* map, int id, int campus){
    if(campus==1){
        return pilaniClientSlots[id];
    }
    else if(campus==2){
        return goaClientSlots[id];
    }
    else if(campus==3){
        return hydClientSlots[id];
    }
    return false;
}

int findSlot(struct ClientMap* map, char *deptt){
    int slot=-1;
    for(int i=0;i<MAX_CLIENTS;i++){
        if(strncmp(map[i].name, deptt, min(strlen(map[i].name), strlen(deptt)))==0){
            slot=i;
            return slot;
        }
    }
    return slot;
}

void list(char str[]){
    str[0]=';';
    for(int i=0;i<MAX_CLIENTS;i++){
        if(goaClientSlots[i]==1){
            strcat(str, "P;");
            strcat(str, pilaniClientMap[i].name);
            strcat(str, "=");
            int slot=findSlot(pilaniClientMap, pilaniClientMap[i].name);
            char slot_str[20]={0};
            sprintf(slot_str, "%d", slot);
            strcat(str, slot_str);
            strcat(str, ";");
        }
    }
    for(int i=0;i<MAX_CLIENTS;i++){
        if(goaClientSlots[i]==1){
            strcat(str, "G;");
            strcat(str, goaClientMap[i].name);
            strcat(str, "=");
            int slot=findSlot(goaClientMap, goaClientMap[i].name);
            char slot_str[20]={0};
            sprintf(slot_str, "%d", slot);
            strcat(str, slot_str);
            strcat(str, ";");
        }
    }
    for(int i=0;i<MAX_CLIENTS;i++){
        if(hydClientSlots[i]==1){
            strcat(str, "H;");
            strcat(str, hydClientMap[i].name);
            strcat(str, "=");
            int slot=findSlot(hydClientMap, hydClientMap[i].name);
            char slot_str[20]={0};
            sprintf(slot_str, "%d", slot);
            strcat(str, slot_str);
            strcat(str, ";");
        }
    }
    strcat(str, "\n");
    int k=strlen(str);
    str[k]='\0';
}

int pilaniSocketIdGoa,pilaniSocketIdHyd, pilaniSocketIdClient, 
goaSocketIdPilani,goaSocketIdHyd, goaSocketIdClient, 
hydSocketIdPilani,hydSocketIdGoa, hydSocketIdClient;
int myCampus;
struct sockaddr_in hyd_addrinfo, pilani_addrinfo, goa_addrinfo, client_addrinfo;

int serverSetupPilani(char* addr, int goaPort, int hydPort){
    myCampus = PILANI;
    //code for server setup
    int opt=1;
    //creating pilani socket
    pilaniSocketIdGoa=socket(AF_INET, SOCK_STREAM, 0);
    if(pilaniSocketIdGoa==-1){
        perror("Pilani socket creation for goa failed\n");
        return -1;
    }
    pilaniSocketIdHyd=socket(AF_INET, SOCK_STREAM, 0);
    if(pilaniSocketIdHyd==-1){
        perror("Pilani socket creation for hyd failed\n");
        return -1;
    }
    //extra option fro pilani socketm can be taken lite, but tp ke liye likh hi do
    //bakchodi kar rha mai toh
    if(setsockopt(pilaniSocketIdGoa, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }
    if(setsockopt(pilaniSocketIdHyd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }
    //configuring pilani server
    pilani_addrinfo.sin_family = AF_INET;
    pilani_addrinfo.sin_addr.s_addr = INADDR_ANY;
    pilani_addrinfo.sin_port = htons(goaPort); 
    //goaPort is port on which pilani server waits for goa client
    //binding to port for goa
    if (bind(pilaniSocketIdGoa, (struct sockaddr *)&pilani_addrinfo, sizeof(pilani_addrinfo)) < 0) {
        perror("Pilani server bind failed for Goa");
        return -1;
    }
    //binding to port for hyd
    pilani_addrinfo.sin_port = htons(hydPort); 
    //hyd Port is pilani server port on which it waits for hyderabad client
    if (bind(pilaniSocketIdHyd, (struct sockaddr *)&pilani_addrinfo, sizeof(pilani_addrinfo)) < 0) {
        perror("Pilani server bind failed for Hyderabad");
        return -1;
    }

   //now listening, on both the ports
    if (listen(pilaniSocketIdGoa, MAX_CLIENTS) < 0) {
        perror("Pilani server listen failed");
        return -1;
    }
    if (listen(pilaniSocketIdHyd, MAX_CLIENTS) < 0) {
        perror("Pilani server listen failed");
        return -1;
    }
    //goa connection to pilani
    goa_addrinfo.sin_family = AF_INET;
    goa_addrinfo.sin_port = htons(goaPort); // Goa server's Port 1
    if (inet_pton(AF_INET, addr, &goa_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (accept(pilaniSocketIdGoa, (struct sockaddr *)&goa_addrinfo, (socklen_t*)(sizeof(goa_addrinfo))) < 0) {
        perror("Connection to Goa server failed");
        return -1;
    }
    //hyd connection to pilani
    hyd_addrinfo.sin_family = AF_INET;
    hyd_addrinfo.sin_port = htons(hydPort); // Hyderabad server's Port 2
    if (inet_pton(AF_INET, addr, &hyd_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (accept(pilaniSocketIdHyd, (struct sockaddr *)&hyd_addrinfo, (socklen_t*)(sizeof(hyd_addrinfo))) < 0) {
        perror("Connection to Hyderabad server failed");
        return -1;
    }
    return 1;
}

int serverSetupGoa(char* addr, int pilaniPort, int hydPort){
    myCampus = GOA;
    //code for server setup
    int opt = 1;
    goaSocketIdHyd = socket(AF_INET, SOCK_STREAM, 0);
    goaSocketIdPilani = socket(AF_INET, SOCK_STREAM, 0);
    if (goaSocketIdHyd == -1||goaSocketIdPilani==-1) {
        perror("Goa server socket creation failed");
        return -1;
    }

    if (setsockopt(goaSocketIdHyd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }
    if (setsockopt(goaSocketIdPilani, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        return -1;
    }

    goa_addrinfo.sin_family = AF_INET;
    goa_addrinfo.sin_addr.s_addr = INADDR_ANY;
    goa_addrinfo.sin_port = htons(hydPort); // Goa server listens on Port 2 for Hyderabad server
    if (bind(goaSocketIdHyd, (struct sockaddr *)&goa_addrinfo, sizeof(goa_addrinfo)) < 0) {
        perror("Goa server bind failed");
        return -1;
    }
//does not need to bind to pilani server, as goa server will be client to that
    if (listen(goaSocketIdHyd, MAX_CLIENTS) < 0) {
        perror("Goa server listen failed");
        return -1;
    } //listening to hyderabad server
    hyd_addrinfo.sin_family = AF_INET;
    hyd_addrinfo.sin_port = htons(hydPort); // Hyderabad server's Port 2
    if (inet_pton(AF_INET, addr, &hyd_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (accept(goaSocketIdHyd, (struct sockaddr *)&hyd_addrinfo, (socklen_t*)(sizeof(goa_addrinfo))) < 0) {
        perror("Connection to Hyderabad server failed");
        return -1;
    }

   //from pilani port
    pilani_addrinfo.sin_family = AF_INET;
    pilani_addrinfo.sin_port = htons(pilaniPort); // Pilani server's Port 1
    if (inet_pton(AF_INET, addr, &pilani_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (connect(goaSocketIdPilani, (struct sockaddr *)&pilani_addrinfo, sizeof(pilani_addrinfo)) < 0) {
        perror("Connection to Pilani server failed");
        return -1;
    }
   //to pilani port,  connect
    return 1;
}

int serverSetupHyderabad(char* addr, int pilaniPort, int goaPort){
    myCampus = HYDERABAD;
    //code for server setup
    
    int opt = 1;

    // Create socket for Hyderabad server
    hydSocketIdPilani = socket(AF_INET, SOCK_STREAM, 0);
    hydSocketIdGoa = socket(AF_INET, SOCK_STREAM, 0);
    if (hydSocketIdPilani == -1 || hydSocketIdGoa==-1) {
        perror("Hyderabad server socket creation failed");
        return -1;
    }
    
    pilani_addrinfo.sin_family = AF_INET;
    pilani_addrinfo.sin_addr.s_addr = INADDR_ANY;
    pilani_addrinfo.sin_port = htons(pilaniPort); // Hyderabad server connects to pilani on pilani port
    
    if (inet_pton(AF_INET, addr, &pilani_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (connect(hydSocketIdPilani, (struct sockaddr *)&pilani_addrinfo, sizeof(pilani_addrinfo)) < 0) {
        perror("Connection to Pilani server failed");
        return -1;
    }
    goa_addrinfo.sin_family = AF_INET;
    goa_addrinfo.sin_addr.s_addr = INADDR_ANY;
    goa_addrinfo.sin_port = htons(goaPort);
    
    if (inet_pton(AF_INET, addr, &goa_addrinfo.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (connect(hydSocketIdGoa, (struct sockaddr *)&goa_addrinfo, sizeof(goa_addrinfo)) < 0) {
        perror("Connection to Goa server failed");
        return -1;
    }
    
    return 1;
   
}

int main(int argc, char *argv[])
{
    if (argc != 6)
	{
		printf("Refer Qn for arguments\n");
		return -1;
	}
    char* addr = argv[1];
	int port1 = atoi(argv[2]);
    int port2 = atoi(argv[3]);
    int port3 = atoi(argv[4]);
    int campus = atoi(argv[5]);

    if(campus == PILANI){
        serverSetupPilani(addr, port1, port2);
        int opt=1;
        //creating pilani socket
        pilaniSocketIdClient=socket(AF_INET, SOCK_STREAM, 0);
        
        if(pilaniSocketIdClient==-1){
            perror("Pilani socket creation for goa failed\n");
            return -1;
        }
        if(setsockopt(pilaniSocketIdClient, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt");
            return -1;
        }

        pilani_addrinfo.sin_family = AF_INET;
        pilani_addrinfo.sin_port = htons(port3);
        pilani_addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);


        if (inet_pton(AF_INET, addr, &pilani_addrinfo.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            close(pilaniSocketIdClient);
            exit(1);
        }
        if (bind(pilaniSocketIdClient, (struct sockaddr*) &pilani_addrinfo, sizeof(pilani_addrinfo)) == -1) {
            perror("server: bind");
            close(pilaniSocketIdClient);
            exit(1);
        }
        
        // 3. LISTEN
        if (listen(pilaniSocketIdClient, 1024) == -1) {
            perror("listen");
            close(pilaniSocketIdClient);
            exit(1);
        }
        printf("Server for pilani client started\n");
    }
    else if(campus == GOA){
        serverSetupGoa(addr, port1, port2);
        int opt=1;
        goaSocketIdClient=socket(AF_INET, SOCK_STREAM, 0);
        if(goaSocketIdClient==-1){
            perror("Pilani socket creation for goa failed\n");
            return -1;
        }
        if(setsockopt(goaSocketIdClient, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt");
            return -1;
        }
        goa_addrinfo.sin_family = AF_INET;
        goa_addrinfo.sin_port = htons(port3);
        goa_addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);

        if (inet_pton(AF_INET, addr, &goa_addrinfo.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            close(goaSocketIdClient);
            exit(1);
        }
        if (bind(pilaniSocketIdClient, (struct sockaddr*) &goa_addrinfo, sizeof(goa_addrinfo)) == -1) {
            perror("server: bind");
            close(goaSocketIdClient);
            exit(1);
        }
        
        if (listen(goaSocketIdClient, 1024) == -1) {
            perror("listen");
            close(goaSocketIdClient);
            exit(1);
        }
    }
    else if(campus == HYDERABAD){
        int opt=1;
        serverSetupHyderabad(addr, port1, port2);
        hydSocketIdClient=socket(AF_INET, SOCK_STREAM, 0);
        if(hydSocketIdClient==-1){
            perror("Pilani socket creation for goa failed\n");
            return -1;
        }
        if(setsockopt(hydSocketIdClient, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
            perror("setsockopt");
            return -1;
        }

        hyd_addrinfo.sin_family = AF_INET;
        hyd_addrinfo.sin_port = htons(port3);
        hyd_addrinfo.sin_addr.s_addr = htonl(INADDR_ANY);

        if (inet_pton(AF_INET, addr, &hyd_addrinfo.sin_addr) <= 0) {
            printf("\nInvalid address/ Address not supported \n");
            close(hydSocketIdClient);
            exit(1);
        }
        if (bind(hydSocketIdClient, (struct sockaddr*) &hyd_addrinfo, sizeof(hyd_addrinfo)) == -1) {
            perror("server: bind");
            close(hydSocketIdClient);
            exit(1);
        }
        
        // 3. LISTEN
        if (listen(hydSocketIdClient, 1024) == -1) {
            perror("listen");
            close(hydSocketIdClient);
            exit(1);
        }
    }
    else{
        printf("Invalid campus\n");
        return -1;
    }
   //server setup should be done by now
    //start listening for department connections and subsequently create threads to handle them
    int new_server_sockfd;
    socklen_t sin_size = sizeof(client_addrinfo);
    char init_reply[1024]={0};
    struct packet* init_packet;
    
    if(campus==PILANI){
        new_server_sockfd = accept(pilaniSocketIdClient, (struct sockaddr*) &client_addrinfo, &sin_size);
        printf("%s", "pilani client accepted\n");
        fflush(stdout);
            
        memset(init_reply, 0, sizeof(init_reply));
        int recv_count;
        recv_count=recv(new_server_sockfd, init_reply, sizeof(init_reply), 0);
        if(recv_count==0 || recv_count==-1){
            perror("recv");
            fflush(stdout);
            exit(1);
        }
        int slot=put(pilaniClientMap, init_reply, campus);
        init_packet=deserialize(init_reply);
        if(init_packet->type==1){
            printf("Sending first ack for pilani packet\n");
            struct packet* ackNackPacket=generatePacket(2, 5, 5+strlen(init_reply), 0, slot, 0, 1, 0, 1, 1, 1, "");
            char *ackNackBuffer=serialize(ackNackPacket);
            int send_count=send(new_server_sockfd, ackNackBuffer, sizeof(ackNackBuffer), 0);
            if(send_count==-1){
                perror("send ack nack error\n");
                close(new_server_sockfd);
                exit(1);
            }
            char list_buffer[1024];
            memset(list_buffer, 0, sizeof(list_buffer));
            list(list_buffer);
            struct packet *listPacket;
            send_count=send(new_server_sockfd, list_buffer, sizeof(list_buffer), 0);
            if(send_count==-1){
                perror("send list error\n");
                close(new_server_sockfd);
                exit(1);
            }


        }

    }
    if(campus==GOA){
        new_server_sockfd = accept(goaSocketIdClient, (struct sockaddr*) &client_addrinfo, &sin_size);
        printf("%s", "goa client accepted\n");
        fflush(stdout);
            
        memset(init_reply, 0, sizeof(init_reply));
        int recv_count;
        recv_count=recv(new_server_sockfd, init_reply, sizeof(init_reply), 0);
        if(recv_count==0 || recv_count==-1){
            perror("recv");
            fflush(stdout);
            exit(1);
        }
        int slot=put(goaClientMap, init_reply, campus);
        init_packet=deserialize(init_reply);
        if(init_packet->type==1){
            printf("Sending first ack for pilani packet\n");
            struct packet* ackNackPacket=generatePacket(2, 5, 5+strlen(init_reply), 0, slot, 0, 1, 0, 1, 2, 2, "");
            char *ackNackBuffer=serialize(ackNackPacket);
            int send_count=send(new_server_sockfd, ackNackBuffer, sizeof(ackNackBuffer), 0);
            if(send_count==-1){
                perror("send ack nack error\n");
                close(new_server_sockfd);
                exit(1);
            }
            char list_buffer[1024];
            memset(list_buffer, 0, sizeof(list_buffer));
            list(list_buffer);
            struct packet *listPacket;
            send_count=send(new_server_sockfd, list_buffer, sizeof(list_buffer), 0);
            if(send_count==-1){
                perror("send list error\n");
                close(new_server_sockfd);
                exit(1);
            }


        }
    }

    if(campus==HYDERABAD){
        new_server_sockfd = accept(hydSocketIdClient, (struct sockaddr*) &client_addrinfo, &sin_size);
        printf("%s", "hyderabad client accepted\n");
        fflush(stdout);
            
        memset(init_reply, 0, sizeof(init_reply));
        int recv_count;
        recv_count=recv(new_server_sockfd, init_reply, sizeof(init_reply), 0);
        if(recv_count==0 || recv_count==-1){
            perror("recv");
            fflush(stdout);
            exit(1);
        }
        int slot=put(hydClientMap, init_reply, campus);
        init_packet=deserialize(init_reply);
        if(init_packet->type==1){
            printf("Sending first ack for hyd packet\n");
            struct packet* ackNackPacket=generatePacket(2, 5, 5+strlen(init_reply), 0, slot, 0, 1, 0, 1, 1, 1, "");
            char *ackNackBuffer=serialize(ackNackPacket);
            int send_count=send(new_server_sockfd, ackNackBuffer, sizeof(ackNackBuffer), 0);
            if(send_count==-1){
                perror("send ack nack error\n");
                close(new_server_sockfd);
                exit(1);
            }
            char list_buffer[1024];
            memset(list_buffer, 0, sizeof(list_buffer));
            list(list_buffer);
            struct packet *listPacket;
            send_count=send(new_server_sockfd, list_buffer, sizeof(list_buffer), 0);
            if(send_count==-1){
                perror("send list error\n");
                close(new_server_sockfd);
                exit(1);
            }


        }
    }
    
    
}



