/*
 * socket.c
 *
 *  Created on: Oct 8, 2015
 *      Author: tom
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mysocket.h"

/*static void error(const char *msg)
{
    (void)msg;
}

static void Received(char *data, int n)
{
    if( 0 != fpMsgReceived)
    {
        (fpMsgReceived)(data, n);
    }
}*/

int Sock_InitServer(int port, commEngine_fpHandles_t *pfpHandles)
{
    (void)port;
    (void)pfpHandles;
    return -1;
}

int Sock_InitClient(char *host, int port, commEngine_fpHandles_t *pfpHandles)
{
    (void)host;
    (void)port;
    (void)pfpHandles;
    return -1;
}

int Sock_Send(uint8_t *data, uint8_t length, int16_t handle)
{
    (void)data;
    (void)length;
    (void)handle;
    return 0;
}

int Receive()
{
    return 0;
}

int Sock_Task()
{
    return 0;
}

int Sock_Close(int16_t sockfd)
{
    return 0;
}
