#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rtp.h"

#define DEBUG 0

#if DEBUG
 #define DEBUG_PRINTF(o) printf(o)
 #define DEBUG_PRINTFVAR(o, v) printf(o, v)
#else
 #define DEBUG_PRINTF(o) printf("")
 #define DEBUG_PRINTFVAR(o, v) printf("")
#endif

/* GIVEN Function:
 * Handles creating the client's socket and determining the correct
 * information to communicate to the remote server
 */
CONN_INFO* setup_socket(char* ip, char* port){
    struct addrinfo *connections, *conn = NULL;
    struct addrinfo info;
    memset(&info, 0, sizeof(struct addrinfo));
    int sock = 0;
    
    info.ai_family = AF_INET;
    info.ai_socktype = SOCK_DGRAM;
    info.ai_protocol = IPPROTO_UDP;
    getaddrinfo(ip, port, &info, &connections);
    
    /*for loop to determine corr addr info*/
    for(conn = connections; conn != NULL; conn = conn->ai_next){
        sock = socket(conn->ai_family, conn->ai_socktype, conn->ai_protocol);
        if(sock <0){
            if(DEBUG)
                perror("Failed to create socket\n");
            continue;
        }
        if(DEBUG)
            printf("Created a socket to use.\n");
        break;
    }
    if(conn == NULL){
        perror("Failed to find and bind a socket\n");
        return NULL;
    }
    CONN_INFO* conn_info = malloc(sizeof(CONN_INFO));
    conn_info->socket = sock;
    conn_info->remote_addr = conn->ai_addr;
    conn_info->addrlen = conn->ai_addrlen;
    return conn_info;
}

void shutdown_socket(CONN_INFO *connection){
    if(connection)
        close(connection->socket);
}

/*
 * ===========================================================================
 *
 *			STUDENT CODE STARTS HERE. PLEASE COMPLETE ALL FIXMES
 *
 * ===========================================================================
 */


/*
 *  Returns a number computed based on the data in the buffer.
 */
static int checksum(char *buffer, int length){
    int i;
    int sum = 0;
    for (i = 0; i < length; i++) {
        sum += buffer[i];
    }
    
    return sum;
}

/*
 *  Converts the given buffer into an array of PACKETs and returns
 *  the array.  The value of (*count) should be updated so that it
 *  contains the length of the array created.
 */
static PACKET* packetize(char *buffer, int length, int *count){
    DEBUG_PRINTF("Calling packetize()");
    int num = length / MAX_PAYLOAD_LENGTH;
    int endCheck = length % MAX_PAYLOAD_LENGTH;
    if (endCheck) {
        num++;
    }
    *count = num;
    
    PACKET* returnArray = malloc(num * sizeof(PACKET));
    
    DEBUG_PRINTF("Setting base data in packetize()\n");
    int i;
    for (i = 0; i < num; i++) {
        returnArray[i].type = DATA;
        returnArray[i].payload_length = MAX_PAYLOAD_LENGTH;
        returnArray[i].checksum = checksum(&buffer[MAX_PAYLOAD_LENGTH * i],
                                           returnArray[i].payload_length);
    }
    
    DEBUG_PRINTF("Adjusting last data in packetize()\n");
    returnArray[num - 1].type = LAST_DATA;
    if (endCheck) {
        returnArray[num - 1].payload_length = endCheck;
    }
    
    DEBUG_PRINTF("Calculating payloads in packetize()\n");
    int j;
    for (i = 0; i < num; i++) {
        for (j = 0; j < returnArray[i].payload_length; j++)
        {
            returnArray[i].payload[j] = buffer[MAX_PAYLOAD_LENGTH * i + j];
        }
        
        returnArray[i].checksum = checksum(returnArray[i].payload,
                                           returnArray[i].payload_length);
    }
    
    return returnArray;
}

/*
 * Send a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
int rtp_send_message(CONN_INFO *connection, MESSAGE*msg){
	DEBUG_PRINTF("Sending message from rtp_send_message()\n");
    /* ---- FIXME ----
     * The goal of this function is to turn the message buffer
     * into packets and then, using stop-n-wait RTP protocol,
     * send the packets and re-send if the response is a NACK.
     * If the response is an ACK, then you may send the next one
     */
    
    int *num = malloc(sizeof(int));
    PACKET *response = malloc(sizeof(PACKET));
    PACKET *packets = packetize(msg->buffer, msg->length, num);
    
    DEBUG_PRINTF("time to transfer data in rtp_send_message()\n");
    DEBUG_PRINTFVAR("We have %d packets\n", *num);

    int i = 0;
    while (i < *num) {
    	DEBUG_PRINTFVAR("Sending packet %d \n", i);
        sendto(connection->socket, (void *) &packets[i], sizeof(PACKET), 0,
               connection->remote_addr, connection->addrlen);
        DEBUG_PRINTF("Send to finished()\n");
        recvfrom(connection->socket, (void *) response, sizeof(PACKET), 0,
                 connection->remote_addr, &connection->addrlen);
        DEBUG_PRINTF("Response recieved()\n");
        if (response->type == ACK)
        {
            i++;
        }
    }

    DEBUG_PRINTF("Sending is complete! \n");
    
    free(packets);
    free(response);
    return 1;
}

/*
 * Receive a message via RTP using the connection information
 * given on UDP socket functions sendto() and recvfrom()
 */
MESSAGE* rtp_receive_message(CONN_INFO *connection){
	DEBUG_PRINTF("rtp_receive_message() called! \n");
    /* ---- FIXME ----
     * The goal of this function is to handle
     * receiving a message from the remote server using
     * recvfrom and the connection info given. You must
     * dynamically resize a buffer as you receive a packet
     * and only add it to the message if the data is considered
     * valid. The function should return the full message, so it
     * must continue receiving packets and sending response
     * ACK/NACK packets until a LAST_DATA packet is successfully
     * received.
     */
    
    MESSAGE *message = malloc(sizeof(MESSAGE));
    int allDataSent = 0;
    
    while (!allDataSent) {
        PACKET *packet = malloc(sizeof(PACKET));
        PACKET *response = malloc(sizeof(PACKET));
        
        recvfrom(connection->socket, (void *) packet, sizeof(PACKET), 0,
                 connection->remote_addr, &connection->addrlen);
        
        if (checksum(packet->payload, packet->payload_length) == packet->checksum) {
            response->type = ACK;
            response->payload_length = 0;
            response->checksum = 0;
            sendto(connection->socket, (void *) response, sizeof(PACKET), 0,
                   connection->remote_addr, connection->addrlen);
            
            int totalLength = packet->payload_length + message->length;
            DEBUG_PRINTFVAR("rtp_receive_message() buffer length is: %d! \n", (packet->payload_length + message->length));
            char* buff = malloc((totalLength) * sizeof(char));
            
            int i;
            for (i = 0; i < message->length; i++) {
            	DEBUG_PRINTFVAR("Message index: %d", i);
            	DEBUG_PRINTFVAR("/%d\n", totalLength);
                buff[i] = message->buffer[i];
            }
            
            for (i = 0; i < packet->payload_length; i++) {
                buff[i + message->length] = packet->payload[i];
            }
             
            message->buffer = buff;
            message->length += packet->payload_length;
            
            if (packet->type == LAST_DATA) {
                allDataSent = 1;
            }
        } else {
            response->type = NACK;
            response->payload_length = 0;
            response->checksum = 0;
            sendto(connection->socket, (void *) response, sizeof(PACKET), 0,
                   connection->remote_addr, connection->addrlen);
        }

    }

    DEBUG_PRINTF("rtp_receive_message() complete! \n");
    
    return message;
}
