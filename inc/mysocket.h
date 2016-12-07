/*
 * socket.h
 *
 *  Created on: Oct 8, 2015
 *      Author: tom
 */

#ifndef SOCKET_H_
#define SOCKET_H_


#include <stdint.h>

typedef enum
{
    eSOCK_INVALID_FD    = -100,
    eSOCK_INVALID_PORT,
    eSOCK_INVALID_HOSTNAME,
    eSOCK_HOSTNAME_DOES_NOT_EXIST,

    eSOCK_FAILED_TO_INIT,
    eSOCK_FAILED_TO_ACCEPT,
    eSOCK_FAILED_TO_BIND,
    eSOCK_FAILED_TO_CONNECT,
    eSOCK_FAILED_TO_LISTEN,
    eSOCK_FAILED_TO_READ,
    eSOCK_FAILED_TO_SEND,
    eSOCK_FAILED_TO_SELECT,



}eSOCK_ERRORS;

typedef void (*fpSockConnect)   (int16_t handle);
typedef void (*fpSockDisconnect)(int16_t handle);
typedef void (*fpReceiveData)   (int16_t handle, uint8_t *data, int8_t length);

typedef struct
{
    fpSockConnect       fpConnect;
    fpSockDisconnect    fpDisconnect;
    fpReceiveData       fpReceive;
} Sock_fpHandles_t;

typedef int sockfd_t;

int8_t  Sock_Init           (Sock_fpHandles_t *pSockHandles);
sockfd_t Sock_SetupClient   (const char *host, int port);
sockfd_t Sock_SetupServer   (int port);
int     Sock_ServerTask     (sockfd_t sockfd);
int     Sock_Send           (sockfd_t sockfd, uint8_t *data, size_t length);
int     Sock_DataAvailable  (sockfd_t sockfd);
int     Sock_ReadByte       (sockfd_t sockfd, char *byte);
int     Sock_Close          (sockfd_t sockfd);
int     Sock_Task           (sockfd_t sockfd);

#endif /* SOCKET_H_ */
