#include "header.h"

int main(int argc, char *argv[])
{
    struct event *eventptr;

    FILE *file = fopen(argv[2], "r");
    char ch;
    int max_iterations = atoi(argv[1]);
    num_nodes = getLineCount(file);
    dts = (struct distance_table *) malloc(num_nodes * sizeof(struct distance_table));
    link_costs = (int **) malloc(num_nodes * sizeof(int *));
    for (int i = 0; i < num_nodes; i++)
    {
        link_costs[i] = (int *)malloc(num_nodes * sizeof(int));
        for(int j = 0; j < num_nodes; j++){
            fscanf(file, "%d", &link_costs[i][j]);
        }
    }
    FILE *file1 = fopen(argv[3], "r");
    int num_traffic = getLineCount(file1);
    for(int i = 0; i < num_traffic; i++){
        int traffic[3];
        for(int j=0;j<3;j++){
            fscanf(file1, "%d", &traffic[j]);
        }
        struct event* evptr = (struct event *)malloc(sizeof(struct event));
        evptr->eventity = traffic[0];
        evptr->dest_node = traffic[1];
        evptr->evtime = traffic[2];
        evptr->evtype = ROUTE;
        insertevent(evptr);
    }
    FILE *file2 = fopen(argv[4], "r");
    int num_link_update = getLineCount(file2);
    for(int i = 0; i < num_link_update; i++){
        int link_update[4];
        for(int j=0;j<4;j++){
            fscanf(file2, "%d", &link_update[j]);
        }
        struct event* evptr = (struct event *)malloc(sizeof(struct event));
        evptr->eventity = link_update[0];
        evptr->dest_node = link_update[1];
        evptr->evtime = link_update[3];
        evptr->evtype = LINK_UPDATE;
        evptr->cost = link_update[2];
        insertevent(evptr);
    }
    for(int i=0; i<max_iterations; i++){
        struct event* evptr = (struct event *)malloc(sizeof(struct event));
        evptr->evtime = i;
        evptr->evtype = DUMMY;
        insertevent(evptr);
    }
    for (int i = 0; i < num_nodes; i++)
    {
        dts[i].costs = (int**) malloc(num_nodes*sizeof(int*));
        dts[i].next_hop = (int*) malloc(num_nodes*sizeof(int));
        for(int j=0;j<num_nodes;j++){
            dts[i].costs[j] = (int *)malloc(num_nodes * sizeof(int));
            for(int k=0;k<num_nodes;k++){
                if(j==k)
                    dts[i].costs[j][k] = 0;
                else
                    dts[i].costs[j][k] = -1;
            }
            dts[i].next_hop[j] = -1;
        }
        router_init(&dts[i], i, link_costs[i], num_nodes);
    }
    int k = 0; 
    while (1) 
    {
        // printevlist();
        eventptr = evlist;
        if (eventptr==NULL || k==max_iterations)
            break;
        evlist = evlist->next;
        if (evlist!=NULL)
            evlist->prev=NULL;
        clocktime = eventptr->evtime;
        if(k<clocktime){
            printf("k=%d:\n",k);
            printdts();
            k++;
        }
        if(eventptr->evtype == ROUTER_UPDATE){
            router_update(&dts[eventptr->eventity], eventptr->eventity, eventptr->packet_buffer_ptr, link_costs[eventptr->eventity], num_nodes);
            free(eventptr->packet_buffer_ptr);
        }
        else if(eventptr->evtype == ROUTE){
            int hops = 0;
            int next_hop = -1;
            int cur_node = eventptr->eventity;
            printf("Routing from %d to %d: %d", cur_node, eventptr->dest_node, cur_node);
            while(next_hop != eventptr->dest_node){
                next_hop = route_packet(&dts[cur_node], eventptr->dest_node);
                if(next_hop == -1){
                    printf("> No forwarding entry for %d from %d", eventptr->dest_node, cur_node);
                    break;
                }
                if(link_costs[cur_node][next_hop]==-1){
                    printf("\nWARNING: No link from %d to %d\n", cur_node, next_hop);
                    exit(1);
                }
                hops++;
                if(hops>MAX_HOPS){
                    printf("\nWARNING: Exceeded max hops in routing\n");
                    exit(1);
                }
                printf(">%d", next_hop);
                cur_node = next_hop;
            }
            printf("\n");
        }
        else if(eventptr->evtype==LINK_UPDATE){
            link_costs[eventptr->eventity][eventptr->dest_node] = eventptr->cost;
            link_costs[eventptr->dest_node][eventptr->eventity] = eventptr->cost;
            router_link_update(&dts[eventptr->eventity], eventptr->eventity, eventptr->dest_node, link_costs[eventptr->eventity], num_nodes);
        }
        free(eventptr);
    }

    while(k<=max_iterations){
        printf("k=%d:\n",k);
        printdts();
        k++;
    }
    return 0;
}
