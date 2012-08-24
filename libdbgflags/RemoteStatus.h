#ifndef _REMOTE_STATUS_H_
#define _REMOTE_STATUS_H_

#include "protocol.h"

#define REMOTE_STATUS_KEY 0xFF
static __inline int isRemoteStatus(const unsigned char *buffer,const size_t bytesReceived ) {
    return ((bytesReceived == sizeof(RemoteStatus)) && (REMOTE_STATUS_KEY == buffer[0]));
}

static __inline int sendErrorCode(int connected_socket,const int errorCode) {
    const RemoteStatus remoteStatus = {REMOTE_STATUS_KEY,errorCode};
    int error = sendBuffer(connected_socket,&remoteStatus,sizeof(RemoteStatus),NULL);
    return error;
}

static __inline int getRemoteStatus(const unsigned char *buffer ) {
    const RemoteStatus *remoteStatus = (const RemoteStatus *)buffer;
    return (remoteStatus->status);
}

#endif /* _REMOTE_STATUS_H_ */

