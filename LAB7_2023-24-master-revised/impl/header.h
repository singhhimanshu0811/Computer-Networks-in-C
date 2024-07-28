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

struct packet {
    int version;
    int headerLength;
    int totalLength;
    int srcDept;
    int destDept;
    int checkSum;
    int hops;
    int type;
    int ACK;
    int srcCampus;
    int destCampus;
    char data[1024];
};

//You can add the implementations of functions in helper.c to avoid code cluttering

void printPacket(struct packet *p);
unsigned char* serialize( struct packet* p);
struct packet *deserialize(unsigned char* buffer);

struct packet *generatePacket(int version, int headerLength, int totalLength, 
                              int srcDept, int destDept, int checkSum, int hops, 
                              int type, int ACK, int srcCampus, int destCampus, char* data);

unsigned char* generateUnicastPacket(char* input, int srcCampus, int srcDept, int *validDept[3], int numofValidDept[3]);
unsigned char* generateBroadcastPacket(char* input, int srcCampus, int srcDept, int *validDept[3], int numofValidDept[3]);
unsigned char* generateControlPacket(char* input, int srcCampus, int srcDept, int *validDept[3], int numofValidDept[3]);