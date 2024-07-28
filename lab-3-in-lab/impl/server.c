#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

void findReply(char* msg, char* reply) {
    const char delim[]=":";
    char *temp=strtok(msg, delim);
    char *reqd="ECHO";
    if(strcmp(temp,reqd)!=0){
        sprintf(reply, "%s", "Invalid command: Unable to echo!");
		return;
    }
    char *message=strtok(NULL, delim);
    char *num=strtok(NULL, delim);
    if(num==NULL){
            char messageTemp[5000]="";
            strncat(messageTemp, message, 5);
            sprintf(reply, "%s:%s", "OHCE", messageTemp);
    }
    else{
        int numBytes=atoi(num);
        int temp=strlen(msg);
        if(numBytes<0){
            sprintf(reply, "%s", "Error: Negative number of bytes");
            return;
        }
        else if(numBytes>temp){
            char messageTemp[5000]="";
            strncat(messageTemp, message, strlen(message));
            char str[1000];
            int l=strlen(message);
            sprintf(str, " (%d bytes sent)", l);
            strcat(messageTemp, str);
            sprintf(reply, "%s:%s", "OHCE", messageTemp);
        }
        else{
            char messageTemp[5000]="";
            strncat(messageTemp, message, numBytes);
            sprintf(reply, "%s:%s", "OHCE", messageTemp);
            
        }
    }
}

// create connection
int create_connection(char* addr, int port) {
    int server_sockfd;
    struct sockaddr_in server_addrinfo;
    int yes = 1;
    char server_IP[INET6_ADDRSTRLEN];
    unsigned int server_port;
    strcpy(server_IP, addr);
    server_port = (port);

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
    
    return server_sockfd;
}

// Accept incoming connections
int client_connect(int socket_id) {
    int new_server_sockfd;
    struct sockaddr_in client_addrinfo;
    socklen_t sin_size = sizeof(client_addrinfo);
    new_server_sockfd = accept(socket_id, (struct sockaddr*) &client_addrinfo, &sin_size);
    if (new_server_sockfd == -1) {
        perror("accept");
        close(new_server_sockfd);
        exit(1);
    }
    
    return new_server_sockfd;
}

// Echo input from client
void echo_input(int socket_id) {
    char msg[1024];
    char reply[1024];
    while (1) {
        memset(msg, 0, sizeof(msg));
        memset(reply, 0, sizeof(reply));

        // 5. RECEIVE
        int recv_count;
        if ((recv_count = recv(socket_id, msg, sizeof(msg), 0)) == -1) {
            perror("recv");
            close(socket_id);
            exit(1);
        }
        else if(recv_count==0){
            break;
        }

        findReply(msg, reply);
        printf("%s", reply);
        printf("%s", msg);
        int send_count;
        if ((send_count = send(socket_id, reply, strlen(reply), 0)) == -1) {
            perror("send");
            close(socket_id);
            exit(1);
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Use 2 cli arguments\n");
        return -1;
    }
    
    // extract the address and port from the command line arguments
    char addr[INET6_ADDRSTRLEN];    
    strcpy(addr, argv[1]);
     
    unsigned int port;                
    port = atoi(argv[2]);

    int socket_id = create_connection(addr, port);
    int client_id = client_connect(socket_id);
    echo_input(client_id);
    close(socket_id);
    return 0;    
}

