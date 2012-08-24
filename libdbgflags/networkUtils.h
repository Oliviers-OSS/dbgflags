#ifndef _NETWORK_UTILS_H_
#define _NETWORK_UTILS_H_

#include <sys/types.h>
#include <sys/socket.h>

static __inline int sendBuffer(int connected_socket,const void *buffer,const size_t bufferSize, size_t *bytesSend) {
    int error = EXIT_SUCCESS;
    const unsigned char *cursor = (unsigned char *)buffer;
    size_t toSend = bufferSize;

    while(toSend != 0) {
        const ssize_t sizeSend = send(connected_socket,cursor,toSend,0);
        if (sizeSend > 0) {
            DEBUG_VAR(sizeSend,"%d");
            cursor += sizeSend;
            toSend -= sizeSend;
        } else if (0 == sizeSend) {
            error = EPIPE;
            toSend = 0;
            DEBUG_MSG("sizeSend is NULL !");
        } else { /* error */
            error = errno;
            toSend = 0;
        }
    } /* while(toSend != 0) */

    if (bytesSend != NULL) {
        *bytesSend = cursor - (unsigned char *)buffer;
    }

    return error;
}

static __inline int receiveBuffer(int connected_socket,void *buffer,const size_t bufferSize,size_t *bytesReceived) {
    int error = EXIT_SUCCESS;
    unsigned char *cursor = (unsigned char *)buffer;
    size_t toReceive = bufferSize;

    while(toReceive != 0) {
        const ssize_t sizeReceived = recv(connected_socket,cursor,toReceive,0);
        if (sizeReceived > 0) {
            DEBUG_VAR(sizeReceived,"%d");
            DEBUG_DUMP_MEMORY(buffer,sizeReceived);
            cursor += sizeReceived;
            toReceive -= sizeReceived;
        } else if (0 == sizeReceived){
            error = EPIPE;
            toReceive = 0;
            DEBUG_MSG("sizeReceived is NULL !");
        } else { /* error */
            error = errno;
            toReceive = 0;
        }
    } /* while(toReceive != 0) */

    if (bytesReceived != NULL) {
        *bytesReceived = cursor - (unsigned char *)buffer;
    }

    return error;
}

#endif /*_NETWORK_UTILS_H_*/
