#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bitfield.h"
#include "logging_utils.h"
#include "messages.h"
#include "pieces.h"
#include "socket_helpers.h"
#include "state.h"

#define DEBUG 1
#define N_INTEGER_BYTES 4

#if defined(DEBUG) && DEBUG > 0
 #define DEBUG_PRINT(fmt, args...) fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt, \
    __FILE__, __LINE__, __func__, ##args)
#else
 #define DEBUG_PRINT(fmt, args...) /* Don't do anything in release builds */
#endif

void print_my_status(){
    int i, bitfield, cur, n_pieces, downloaded, left, uploaded;
    char *headers[] = {"Bitfield", "Downloaded", "Uploaded", "Left"};
    int n_headers = 4;
    
    printf("My status:\n");
    printf("\t");
    for (i = 0; i < n_headers; i++){
        print_str_cell(headers[i]);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

    n_pieces = localstate.n_pieces;
    bitfield = what_is_my_bitfield();

    printf("\t");
    downloaded = 0;
    for (i = 0; i < n_pieces; i++){
        cur = bitfield >> (n_pieces - 1 - i) & 1;
        printf("%d", cur);
        if (cur)
            downloaded++;
    }
    printf("%*s | ", COLUMN_WIDTH - n_pieces, "");

    uploaded = 0; // Nobody has uploaded anything to anyone yet!
    left = n_pieces - downloaded;

    print_int_cell(downloaded);
    print_int_cell(uploaded); 
    print_int_cell(left);
    printf("\n");
    
    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

}

void encode_int_as_char(int num, char *buf, int length){
    int i;
    for (i = 0; i < length; i++){
        buf[i] = (char)(num >> ((8 * (length - i - 1)) & 0xFF));
    }
}

int decode_int_from_char(char *enc, int length){
    int i, num, cur;

    num = 0;
    for (i = 0; i < length; i++){
        cur = (unsigned char) enc[i] << (8 * (length - i - 1));
        num += (int) cur;
    }

    return num;
}

int send_peer_message(int sockfd, msg_t *msg){
    char *buf;
    int full_length, nbytes;

    full_length = msg->length + N_INTEGER_BYTES + 1;

    buf = malloc(sizeof(char) * full_length);

    encode_int_as_char(msg->length, buf, N_INTEGER_BYTES);

    buf[N_INTEGER_BYTES] = msg->type;

    memcpy(buf + N_INTEGER_BYTES + 1, msg->payload, msg->length - 1);

    DEBUG_PRINT("sending message of type %d\n", msg->type);

    nbytes = send_on_socket(sockfd, buf, full_length);

    if (nbytes != full_length)
        fprintf(stderr, "SENDING PEER MSG to socket %d.  meant to send %d, only send %d bytes\n", sockfd, full_length, nbytes);
    
    free(buf);

    return nbytes;
}


int send_bitfield_message(int sockfd){
    int bitfield;
    int n_bitfield_bytes = how_many_bytes_in_my_bitfield();
    char bf_buf[n_bitfield_bytes];
    msg_t msg;
    

    // create encoded bitfield
    bitfield = what_is_my_bitfield();
    bitfield = bitfield << how_many_shift_bits_in_my_bitfield();

    encode_int_as_char(bitfield, bf_buf, n_bitfield_bytes);

    // create message struct
    msg.length = n_bitfield_bytes + 1;
    msg.type = BITFIELD;
    msg.payload = bf_buf;

	DEBUG_PRINT("Sending BITFIELD message to sockfd: %d\n", sockfd);

    // send message struct
    return send_peer_message(sockfd, &msg);
}

void print_peer_bitfields(){
    int i;
    peer_t *p;
    char *headers[] = {"Peer Id", "Status", "Bitfield", "Down/s", "Up/s"};
    int n_headers = 5;
    char peer_id_cell[SHA_DIGEST_LENGTH+4];

    printf("Peer bitfields:\n");

    printf("\t");
    for (i = 0; i < n_headers; i++){
        print_str_cell(headers[i]);
    }
    printf("\n");

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));

    for (i = 4; i<=localstate.max_sockfd; i++){
        if ((p = localstate.peers[i]) == NULL){
            continue;
		}

        memset(peer_id_cell, 0, SHA_DIGEST_LENGTH + 4);
        memcpy(peer_id_cell, p->peer_id, SHA_DIGEST_LENGTH);
        sprintf(peer_id_cell + SHA_DIGEST_LENGTH, "-%d", i);

		printf("\t");
        print_str_cell(peer_id_cell);
        print_str_cell("0101");
        print_bitfield_cell(p->bitfield);
        print_int_cell(0);
        print_int_cell(0);
		printf("\n");
    }

    print_horizontal_line(n_headers * (3 +COLUMN_WIDTH));
}

peer_t *get_peer(int sockfd){
    return localstate.peers[sockfd];
}

int handle_bitfield_message(int sockfd, msg_t *msg){
    // If bitfield message validates, then it will set the bitfield field on the peer struct at the sockfd.
    peer_t *p;
    int bitfield;

    // For receiving messsage
    int n_bitfield_bytes = how_many_bytes_in_my_bitfield();;

    // for parsing message
    int n_shift_bits = how_many_shift_bits_in_my_bitfield();
	
	DEBUG_PRINT("received BITFIELD message on sockfd: %d\n", sockfd);

    if (msg->length != n_bitfield_bytes + 1){
        fprintf(stderr, "Expected length prefix %d.  Instead got %d\n", n_bitfield_bytes + 1, msg->length);
        return -1;
    }

    if (msg->type != BITFIELD){
        fprintf(stderr, "expected message type of BITFIELD=5 from peer. Instead got %d\n", msg->type);
        return -1;
    }

    bitfield = decode_int_from_char(msg->payload, n_bitfield_bytes);

    if ((bitfield & ((int) pow((double) 2, n_shift_bits) - 1)) > 0){
        fprintf(stderr, "trailing zeros were set in peers bitfield\n");
        return -1;
    }

    bitfield = bitfield >> how_many_shift_bits_in_my_bitfield();

    p = get_peer(sockfd);
    p->bitfield = bitfield;
    p->cleared_bitfield = 1;

    return 0;
}

int bitfield_has_piece(int bitfield, int index){
    int power;

    power = (int)pow((double)2, localstate.n_pieces - 1 - index);

    if (power & bitfield)
        return 1;
    return 0;
}

int peer_has_piece(int sockfd, int index){
    int bitfield;

    bitfield = localstate.peers[sockfd]->bitfield;

    return bitfield_has_piece(bitfield, index);
}

int i_have_piece(int index){
    int bitfield;

    bitfield = what_is_my_bitfield();

    return bitfield_has_piece(bitfield, index);
}


int send_request_message(int sockfd, int piece){
    msg_t msg;
    char data[12];

    encode_int_as_char(piece, data, 4);
    encode_int_as_char(0, data + 4, 4);
    encode_int_as_char(localstate.piece_length, data + 8, 4);

    msg.length = 13;
    msg.type = REQUEST;
    msg.payload = data;

    if (send_peer_message(sockfd, &msg) <= 0){
        fprintf(stderr, "could not send piece request.");
        return 0;
    }

    localstate.peers[sockfd]->last_contact = get_epoch_time();
    localstate.peers[sockfd]->requested_piece = piece;

    DEBUG_PRINT("sending REQUEST message to sockfd: %d for piece: %d\n", sockfd, piece);

    return 1;
}

void send_request_messages(){
    int mybitfield, i, j;
    peer_t *p;

    mybitfield = what_is_my_bitfield();

    for (i = 0; i < localstate.n_pieces; i++){
        if (i_have_piece(i))
            continue;

        for (j = 0; j <= localstate.max_sockfd; j++){
            p = localstate.peers[j];
            if (p == NULL || !peer_has_piece(j, i))
                continue;
            if (p->requested_piece == -1){
                send_request_message(j, i);
                break;
            }
        }
    }
}

int send_piece_message(int sockfd, int piece_idx){
    msg_t msg;
    FILE *f;
    int begin, piece_length, nbytes;
    char *piece_hash_digest, *payload_buf;
    char piece_filename[FILENAME_WITH_EXT_BUFLEN];
    
    DEBUG_PRINT("sending a PIECE message sockfd: %d with index: %d\n", sockfd, piece_idx);

    msg.type = PIECE;

    piece_hash_digest = localstate.piece_hash_digests[piece_idx];
    get_filename_with_extension(piece_hash_digest, piece_filename);

    if (piece_idx == localstate.n_pieces-1)
        piece_length = localstate.last_piece_size;
    else
        piece_length = localstate.piece_length;

    msg.length = 1 + 4 + 4 + piece_length; // msg_id + index + begin + block

    if ((payload_buf = malloc((msg.length) * sizeof(char))) == NULL){
        perror("malloc");
        return -1;
    }

    encode_int_as_char(piece_idx, payload_buf, N_INTEGER_BYTES);

    begin = 0; // Maybe later I'll implement block sizes other than piece size...
    encode_int_as_char(begin, payload_buf + 4, N_INTEGER_BYTES);

    if ((f = fopen(piece_filename, "r")) == NULL){
        perror("fopen");
        free(payload_buf);
        return -1;
    }

    if (fread(payload_buf+8, 1, piece_length, f) == 0){
        perror("fread");
        free(payload_buf);
        return -1;
    }

    if (fclose(f) == EOF){
        perror("fclose");
        free(payload_buf);
        return -1;
    }

    msg.payload = payload_buf;

    nbytes = send_peer_message(sockfd, &msg);

    free(payload_buf);

    return nbytes;
}

int handle_request_message(int sockfd, msg_t *msg){
    int index, begin, length;

    if (msg->length != 13){
        fprintf(stderr, "Request message did not specify correct length. Should be 13, but it was %d.\n", msg->length);
        return -1;
    }

    index = decode_int_from_char(msg->payload, 4);
    begin = decode_int_from_char(msg->payload + 4, 4);
    length = decode_int_from_char(msg->payload + 8, 4);
    
    DEBUG_PRINT("Received REQUEST on sockfd %d for piece %d\n", sockfd, index);

    if (begin != 0){
        fprintf(stderr, "Have not implemented sending blocks that aren't the first in their piece\n");
        return -1;
    }

    if (length == localstate.piece_length);
    else if (length == localstate.last_piece_size);
    else {
        fprintf(stderr, "Have not implemented sending blocks not equal to the piece size yet\n");
        return -1;
    }

    if (send_piece_message(sockfd, index) <= 0){
        fprintf(stderr, "There was an error fulfilling a piece request\n");
        return -1;
    }


    return 0;
}
void send_have_messages(int index){
    msg_t msg;
    int i;
    char payload[4];

    encode_int_as_char(index, payload, N_INTEGER_BYTES);

    msg.length = 5;
    msg.type = HAVE;
    msg.payload = payload;

    for (i = 0; i <= localstate.max_sockfd; i++){
        if (localstate.peers[i] == NULL)
            continue;
        send_peer_message(i, &msg);
    }
}

int handle_piece_message(int sockfd, msg_t *msg){
    FILE *f;
    int index, begin, expected_length, piece_length;
    char *piece_contents, *cur_hash_digest;
    char piece_filename[FILENAME_WITH_EXT_BUFLEN];

    DEBUG_PRINT("received a PIECE message on sockfd %d for index: %d\n", 
            sockfd,
            decode_int_from_char(msg->payload, 4));

    index = decode_int_from_char(msg->payload, 4);
    begin = decode_int_from_char(msg->payload + 4, 4);
    piece_length = decode_int_from_char(msg->payload + 8, 4);

    if (index != localstate.peers[sockfd]->requested_piece){
        fprintf(stderr, "Index of send piece wrong.  Requested %d.  Got %d\n",
                localstate.peers[sockfd]->requested_piece, index);
        return -1;
    }
    
    if (begin != 0){
        fprintf(stderr, "Have not implemented sending blocks that aren't the first in their piece\n");
        return -1;
    }

    if (index == localstate.n_pieces-1)
        piece_length = localstate.last_piece_size;
    else 
        piece_length = localstate.piece_length;


    expected_length = piece_length + 9;
    if (msg->length != expected_length){
        fprintf(stderr, "Piece message was not the correct length.  Should be %d, was %d\n", expected_length, msg->length);
        return -1;
    }

    cur_hash_digest = localstate.piece_hash_digests[index];
    piece_contents = msg->payload + 8;

    if (validate_piece(cur_hash_digest, piece_contents, piece_length) == -1){
        fprintf(stderr, "Piece received did not validate against it's hash.\n");
        return -1;
    }

    get_filename_with_extension(cur_hash_digest, piece_filename);
    if ((f = fopen(piece_filename, "w")) == NULL){
        perror("fopen");
        return -1;
    }

    if (fwrite(piece_contents, 1, piece_length, f) <= 0){
        perror("fwrite");
        return -1;
    }

    fclose(f);

    // Note, bitfield is automatically updated, because piece hash file exists.
    
    // flush request messages before broadcasting have in order to prevent request message getting lost in rapid succession of
    // have / request message on same buffer.
    localstate.peers[sockfd]->requested_piece = -1;
    send_request_messages();

    send_have_messages(index);

    return 0;
}

int handle_have_message(int sockfd, msg_t *msg){
    int index, idx_bitfield;

    if (msg->length != 5){
        fprintf(stderr, "Have message was not correct length.\n");
        return -1;
    }

    index = decode_int_from_char(msg->payload, N_INTEGER_BYTES);

    printf("received HAVE message on sockfd: %d for piece: %d\n", sockfd, index);
    idx_bitfield = (int)pow((double) 2, localstate.n_pieces - 1 - index);
    localstate.peers[sockfd]->bitfield |= idx_bitfield;

    return 0;
}

int dispatch_peer_message(int sockfd, char *receive_buffer) {
    int msg_type, i;
    msg_t msg;
    char length_buffer[N_INTEGER_BYTES];

    for (i = 0; i < N_INTEGER_BYTES; i++)
        length_buffer[i] = receive_buffer[i];

    msg.length = decode_int_from_char(length_buffer, N_INTEGER_BYTES);
    msg_type = receive_buffer[N_INTEGER_BYTES];
    msg.type = msg_type;

    if (msg.type != BITFIELD && localstate.peers[sockfd]->cleared_bitfield == 0)
        return -1;

    msg.payload = receive_buffer + N_INTEGER_BYTES + 1;

    switch(msg.type){
        case (CHOKE):
            fprintf(stderr, "CHOKE message, length=%d\n", msg.length);
            break;
        case (UNCHOKE):
            fprintf(stderr, "UNCHOKE message\n");
            break;
        case (INTERESTED):
            fprintf(stderr, "INTERESTED message\n");
            break;
        case (NOT_INTERESTED):
            fprintf(stderr, "NOT_INTERESTED message\n");
            break;
        case (HAVE):
            if (handle_have_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (BITFIELD):
            if (handle_bitfield_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (REQUEST):
            if (handle_request_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (PIECE):
            if (handle_piece_message(sockfd, &msg) == -1)
                return -1;
            break;
        case (CANCEL):
            fprintf(stderr, "CANCEL message\n");
            break;
        default:
            fprintf(stderr, "Unknown message type: %d\n", msg.type);
            return -1;
    }

    return msg.length;
}

int receive_peer_message(int sockfd){
    int nbytes, bytes_read, cum_bytes_read;

    // longest possible length is in a piece message.
    // (length prefix) (msg type) (index) (begin) (piece length) + null-termination (received) + null-termination (in receive function);
    int _longest_possible_length = 4 + 1 + 4 + 4 + localstate.piece_length + 2; 
    int longest_possible_length = 5 * _longest_possible_length;
    char receive_buffer[longest_possible_length];

    memset(receive_buffer, 0, longest_possible_length);
    if ((nbytes = receive_on_socket(sockfd, receive_buffer, longest_possible_length)) < 0){
        return -1;
    }
    memset(receive_buffer + nbytes, 0, longest_possible_length-nbytes);

    DEBUG_PRINT("received nbytes=%d on socket %d\n", nbytes, sockfd);

    cum_bytes_read = 0;
    bytes_read = 0;
    while (nbytes > 0){
        DEBUG_PRINT("nbytes left on socket %d...: %d\n", sockfd, nbytes);
        bytes_read = dispatch_peer_message(sockfd, receive_buffer + cum_bytes_read); 
        fprintf(stderr, "bytes read: %d\n", bytes_read);
        if (bytes_read == -1){
            fprintf(stderr, "Something strange happened while parsing messages on sockfd %d\n", sockfd);
            break;
        }

        bytes_read += N_INTEGER_BYTES + 1;  // +1 for null-termination

        cum_bytes_read += bytes_read;
        nbytes -= bytes_read;
    }

    return 0;
}
