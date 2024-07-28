#include "impl.h"
#include <stdio.h>

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
    /* Todo: Please write the code here*/
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
    for(int i=0;i<num_nodes;i++){
        my_dt->next_hop[i]=-1;
    }
    for(int i = 0; i < num_nodes; i++){
        if(my_link_costs[i] > 0){        // send to connected neighbours
        //j loop is for filling the distance vector for a particular neighbour telling value for all neighbours
            struct packet* pkt = (struct packet*)malloc(sizeof(struct packet));
            pkt->source_router = my_node;
            pkt->dest_router = i;
            pkt->num_entries=0;
            my_dt->next_hop[i]=i;
            //always initialise all variables of a pointer, after making, dont assume anything
            for(int j = 0; j < num_nodes; j++){  // fill the connected neighbours
               if(my_link_costs[j] > 0){
                    pkt->distance_vector[pkt->num_entries].target_router = j;
                    pkt->distance_vector[pkt->num_entries].cost = my_link_costs[j];
                    pkt->num_entries++;
                }
            }
           // my_dt 
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
            my_dt->next_hop[target_router]=src;
            
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

int route_packet(struct distance_table *my_dt, int dest_node){
    /* Todo: Please write the code here*/
    return my_dt->next_hop[dest_node];
}

void router_link_update(struct distance_table *my_dt, int my_node, int changed_index, int *my_link_costs, int num_nodes){
    int INT_MAX=999999;
    /* Todo: Please write the code here*/
    //my_dt->costs[my_node][changed_index]=my_link_costs[changed_index];
    for( int k = 0; k < num_nodes; k++ ){
        if( k!= my_node ){
            int min_cost = INT_MAX;
            for( int neig = 0; neig < num_nodes; neig++ ){
                if( my_link_costs[neig] != -1 && neig != my_node && my_dt->costs[neig][k] != -1){
                    // found a neighbour and neighbour has a known path to target
                    int prop_cost = my_link_costs[neig] + my_dt->costs[neig][k];
                    if( prop_cost < min_cost ){
                        min_cost = prop_cost;
                    }
                }
            }
            int curr_cost = my_dt->costs[my_node][k];
            if( min_cost == INT_MAX ){
                min_cost = -1;
            }
            if( min_cost != curr_cost ){
                my_dt->costs[my_node][k] = min_cost;
            }
        }
    }
    struct packet* final_packet=(struct packet*)malloc(sizeof(struct packet));
    final_packet->source_router=my_node;
    final_packet->num_entries=0;
    
    //my_dt->costs[changed_index][my_node]=my_link_costs[changed_index];
    //printf("%d ok\n", my_dt->costs[1][2]);
    int flag=1;
    for(int i=0;i<num_nodes;i++){
        if(my_link_costs[i]<0)continue;
        //final_packet->num_entries++;
        int target_router=changed_index;
        int advertised_cost=my_dt->costs[i][target_router];
        if(i==target_router)continue;
        if(advertised_cost<0){
            //printf("%d, %d, %d fuck off pls\n", i, target_router, my_link_costs[i]);
            continue;
        }
        //printf("%d, %d, %d, %d, %d\n", i, my_node, target_router, advertised_cost, my_dt->costs[my_node][target_router]);
         if((my_link_costs[i]+advertised_cost < my_link_costs[target_router] )|| (my_dt->costs[my_node][target_router]==-1)){
    
            final_packet->distance_vector[final_packet->num_entries].cost= my_dt->costs[my_node][target_router];
            final_packet->distance_vector[final_packet->num_entries].target_router=target_router;
           
            final_packet->num_entries = final_packet->num_entries + 1;
            
            my_dt->costs[my_node][target_router]=my_link_costs[i]+advertised_cost;
            my_dt->next_hop[target_router]=i;
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
