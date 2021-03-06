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

#include "common.h"
#include "rdt_struct.h"
#include "rdt_sender.h"

#define INITIAL_SEQUENCE_NUMBER 1
#define INITIAL_SEND_BASE 0
#define INITIAL_NEXT_SEND 1
#define WINDOW_SIZE 10
#define NUM_ARRIVIUNG_MESSAGES 10
#define TIMEOUT 0.3

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int next_seq_num;
int send_base;
int next_send;

packet *send_buffer[MAX_SEQ_NUM];

/* Just in case we overrun the nearly-infinite buffer, which used to be much smaller */
int modulo_greater_than(int a, int b, int modulo, int window){
    if (a > b || (a + window > (b+window) % modulo))
        return 1;
    return 0;
}

/* Useful for debugging */
void print_header(char *data){
    int i;
    for (i = 0; i < HEADER_SIZE; i++)
        printf("header #%d: %d\n", i, data[i]);

}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    next_seq_num = INITIAL_SEQUENCE_NUMBER;
    send_base = INITIAL_SEND_BASE;
    next_send = INITIAL_NEXT_SEND;
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
    for (i = send_base; i < next_seq_num; i++)
        if (send_buffer[i] != NULL) 
            free(send_buffer[i]);
}

packet *create_packet(int payload_size, int seq_num, char *data){
    char checksum[CHECKSUM_SIZE];

    packet *pkt = (packet*)malloc(sizeof(packet));

    pkt->data[0] = payload_size;
    int_to_char(seq_num, pkt->data + 1, 4);
    if (char_to_int(pkt->data + 1, 4) != seq_num){
        printf("integer to character conversion did not work\n");
        exit(1);
    }
    memcpy(pkt->data+HEADER_SIZE, data, payload_size);

    add_checksum_at_index(pkt->data, 5);

    return pkt;

}

void transmit_n_new_packets(int n){
    packet *pkt;

    while (modulo_greater_than(send_base + WINDOW_SIZE, next_send, MAX_SEQ_NUM, WINDOW_SIZE)){
        if ((pkt = send_buffer[next_send]) == NULL)
            break;

        Sender_ToLowerLayer(pkt);

        next_send++;
        next_send = next_send % MAX_SEQ_NUM;
    }
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{
    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - HEADER_SIZE;

    /* split the message if it is too big */

    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;

    packet *pkt;

    while (msg->size-cursor > maxpayload_size) {
        /* create a packet */
        /* printf("sender --> creating packet with seq_num: %d\n", next_seq_num); */
        pkt = create_packet(maxpayload_size, next_seq_num, msg->data+cursor);

        /* add packet to buffer */
        if (send_buffer[next_seq_num] != NULL){
            puts("About to overwrite a packet!");
            exit(1);
        }
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
        /* printf("sender --> creating packet with seq_num: %d\n", next_seq_num); */
        pkt = create_packet(msg->size-cursor, next_seq_num, msg->data+cursor);
        /* printf("sender --> message content: %s\n", pkt->data); */

        /* add packet to buffer */
        if (send_buffer[next_seq_num] != NULL){
            puts("About to overwrite a packet!");
            exit(1);
        }
        send_buffer[next_seq_num] = pkt;

        /* move the seqence number */
        next_seq_num++;
        next_seq_num %= MAX_SEQ_NUM; 
    }

    transmit_n_new_packets(WINDOW_SIZE);

    if (!Sender_isTimerSet())
        Sender_StartTimer(TIMEOUT);
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
    int i;
    int ack_num = char_to_int(pkt->data + 1, 4);

    printf("sender --> received ack_num: %d\n", ack_num);

    if (!check_packet_not_corrupted(pkt->data)){
        fprintf(stderr, "sender --> packet appears to be corrupted\n");
        if (!Sender_isTimerSet())
            Sender_StartTimer(TIMEOUT);
        return;
    }

    if (modulo_greater_than(ack_num, send_base, MAX_SEQ_NUM, WINDOW_SIZE)){
        for (i = send_base; i < ack_num; i++){
            free(send_buffer[i]);
            send_buffer[i] = NULL;
        }

        send_base = ack_num;

        transmit_n_new_packets(1);

        Sender_StopTimer();
        if (modulo_greater_than(next_seq_num, send_base, MAX_SEQ_NUM, WINDOW_SIZE))
            Sender_StartTimer(TIMEOUT);
    }

    /* printf("sender --> incremented send_base: %d\n", send_base); */
}

void retransmit_packets(){
    packet *pkt;
    int i;

    for (i = send_base; i < next_send; i++){
        if ((pkt = send_buffer[i]) == NULL){
            fprintf(stderr, "in flight packet's pointer was found to be null.  This indicates a bug\n");
            exit(1);
        }

        Sender_ToLowerLayer(pkt);
    }
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
    retransmit_packets();
    Sender_StartTimer(TIMEOUT);
}
