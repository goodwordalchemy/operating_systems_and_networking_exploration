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
#define WINDOW_SIZE 10
#define NUM_ARRIVIUNG_MESSAGES 10

int next_seq_num;
int send_base;

int seq_num_to_window_idx(int seq_num){
    return ((seq_num - 1) / RDT_PKTSIZE) % WINDOW_SIZE;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    next_seq_num = INITIAL_SEQUENCE_NUMBER;
    send_base = INITIAL_SEQUENCE_NUMBER;
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
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

    /* reuse the same packet data structure */
    packet pkt;

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    while (msg->size-cursor > maxpayload_size) {
        /* fill in the packet */
        pkt.data[0] = maxpayload_size;
        pkt.data[1] = next_seq_num;
        printf("sender --> sending seq_num: %d\n", next_seq_num);
        memcpy(pkt.data+header_size, msg->data+cursor, maxpayload_size);

        /* send it out through the lower layer */
        Sender_ToLowerLayer(&pkt);

        /* move the cursor */
        cursor += maxpayload_size;

        /* move the seqence number */
        next_seq_num++;
        next_seq_num %= MAX_SEQ_NUM; 
    }

    /* send out the last packet */
    if (msg->size > cursor) {
        /* fill in the packet */
        pkt.data[0] = msg->size-cursor;
        pkt.data[1] = next_seq_num;
        printf("sender --> sending seq_num: %d\n", next_seq_num);
        memcpy(pkt.data+header_size, msg->data+cursor, pkt.data[0]);

        /* send it out through the lower layer */
        Sender_ToLowerLayer(&pkt);

        /* move the seqence number */
        next_seq_num++;
        next_seq_num %= MAX_SEQ_NUM; 
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    int ack_num = pkt->data[1];

    printf("sender --> received ack_num: %d\n", ack_num);

    if (ack_num != send_base)
        send_base = ack_num;

    printf("sender --> incremented send_base: %d\n", send_base);
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
}
