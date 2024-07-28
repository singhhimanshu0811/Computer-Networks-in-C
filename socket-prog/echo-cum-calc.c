#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

void findReply(char* msg, char* reply){
    int a, b;
    char op;
    if(sscanf(msg, "%d%c%d", &a, &op, &b) == 3){
        switch(op){
            case '+':
                sprintf(reply, "%d\n", a+b);
                break;
            case '-':
                sprintf(reply, "%d\n", a-b);
                break;
            case '*':
                sprintf(reply, "%d\n", a*b);
                break;
            case '/':
                sprintf(reply, "%d\n", a/b);
                break;
            default:
                strcpy(reply, msg);
        }
    }
    else{
        strcpy(reply, msg);
    }
}

void server(char* argv[]){
    int server_sockfd, new_server_sockfd, recv_count, send_count;
    struct sockaddr_in server_addrinfo, client_addrinfo;
    socklen_t sin_size;
    int yes = 1;
    char msg[1024];
    char reply[1024];

    char server_IP[INET6_ADDRSTRLEN];
    unsigned int server_port;
    char client_IP[INET6_ADDRSTRLEN];
    unsigned int client_port;

    strcpy(server_IP, argv[1]);
    server_port = atoi(argv[2]);

    // 1. SOCKET
    if((server_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
        perror("server: socket");
        exit(1);
    }

    // SockOptions
    if(setsockopt(server_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1){
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

    // 2. BIND
    if(bind(server_sockfd, (struct sockaddr*) &server_addrinfo, sizeof(server_addrinfo)) == -1){
        perror("server: bind");
        close(server_sockfd);
        exit(1);
    }
    
    // 3. LISTEN
    if(listen(server_sockfd, 3) == -1){
        perror("listen");
        close(server_sockfd);
        exit(1);
    }

    server_port = ntohs(server_addrinfo.sin_port);
    printf("%s:%u Server: Waiting for new connections...\n", server_IP, server_port);


    sin_size = sizeof(client_addrinfo);

    // 4. ACCEPT
    new_server_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addrinfo, &sin_size);
    if(new_server_sockfd == -1){
        perror("accept");
        close(server_sockfd);
        exit(1);
    }



    if(inet_ntop(client_addrinfo.sin_family, &client_addrinfo.sin_addr, client_IP, sizeof(client_IP)) <= 0){
        printf("\nAddress Conversion Error\n");
        close(server_sockfd);
        close(new_server_sockfd);
        exit(1);
    }
    client_port = ntohs(client_addrinfo.sin_port);
    printf("Server: Got connection from %s:%u\n", client_IP, client_port);
    
    // COMMUNICATION
    while(1){
        memset(msg, 0, 1024);
        memset(reply, 0, 1024);

        // 5. RECEIVE
        if((recv_count = recv(new_server_sockfd, msg, 1024, 0)) == -1){
            perror("recv");
            close(server_sockfd);
            close(new_server_sockfd);
            exit(1);
        }

        findReply(msg, reply);
        
        // 6. SEND
        if((send_count = send(new_server_sockfd, reply, strlen(reply), 0)) == -1){
            perror("send");
            close(server_sockfd);
            close(new_server_sockfd);
            exit(1);
        }
        printf("Client messaged: %s\n", msg);
        printf("Server echoed: %s\n\n", reply);
        
        if(strcmp(msg, "EXIT") == 0){
            close(new_server_sockfd);

            printf("%s:%u Server: Waiting for new connections...\n", server_IP, server_port);
            sin_size = sizeof(client_addrinfo);
            
            new_server_sockfd = accept(server_sockfd, (struct sockaddr*) &client_addrinfo, &sin_size);
            if(new_server_sockfd == -1){
                close(server_sockfd);
                perror("accept");
                exit(1);
            }

            if(inet_ntop(client_addrinfo.sin_family, &client_addrinfo.sin_addr, client_IP, sizeof(client_IP)) <= 0){
                printf("\nAddress Conversion Error\n");
                close(server_sockfd);
                close(new_server_sockfd);
                exit(1);
            }
            client_port = ntohs(client_addrinfo.sin_port);
            printf("Server: Got connection from %s:%u\n", client_IP, client_port);
        }
    }
    
    // 7. CLOSE
    close(server_sockfd);
}

void client(char* argv[]){
    int client_sockfd, recv_count, send_count;
    struct sockaddr_in server_addrinfo, client_addrinfo;
    char msg[1024];
    char reply[1024];

    char client_IP[INET6_ADDRSTRLEN];
    unsigned int client_port;
    char serverIP[INET6_ADDRSTRLEN];
    unsigned int server_port;
    

    strcpy(serverIP, argv[1]);
    server_port = atoi(argv[2]);

    // 1. SOCKET
    if((client_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
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
        perror("client: connect");
        close(client_sockfd);
        exit(1);
    }

    
    // get client details
    memset(&client_addrinfo, 0, sizeof(client_addrinfo));
    socklen_t len = sizeof(client_addrinfo);
    getsockname(client_sockfd, (struct sockaddr*) &client_addrinfo, &len);
    if(inet_ntop(client_addrinfo.sin_family, &client_addrinfo.sin_addr, client_IP, sizeof(client_IP)) <= 0){
        printf("\nAddress Conversion Error\n");
        close(client_sockfd);
        exit(1);
    }
    client_port = ntohs(client_addrinfo.sin_port);

    printf("%s:%u is connected to %s:%u\n", client_IP, client_port, serverIP, server_port);


    // COMMUNICATION
    while(1){
        memset(msg, 0, 1024);
        memset(reply, 0, 1024);

        printf("Client: ");
        // User-input
        // fgets(msg, 1024, stdin);
        scanf("%s", msg);

        // 3. SEND
        if((send_count = send(client_sockfd, msg, strlen(msg), 0)) == -1){
            perror("send");
            close(client_sockfd);
            exit(1);
        }

        // 4. RECEIVE
        if((recv_count = recv(client_sockfd, reply, 1024, 0)) == -1){
            perror("recv");
            exit(1);
        }
        printf("Server: %s\n", reply);
        if(strcmp(reply, "EXIT") == 0){
            // 5. CLOSE
            close(client_sockfd);
            break;
        }
    }
     
}

int main(int argc, char* argv[]){
    if(argc != 4){
        printf("Format: <./exe> <IP> <PORT> <s/c>\n");
        exit(1);
    }

    if(strcmp(argv[3], "s") == 0){
        server(argv);
        
    }
    else if(strcmp(argv[3], "c") == 0){
        client(argv);
    }
    else{
        printf("Wrong inputs\n");   
    }
    return 0;
}








