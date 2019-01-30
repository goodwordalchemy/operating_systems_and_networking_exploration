/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * VERSION: 0.2
 * AUTHOR: Kai Shen (kshen@cs.rochester.edu)
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

#define INITIAL_SEQUENCE_NUMBER 1
#define INITIAL_SEND_BASE 0
#define WINDOW_SIZE 10
#define NUM_ARRIVIUNG_MESSAGES 10

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int next_seq_num;
int send_base;

packet *send_buffer[MAX_SEQ_NUM];

int seq_num_to_window_idx(int seq_num){
    return ((seq_num - 1) / RDT_PKTSIZE) % WINDOW_SIZE;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    next_seq_num = INITIAL_SEQUENCE_NUMBER;
    send_base = INITIAL_SEND_BASE;
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());

    int i;
    for (i = 0; i < MAX_SEQ_NUM; i++)
        if (send_buffer[i] != NULL) 
            free(send_buffer[i]);
}

packet *create_packet(int payload_size, int header_size, int seq_num, char *data){
    packet *pkt = (packet*)malloc(sizeof(pkt));
    pkt->data[0] = payload_size;
    pkt->data[1] = seq_num;
    memcpy(pkt->data+header_size, data, payload_size);

    return pkt;

}

void transmit_n_packets(int n){
    int i;
    packet *pkt;

    for (i = send_base; i < send_base + n; i++){
        if ((pkt = send_buffer[i]) == NULL)
            break;
        Sender_ToLowerLayer(pkt);
    }
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    /* 1-byte header indicating the size of the payload */
    int header_size = 2;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    packet *pkt;

    while (msg->size-cursor > maxpayload_size) {
        /* create a packet */
        printf("sender --> creating packet with seq_num: %d\n", next_seq_num);
        pkt = create_packet(maxpayload_size, header_size, next_seq_num, msg->data+cursor);
        printf("sender --> message content: %s\n", pkt->data);

        /* add packet to buffer */
        send_buffer[next_seq_num] = pkt;

        /* move the cursor */
        cursor += maxpayload_size;

        /* move the seqence number */
        next_seq_num++;
        next_seq_num %= MAX_SEQ_NUM; 
    }

    /* send out the last packet */
    if (msg->size > cursor) {
        /* create a packet */
        printf("sender --> creating packet with seq_num: %d\n", next_seq_num);
        pkt = create_packet(msg->size-cursor, header_size, next_seq_num, msg->data+cursor);
        printf("sender --> message content: %s\n", pkt->data);

        /* add packet to buffer */
        send_buffer[next_seq_num] = pkt;

        /* move the seqence number */
        next_seq_num++;
        next_seq_num %= MAX_SEQ_NUM; 
    }

    transmit_n_packets(WINDOW_SIZE);
    printf("finished transmitting first set of packets\n");
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    int i, diff;
    int ack_num = pkt->data[1];

    printf("sender --> received ack_num: %d\n", ack_num);

    if (ack_num > send_base || (ack_num + 10 > (send_base+10) % MAX_SEQ_NUM)){
        for (i = send_base; i < ack_num; i++){
            free(send_buffer[i]);
            send_buffer[i] = NULL;
        }
        diff = MIN(ack_num - send_base, (ack_num + 10) - ((send_base+10) % MAX_SEQ_NUM));

        send_base = ack_num;

        transmit_n_packets(diff);
    }

    printf("sender --> incremented send_base: %d\n", send_base);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
