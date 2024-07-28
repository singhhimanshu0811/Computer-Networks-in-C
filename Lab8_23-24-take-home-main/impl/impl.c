#include "impl.h"
#include <stdio.h>
#define MAX_DV_ENTRIES 256

unsigned char* serialize(struct packet* pkt) {
    /* Todo: Please write the code here*/
    unsigned char *buffer;
    buffer=malloc(3+pkt->num_entries*5 );
    buffer[0]=(unsigned char)(pkt->source_router & 0xFF);
    buffer[1]=(unsigned char)(pkt->dest_router & 0xFF);
    buffer[2]=(unsigned char)(pkt->num_entries & 0xFF);
    int idx=3;int dv_idx=0;
    
    int num_entries=buffer[2];
    while(num_entries--){
        buffer[idx++]=(unsigned char)pkt->distance_vector[dv_idx].target_router;
        buffer[idx++]=(unsigned char)((pkt->distance_vector[dv_idx].cost>>24) & 0xFF);
        buffer[idx++]=(unsigned char)((pkt->distance_vector[dv_idx].cost>>16) & 0xFF);
        buffer[idx++]=(unsigned char)((pkt->distance_vector[dv_idx].cost>>8) & 0xFF);
        buffer[idx++]=(unsigned char)((pkt->distance_vector[dv_idx].cost) & 0xFF);
        dv_idx++;
    }
    return buffer;
}

struct packet *deserialize(unsigned char* buffer) {
    struct packet* pkt = (struct packet*) malloc( sizeof(struct packet));
    pkt->source_router=(int)(buffer[0] & 0xFF);
    pkt->dest_router=(int)(buffer[1] & 0xFF);
    pkt->num_entries=(int)(buffer[2] &0xFF);

    int idx=3;
    int num_entries=pkt->num_entries;
    int dv_idx=0;
    while(num_entries--){
        pkt->distance_vector[dv_idx].target_router=buffer[idx++];
        pkt->distance_vector[dv_idx].cost=(buffer[idx++]<<24) | (buffer[idx++]<<16) | (buffer[idx++]<<8) | (buffer[idx++]);
        dv_idx++;
    }
    return pkt;
}

void router_init(struct distance_table *my_dt, int my_node, int *my_link_costs, int num_nodes)
{
    /* Todo: Please write the code here*/
    for(int i = 0; i < num_nodes; i++){
        my_dt->costs[my_node][i] = my_link_costs[i];
    }
    for(int i = 0; i < num_nodes; i++){
        if(my_link_costs[i] > 0){        // send to connected neighbours
        //j loop is for filling the distance vector for a particular neighbour telling value for all neighbours
            struct packet* pkt = (struct packet*)malloc(sizeof(struct packet));
            pkt->source_router = my_node;
            pkt->dest_router = i;
            pkt->num_entries=0;
            //always initialise all variables of a pointer, after making, dont assume anything
            for(int j = 0; j < num_nodes; j++){  // fill the connected neighbours
               if(my_link_costs[j] > 0){
                    pkt->distance_vector[pkt->num_entries].target_router = j;
                    pkt->distance_vector[pkt->num_entries].cost = my_link_costs[j];
                    pkt->num_entries++;
                }
            } 
            send2neighbor(serialize(pkt));
        }
        
    }
    
}

void router_update(struct distance_table *my_dt, int my_node, unsigned char* packet_buffer, int *my_link_costs, int num_nodes)
{
    /* Todo: Please write the code here*/
    struct packet* pkt=deserialize(packet_buffer);
    struct packet* final_packet=(struct packet*)malloc(sizeof(struct packet));
    final_packet->source_router=my_node;
    final_packet->num_entries=0;
    int src=pkt->source_router;
    int init_entries=pkt->num_entries;
    int flag=1;
   
    for(int i=0;i<init_entries;i++){
        int target_router=pkt->distance_vector[i].target_router;
        int advertised_cost=pkt->distance_vector[i].cost;
        
        if((my_link_costs[src]+advertised_cost < my_dt->costs[my_node][target_router] )|| (my_dt->costs[my_node][target_router]==-1)){
           
            my_dt->costs[my_node][target_router]=my_link_costs[src]+advertised_cost;
            
            final_packet->distance_vector[final_packet->num_entries].cost= my_dt->costs[my_node][target_router];
            final_packet->distance_vector[final_packet->num_entries].target_router=target_router;
           
            final_packet->num_entries = final_packet->num_entries + 1;
            flag=0;
        }
    }
    if(flag==1){
        return;
    }
    int div_idx=0;
    unsigned char* final_buffer;
    for(int i=0;i<num_nodes;i++){
        if(my_link_costs[i]>=0 && i != my_node ){
            final_packet->dest_router=i;
            final_buffer=serialize(final_packet);
            send2neighbor(final_buffer);
        }
    }
}
// int main(){
//     struct packet* pkt = (struct packet*) malloc( sizeof(struct packet));
//     pkt->source_router=1;
//     pkt->dest_router=3;
//     pkt->num_entries=2;
//     pkt->distance_vector[0].target_router=3;
//     pkt->distance_vector[0].cost=128;
//     pkt->distance_vector[1].target_router=2;
//     pkt->distance_vector[1].cost=9876;
//      unsigned char* buffer=serialize(pkt);
//        struct packet *pkt2 =deserialize(buffer); 
//       printf("Packet:  src:%d ,dest:%d num:%d\n",pkt2->source_router,pkt2->dest_router,pkt2->num_entries);
//       for(int i=0;i<pkt2->num_entries;i++){
//         printf("target_router:%d ,cost:%d\n",pkt2->distance_vector[i].target_router,pkt2->distance_vector[i].cost);
//       }
 
//      int c=sizeof(buffer);
//      printf("%d\n",c);
//      //sprintf("size of buffer is:%d\n",c);
//      for(int i=0;i<13;i++){
//         printf("0x%02x\n",buffer[i]);
//      }

// }

