/*
 * socket.h
 *
 *  Created on: Oct 8, 2015
 *      Author: tom
 */

#ifndef SOCKET_H_
#define SOCKET_H_

#include "communication_interface.h"

extern driver_fpHandles_t Sock_Handles;

int Sock_InitClient(char *host, int port, commEngine_fpHandles_t *pfpHandles);
int Sock_InitServer(int port, commEngine_fpHandles_t *pfpHandles);
int Sock_ServerTask();
int Sock_Send(uint8_t *data, uint8_t length, int16_t handle);
//int Sock_SendClient(uint8_t *data, uint8_t length, int16_t handle);
int Sock_Close(int16_t sockfd);
int Sock_Task();

#endif /* SOCKET_H_ */
