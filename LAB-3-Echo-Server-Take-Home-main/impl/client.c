#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


// Create a connection to the server
int create_connection(char* addr, int port) {
   int client_sockfd;
   struct sockaddr_in server_addrinfo;

    char serverIP[INET6_ADDRSTRLEN];
    unsigned int server_port;
    strcpy(serverIP, addr);
    server_port = (port);

    if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        close(client_sockfd);
        perror("client: socket");
        exit(1);
    }

    // NO BIND NECESSARY IN CLIENT!!!

    server_addrinfo.sin_family = AF_INET;
    server_addrinfo.sin_port = htons(server_port);
    if (inet_pton(AF_INET, serverIP, &server_addrinfo.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        close(client_sockfd);
        exit(1);
    }

    // 2. CONNECT
    if(connect(client_sockfd, (struct sockaddr*)&server_addrinfo, sizeof(server_addrinfo)) == -1){ // client connects if server port has started listen()ing and queue is non-full; however server connects to client only when it accept()s
        printf("client: connect");
        close(client_sockfd);
        exit(1);
    }
    
    return client_sockfd;

}

void send_data(int socket_id, FILE* file) {
    char msg[1024];
    	memset(msg, 0, 1024);
     
    	// Take input from user
     
    	fgets(msg, 1024, stdin);
        fprintf(file, "CLIENT:ECHO:");	
        sprintf(file, "%s", msg);	
    	fprintf(file, "\n");
    	if(strcmp(msg, "EXIT\n") == 0)	
    	{
    		// 5. CLOSE
    		printf("Client exited successfully");
    		close(socket_id);
    		exit(0);
    	}
     
    	// 3. SEND
    	int send_count;
    	if((send_count = send(socket_id, msg, strlen(msg), 0)) == -1)	
    	{
    		perror("send");
    		exit(1);
    	}
}

// Receive input from the server
void recv_data(int socket_id, FILE* file) {
    char reply[1024];
    	memset(reply, 0, 1024);
     
    	int recv_count;
    	if((recv_count = recv(socket_id, reply, 1024, 0)) == -1)	
    	{
    		perror("recv");
    		exit(1);
    	}
        fprintf(file, "SERVER:OHCE:");	
        sprintf(file, "%s", reply);	
    	fprintf(file, "\n");
}

int main(int argc, char *argv[])
{
    if (argc != 3)
	{
		printf("Use 2 cli arguments\n");
		return -1;
	}
    
	// extract the address and port from the command line arguments
    char addr[INET6_ADDRSTRLEN];	
    	strcpy(addr, argv[1]);
     
    	unsigned int port;				
    	port = atoi(argv[2]);

	int socket_id = create_connection(addr, port);
    FILE *file=fopen("client_file.txt", 'w');

	while (1)
    {
        send_data(socket_id, file);
        recv_data(socket_id, file);
    }
    fclose(file);
}
