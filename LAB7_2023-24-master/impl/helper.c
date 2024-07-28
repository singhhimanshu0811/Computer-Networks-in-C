#include "header.h"

void printPacket(struct packet *p){  //always follow this format for printing the packet
    printf("{\n");
    printf("    Version: %d\n", p->version);
    printf("    Header Length: %d\n", p->headerLength);
    printf("    Total Length: %d\n", p->totalLength);
    printf("    Source Department: %d\n", p->srcDept);
    printf("    Destination Department: %d\n", p->destDept);
    printf("    CheckSum: %d\n", p->checkSum);
    printf("    Hops: %d\n", p->hops);
    printf("    Type: %d\n", p->type);
    printf("    ACK: %d\n", p->ACK);
    if(p->headerLength == 6){
        printf("    Source Campus: %d\n", p->srcCampus);
        printf("    Destination Campus: %d\n", p->destCampus);
    }
    printf("    Data: %s\n", p->data);
    printf("}\n");
}

unsigned char* serialize( struct packet* p){
    unsigned char *buffer;
    size_t dataSize = strlen(p->data);
    size_t size=sizeof(int)*10+20+sizeof(char)*1024+dataSize;
    buffer=(unsigned char*)malloc(size);
    buffer[0]=(p->version<<4)|(p->headerLength & 0x0F);
    buffer[1]=(p->totalLength & 0xFF);
    buffer[2]=(p->srcDept<<5)|(p->destDept<<2)|(p->checkSum >> 8) & 0b11;
    buffer[3]=(p->checkSum & 0xFF);
    buffer[4]=(p->hops <<5)|(p->type <<2)|p->ACK;
    if(p->headerLength==6){
        buffer[5]=(p->srcCampus<<4)|(p->destCampus);
        if (dataSize > 0) {
            memcpy(buffer + 6, p->data, dataSize);
            p->data[dataSize]='\0';
        }
    }
    else{
        if (dataSize > 0) {
            memcpy(buffer + 5, p->data, dataSize);
            p->data[dataSize]='\0';
        }
    }
    
    
    return buffer;
}

struct packet *deserialize(unsigned char* buffer){
    struct packet *p;
    p->version=(buffer[0]>>4) & 0x0F;
    p->headerLength=buffer[0] & 0x0F;
    p->totalLength=buffer[1];
    p->srcDept=(buffer[2]>>5) & 0x07;
    p->destDept=(buffer[2]>>2) & 0x07;
    p->checkSum=((buffer[2] & 0x03)<<8) | buffer[3];
    p->hops=(buffer[4]>>5) & 0x07;
    p->type=(buffer[4]>>2) & 0x07;
    p->ACK=buffer[4]&0x03;
    p->srcCampus=-1;
    p->destCampus=-1;
    if(p->headerLength==6){
        p->srcCampus=(buffer[5] >> 4) & 0x0F;
        p->destCampus=buffer[5]& 0x0F;
        strcpy(p->data, buffer+6);
        p->data[strlen(p->data)-1]='\0';
    }
    else{
        strcpy(p->data, buffer+5);
        p->data[strlen(p->data)-1]='\0';
    }
    
    return p;
} 

struct packet *generatePacket(int version, int headerLength, int totalLength, 
                              int srcDept, int destDept, int checkSum, int hops, 
                              int type, int ACK, int srcCampus, int destCampus, char* data) {
    //feel free to write your own function with more customisations. This is a very basic func 
    struct packet *p;
    p = (struct packet *)malloc(sizeof(struct packet));
    p->version = version;
    p->headerLength = headerLength;
    p->totalLength = totalLength;
    p->srcDept = srcDept;
    p->destDept = destDept;
    // p->checkSum = (version+headerLength+totalLength+srcDept+destDept+hops+type+ACK);
    // if(headerLength==6){
    //     p->checkSum+=(srcCampus+destCampus);
    // }
    // int length = strlen(data);
    // for (int i = 0; i < length; i++) {
    //     p->checkSum += data[i];
    //     while (p->checkSum >> 8) {
    //         p->checkSum = (p->checkSum & 0xFF) + (p->checkSum >> 8);
    //     }
    // }
        
    // p->checkSum=~p->checkSum;
    p->checkSum=checkSum;
    p->hops = hops;
    p->type = type;
    p->ACK = ACK;
    p->srcCampus = srcCampus;
    p->destCampus = destCampus;
    strcpy(p->data, data);
    return p;
}
