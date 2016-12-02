/*
 * socket.c
 *
 *  Created on: Oct 8, 2015
 *      Author: tom
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define WINDOWS

#ifdef WINDOWS
  #include <winsock2.h>
//  #pragma comment(lib,"ws2_32.lib") //Winsock Library
  #define socklen_t int
#else
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
#endif
#include "mysocket.h"

static void error(const char *msg)
{
    perror(msg);
    //perror(errno);
//    exit(0);
}

commEngine_fpHandles_t *fpMySockHandles;
driver_fpHandles_t Sock_Handles =
{
    &Sock_Send,
};

int16_t sockfd = -1;
struct sockaddr_in serv_addr;

//for server
#define MAX_NOF_CLIENTS     (127)
int16_t clientSockets[MAX_NOF_CLIENTS] = {0};


static void Received(uint8_t *data, int8_t n, int16_t handle)
{
    if( 0 != fpMySockHandles)
    {
        (fpMySockHandles->fpReceive)(data, n, handle);
    }
}

int Sock_InitServer(int port, commEngine_fpHandles_t *pfpHandles)
{
    fpMySockHandles = pfpHandles;

    if (port < 0)
    {
        error("ERROR, no port provided\n");
        return -1;
    }


#ifdef WINDOWS
    WSADATA wsaData;

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif

    //create master socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }
    else
    {
        //set address
        memset((char *) &serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        //Bind socket to localhost with a specified port
        if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        {
            error("ERROR on binding");
        }
        else
        {
            if(listen(sockfd, MAX_NOF_CLIENTS) < 0)
            {
                error("ERROR on listen");
            }
            else
            {
                printf("\nReady for connections");
                return sockfd;
            }
        }
    }
    Sock_Close(sockfd);
    return -1;
}

int Sock_ServerTask()
{
    static uint8_t buffer[100] = {0};

    socklen_t addrLength = sizeof(serv_addr);
    static int highestfd;

    //set of socket descriptors
    fd_set readfds;
    FD_ZERO(&readfds);

    //add master socket to set
    FD_SET(sockfd, &readfds);
    highestfd = sockfd;

    //add child sockets to set
    int16_t sd, i;
    for ( i = 0 ; i < MAX_NOF_CLIENTS ; i++)
    {
        //socket descriptor
        sd = clientSockets[i];

        //if valid socket descriptor then add to read list
        if(sd > 0)
        {
            FD_SET( sd , &readfds);
        }
        //highest file descriptor number, need it for the select function
        if(sd > highestfd)
        {
            highestfd = sd;
        }
    }

    //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
    struct timeval tv;
    tv.tv_sec = 1;      /* 1 Secs Timeout */
    tv.tv_usec = 1;
    int activity = select( highestfd + 1 , &readfds , NULL , NULL , &tv);

    if ((activity < 0) && (errno!=EINTR))
    {
        printf("select error");
    }

    //If something happened on the master socket , then its an incoming connection
    if (FD_ISSET(sockfd, &readfds))
    {
        int newSockfd;
        if ((newSockfd = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t*)&addrLength))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

/*
        struct timeval tv;
        tv.tv_sec = 0;      // 30 Secs Timeout
        tv.tv_usec = 10;    // Not init'ing this can cause strange errors
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
*/
        //inform user of socket number - used in send and receive commands
        printf("\nNew connection , socket fd is %d , ip is : %s , port : %d" , newSockfd , inet_ntoa(serv_addr.sin_addr) , ntohs(serv_addr.sin_port));

        //add new socket to array of sockets
        for (i = 0; i < MAX_NOF_CLIENTS; i++)
        {
            //if position is empty
            if( clientSockets[i] == 0 )
            {
                clientSockets[i] = newSockfd;
                //printf("\nAdding to list of sockets as %d" , i);
                if( 0 != fpMySockHandles)
                {
                    (fpMySockHandles->fpNodeConnect)(newSockfd);
                }
                break;
            }
        }
    }

    //else its some IO operation on some other socket :)
    for (i = 0; i < MAX_NOF_CLIENTS; i++)
    {
        sd = clientSockets[i];
        if (FD_ISSET( sd , &readfds))
        {
            //Check if it was for closing , and also read the incoming message
            int valread;

            valread = recv(sd, (char*)buffer, 20, 0/*MSG_WAITALL*/);
//            valread = read( sd , buffer, 100);
            if (valread <= 0)
            {
                if(valread<0)
                {
                    printf("Socket Error!");
                }
                //Somebody disconnected , get his details and print
                getpeername(sd , (struct sockaddr*)&serv_addr , (socklen_t*)&addrLength);
                printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(serv_addr.sin_addr) , ntohs(serv_addr.sin_port));

                //Close the socket and mark as 0 in list for reuse
                close( sd );
                clientSockets[i] = 0;
                if( 0 != fpMySockHandles)
                {
                    (fpMySockHandles->fpNodeDisconnect)(sd);
                }
            }
            else
            {
                //set the string terminating NULL byte on the end of the data read
                buffer[valread] = '\0';
                printf("\n--%s--",buffer);
                Received((uint8_t *)&buffer[0], valread, sd);
            }
        }
    }
    return 0;
}


int Sock_InitClient(char *host, int port, commEngine_fpHandles_t *pfpHandles)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    fpMySockHandles = pfpHandles;

#ifdef WINDOWS
    WSADATA wsaData;

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
#endif

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("ERROR opening socket");
    }
    else
    {
        if (host==NULL)
        {
            error("ERROR invalid hostname");
        }
        else
        {
            server = gethostbyname(host);
            if (server == NULL)
            {
                error("ERROR, no such host\n");
            }
            else
            {
                memset((char *) &serv_addr, 0, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                memcpy((char *)&serv_addr.sin_addr.s_addr,
                       (char *)server->h_addr,
                       server->h_length);
                serv_addr.sin_port = htons(port);
                if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
                {
                    error("ERROR connecting");
                }
                (fpMySockHandles->fpNodeConnect)(sockfd);

                struct timeval tv;
                tv.tv_sec = 30;  /* 30 Secs Timeout */
                tv.tv_usec = 10;  // Not init'ing this can cause strange errors
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));

                printf("\nConnected to host: [ip]:%d, sockfd: %d", port, sockfd);
                return sockfd;
            }
        }
    }
    Sock_Close(sockfd);
    return -1;
}

int Sock_Send(uint8_t *data, uint8_t length, int16_t handle)
{
    if(handle<0)
    {
        printf("\nERROR Sending to invalid socket: %d", handle);
    }

    //int n = write((int)handle, (char*)data, (size_t)length);
    int n = send((int)handle, (char*)data, (size_t)length, 0);
    if (0 > n )
    {
        //printf("\nERROR (%d) writing to socket: %d\n", errno, handle);
        error("Sock_Send: write failed");
    }
    return n;
}

/*int Sock_SendClient(uint8_t *data, uint8_t length, int16_t handle)
{
    if(sockfd<0)
    {
        error("ERROR invalid socket");
    }

    int n;
    n = write(sockfd, data, length);
    if (n < 0)
    {
         error("ERROR writing to socket");
    }
    return n;
}*/

int Receive()
{
    static uint8_t recvBuffer[100] = {0};
    int n;

    if(sockfd<0)
    {
        error("ERROR invalid socket");
    }

    memset(&recvBuffer, 0, sizeof(recvBuffer));
    n = recv(sockfd, (char*)recvBuffer, sizeof(recvBuffer), 0);
    //n = read(sockfd,&recvBuffer,sizeof(recvBuffer));
    if(n>0)
    {
#ifdef X86
        int i;
        printf("\r\n");
        for(i=0; i<n; i++)
            printf("|%02X",recvBuffer[i]);
#endif

        Received(&recvBuffer[0], n, sockfd);
    }
    return n;
}

int Sock_Task()
{
    return Receive();
}

int Sock_Close(int16_t sockfd)
{
    if(sockfd>=0)
    {
        close(sockfd);
    }
    return 0;
}
