#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFF_SIZE 1024 // Maximum buffer size

// Create a connection to the server
int create_connection(char *addr, int port)
{
    int sockfd;
    struct sockaddr_in server_addr;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Fill server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, addr, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    int connect_status = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (connect_status < 0)
    {
        fprintf( stdout, "Could not find server\n");
        fflush( stdout );
        exit(EXIT_FAILURE);
    }

    return sockfd;
}

// Receive input from the server, writes in the buffer, clears buffer before writing
void recv_data(int socket_id, char *buffer)
{
    memset(buffer, 0, BUFF_SIZE);
    int bytes_received;

    // Receive data from the server
    bytes_received = recv(socket_id, buffer, sizeof(buffer), 0);
    if (bytes_received == -1)
    {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    else if (bytes_received == 0)
    {
        // Connection closed by the server
        fprintf( stdout, "64 Connection closed by the server\n");
        fflush( stdout);
        exit(EXIT_SUCCESS);
    }
    else
    {
        // Data received successfully
        fprintf( stdout, "Received from server: %s\n", buffer);
        fflush( stdout );
    }
}

/*
This is the first command sent by the server to a new client.
When the client receives this command it must print "INITIALIZING......\n"
Once the above string has been printed in the client's terminal it should automatically send its username (provided as a command line input) to the server in the format “\n”
*/
int take_n_send_name(char *name, int socket_fd)
{
    fprintf(stdout, "INITIALIZING......\n");
    fflush(stdout);
    char name_new_lin[BUFF_SIZE] = {0};
    sprintf(name_new_lin, "%s\n", name);
    // send the name
    int bytes_sent = send(socket_fd, name_new_lin, strlen(name_new_lin), 0);
    if (bytes_sent < 0)
    {
        return bytes_sent;
    }
    return 0;
}

int handle_polling(int socket_fd);

// reading the list of people, and any POLL if there is one stuck
// get result from server
int handle_list(int socket_fd)
{
    char buffer[BUFF_SIZE] = {0};
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0)
    {
        return bytes_received;
    }
    else if (bytes_received == 0)
    {
        // Connection closed by the server
        fprintf(stdout, "Connection closed by the server\n");
        fflush( stdout);
        return -1*ENOTCONN;
    }
    else
    {
        // received list data
        char *names = strtok(buffer, "\n");
        char *next_comm = strtok(NULL, "\n");

        char to_print[BUFF_SIZE] = {0};
        int count = 1;
        for (char *n = strtok(names, ":"); n != NULL; n = strtok(NULL, ":"))
        {
            char to_app[BUFF_SIZE] = {0};
            sprintf( to_app, "%d. %s\n", count,n);
            strcat(to_print, to_app);
            count++;
        }
        // to_print[ strlen(to_print)-1 ] = 0;
        fprintf(stdout, "%s", to_print);
        fflush( stdout);

        if (NULL == next_comm)
        {
            return 0;
        }

        // POLL is here
        return handle_polling(socket_fd);
    }
}

/*
This command is sent by the server to the client when it is that client’s chance to send a message to the server.
The polling algorithm is run to identify the next client and this message must be sent to the client.
When the client receives the “POLL\n” command from the server it must print “ENTER CMD: ” in its terminal to indicate to the user that the client is waiting for an input.
*/
int handle_polling(int socket_fd)
{
    fprintf(stdout, "ENTER CMD: ");
    fflush( stdout);
    char client_cmd[BUFF_SIZE];
    // reads in at most one less than size characters from stream and stores them into the buffer pointed to by s
    //  stores the \n char as well
    fgets(client_cmd, BUFF_SIZE, stdin);
    int bytes_sent = send(socket_fd, client_cmd, strlen(client_cmd), 0);
    if (bytes_sent < 0)
    {
        perror("Send failed");
        exit(EXIT_FAILURE);
        return bytes_sent;
    }
    else if( bytes_sent == 0){
        return -1*ENOTCONN;
    }

    // If client wanted to exit
    // After sending this command to the server, the client must print “CLIENT TERMINATED: EXITING……\n” before exiting.
    if (0 == strcmp(client_cmd, "EXIT\n"))
    {
        fprintf(stdout, "CLIENT TERMINATED: EXITING......\n");
        fflush( stdout );
        exit(EXIT_SUCCESS);
    }

    // NOOP, MESG well carry on.
    if (0 == strcmp(client_cmd, "NOOP\n") 
        || 0 == strncmp(client_cmd, "MESG:", 5)
    )
    {
        return 0;
    }

    // LIST\n
    if (0 == strcmp(client_cmd, "LIST\n"))
    {
        return handle_list(socket_fd);
    }

    // reached here, therefore client sent a bad command
    // keep return as 0, reserve negatives for critical/crashing errors
    return 0;
}

int transact(int socket_fd, char *name)
{
    char buffer[BUFF_SIZE] = {0};
    int bytes_received = recv(socket_fd, buffer, sizeof(buffer), 0);
    if (bytes_received < 0)
    {
        return bytes_received;
    }
    else if (bytes_received == 0)
    {
        // Connection closed by the server
        fprintf( stdout, "Connection closed by the server\n");
        fflush( stdout);
        return -1*ENOTCONN;
    }
    else
    {

        // Data received successfully

        if (0 == strcmp(buffer, "NAME\n"))
        {
            return take_n_send_name(name, socket_fd);
        }
        else if (0 == strcmp(buffer, "POLL\n"))
        {
            return handle_polling(socket_fd);
        }
        else
        {
            return -1*EBADMSG;
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf( stdout, "Use 3 cli arguments\n");
        fflush( stdout);
        return -1;
    }

    char *addr = argv[1];
    int port = atoi(argv[2]);
    char *name = argv[3];
    // extract the address and port from the command line arguments

    int server_fd = create_connection(addr, port);
    if (0 > server_fd)
    {
        fprintf(stderr, "Error:%s", strerror(server_fd));
        fflush( stdout);
        return -1;
    }

    while (1)
    {
        int trn_ret = transact(server_fd, name);
        if (trn_ret < 0)
        {
            perror(strerror(-1*trn_ret));
            return trn_ret;
        }
    }

    return 0;
}
