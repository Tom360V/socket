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

//#define WINDOWS

#ifdef WINDOWS
  #include <winsock2.h>
  #include <Windows.h>
  #include <ws2tcpip.h>
//  #pragma comment(lib,"ws2_32.lib") //Winsock Library
  #define socklen_t int
#else
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <sys/ioctl.h>
  #define ioctlsocket ioctl
#endif
#include "mysocket.h"

Sock_fpHandles_t *pMySockHandles;

struct sockaddr_in serv_addr;

//for server
#define MAX_NOF_CLIENTS     (127)
int16_t clientSockets[MAX_NOF_CLIENTS] = {0};

#ifdef WINDOWS
#define PRINT_WSA_ERROR     printf(", WSALastError::%d", WSAGetLastError());
#else
#define PRINT_WSA_ERROR
#endif

#if 1
#define LOG_ERROR(...)  perror(__FUNCTION__);                   \
                        printf("ERROR::%s:%s:", __FUNCTION__, strerror(errno) );    \
                        printf(__VA_ARGS__);                    \
                        PRINT_WSA_ERROR                         \
                        printf("\r\n");                         \
                        fflush(stdout);
#else
    LOG_ERROR()
#endif

#if 0
#define LOG_INFO(...)   printf("INFO ::%s: ",__FUNCTION__); \
                        printf(__VA_ARGS__);                \
                        printf("\r\n");                     \
                        fflush(stdout);
#else
#define LOG_INFO(...)
#endif


/******************************************************************************
 *
 */
int8_t Sock_Init(Sock_fpHandles_t *pSockHandles)
{
    pMySockHandles = pSockHandles;

#ifdef WINDOWS
    WSADATA wsaData;

    // Initialize Winsock
    int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0)
    {
        LOG_ERROR("WSAStartup failed: %d", iResult);
        return eSOCK_FAILED_TO_INIT;
    }
#endif
    return 0;
}

/******************************************************************************
 *
 */
static void sock_Received(sockfd_t sockfd, uint8_t *data, int8_t n)
{
    if( 0 != pMySockHandles)
    {
        (pMySockHandles->fpReceive)((int16_t)sockfd, data, n);
    }
}

/******************************************************************************
 *
 */
sockfd_t Sock_SetupServer(int port)
{
    int16_t sockfd;

    if (port < 0)
    {
        LOG_ERROR("invalid port");
        return(eSOCK_INVALID_PORT);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_ERROR("invalid sockfd");
        return eSOCK_INVALID_FD;
    }

    //set address
    memset((char *) &serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    //Bind socket to localhost with a specified port
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        LOG_ERROR("failed to bind");
        Sock_Close(sockfd);
        return eSOCK_FAILED_TO_BIND;
    }

    if(listen(sockfd, MAX_NOF_CLIENTS) < 0)
    {
        LOG_ERROR("failed to listen");
        Sock_Close(sockfd);
        return eSOCK_FAILED_TO_LISTEN;
    }
    return sockfd;
}

int Sock_ServerTask(sockfd_t sockfd)
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
        LOG_ERROR("failed to select");
        return eSOCK_FAILED_TO_SELECT;
    }

    //If something happened on the master socket , then its an incoming connection
    if (FD_ISSET(sockfd, &readfds))
    {
        int newSockfd;
        if ((newSockfd = accept(sockfd, (struct sockaddr *)&serv_addr, (socklen_t*)&addrLength))<0)
        {
            LOG_ERROR("failed to accept");
            return eSOCK_FAILED_TO_ACCEPT;
        }

        /*
        struct timeval tv;
        tv.tv_sec = 0;      // 30 Secs Timeout
        tv.tv_usec = 10;    // Not init'ing this can cause strange errors
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
         */

        //inform user of socket number - used in send and receive commands
        LOG_INFO("New connection , socket fd is %d , ip is : %s , port : %d" , newSockfd , inet_ntoa(serv_addr.sin_addr) , ntohs(serv_addr.sin_port));

        //add new socket to array of sockets
        for (i = 0; i < MAX_NOF_CLIENTS; i++)
        {
            //if position is empty
            if( clientSockets[i] == 0 )
            {
                clientSockets[i] = newSockfd;
                LOG_INFO("Adding to list of sockets as %d" , i);
                if( 0 != pMySockHandles)
                {
                    (pMySockHandles->fpConnect)((int16_t)newSockfd);
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
            if (valread <= 0)
            {
                if(valread<0)
                {
                    LOG_ERROR("failed to receive");
                    return eSOCK_FAILED_TO_READ;
                }
                //Somebody disconnected , get his details and print
                getpeername(sd , (struct sockaddr*)&serv_addr , (socklen_t*)&addrLength);
                LOG_INFO("Host disconnected , ip %s , port %d \n" , inet_ntoa(serv_addr.sin_addr) , ntohs(serv_addr.sin_port));

                //Close the socket and mark as 0 in list for reuse
                Sock_Close( sd );
                clientSockets[i] = 0;
                if( 0 != pMySockHandles)
                {
                    (pMySockHandles->fpDisconnect)((int16_t)sd);
                }
            }
            else
            {
                //set the string terminating NULL byte on the end of the data read
                buffer[valread] = '\0';
                LOG_INFO("--%s--",buffer);
                sock_Received(sd, (uint8_t *)&buffer[0], valread);
            }
        }
    }
    return 0;
}

int8_t Sock_validateSocket(sockfd_t sockfd)
{
    int error = 0;
    socklen_t len = sizeof (error);
    if(0 > getsockopt (sockfd, SOL_SOCKET, SO_ERROR, (char*)&error, &len) )
    {
        LOG_ERROR("invalid sockfd:%d (getsockopt)", sockfd);
        return eSOCK_INVALID_FD;
    }
    return 0;
}

sockfd_t Sock_SetupClient(const char *host, int port)
{
    struct sockaddr_in serv_addr;
    struct hostent *server;
    sockfd_t sockfd = -1;

    if (host==NULL)
    {
        LOG_ERROR("invalid host");
        return eSOCK_INVALID_HOSTNAME;
    }

    server = gethostbyname(host);
    if (server == NULL)
    {
        LOG_ERROR("hostname does not exist");
        return eSOCK_HOSTNAME_DOES_NOT_EXIST;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        LOG_ERROR("invalid sockfd");
        return eSOCK_INVALID_FD;
    }

    memset((char *)&serv_addr, 0, sizeof(serv_addr));
    memcpy((char *)&serv_addr.sin_addr.s_addr, (char *)server->h_addr, server->h_length);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    if (0 != connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)))
    {
        LOG_ERROR("failed to connect");
        Sock_Close(sockfd);
        return eSOCK_FAILED_TO_CONNECT;
    }

    //Trigger fpConnect!
    if(0 != pMySockHandles)
    {
        (pMySockHandles->fpConnect)((int16_t)sockfd);
    }

    struct timeval tv;
    tv.tv_sec = 5;  /* 30 Secs Timeout */
    tv.tv_usec = 1;  // Not init'ing this can cause strange errors
    if(0 > setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)))
    {
        LOG_ERROR("failed to set sockoptions, sockfd:%d", sockfd);
        Sock_Close(sockfd);
        return eSOCK_INVALID_FD;
    }

    if(0 > setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO , (char *)&tv,sizeof(struct timeval)))
    {
        LOG_ERROR("failed to set sockoptions, sockfd:%d", sockfd);
        Sock_Close(sockfd);
        return eSOCK_INVALID_FD;
    }

    LOG_INFO("Connected to host: %s:%d, sockfd: %d\r\n",
                        inet_ntoa(serv_addr.sin_addr), port, sockfd);
    return sockfd;
}

int Sock_Send(sockfd_t sockfd, uint8_t *data, size_t length)
{
    if(0 > sockfd)
    {
        LOG_ERROR("invalid sockfd");
        return eSOCK_INVALID_FD;
    }

    int n = send ((int)sockfd, (char*)data, (size_t)length, 0);
    if (0 > n )
    {
#ifdef WINDOWS
        LOG_ERROR("failed to send, send:%d (lastError:%d)", n, WSAGetLastError());
#endif
        return eSOCK_FAILED_TO_SEND;
    }
    LOG_INFO("Send Completed [%d]!", n);
    return n;
}

int Sock_DataAvailable(sockfd_t sockfd)
{
#if 0
    int retval;
    fd_set rfds;
    struct timeval tv;
    tv.tv_sec  = 1;
    tv.tv_usec = 1;

    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    retval = select(sockfd+1, &rfds, NULL, NULL, &tv);

    LOG_INFO("%i, %d", (sockfd_t)sockfd, retval);
    if (retval == -1)
    {
        return 0;
    }
    return 1;

#else
    u_long count = 0;
    ioctlsocket((int)sockfd, FIONREAD, (u_long*)&count);
    LOG_INFO("socket: %d, count:%lu\r\n", sockfd, count);
    return (count!=0);
#endif
}

int Sock_ReadByte(sockfd_t sockfd, char *byte)
{
    int length = recv(sockfd, byte, 1, 0/*flags*/);
    LOG_INFO("%d", length);
    return length;
}

int Receive(sockfd_t sockfd)
{
    static uint8_t recvBuffer[100] = {0};
    int n;

    if(sockfd<0)
    {
        LOG_ERROR("Invalid sockfd");
        return eSOCK_INVALID_FD;
    }

    memset(&recvBuffer, 0, sizeof(recvBuffer));
    n = recv(sockfd, (char*)recvBuffer, sizeof(recvBuffer), 0);
    if(n>0)
    {
#ifdef X86
        int i;
        printf("\r\n");
        for(i=0; i<n; i++)
            printf("|%02X",recvBuffer[i]);
#endif

        sock_Received(sockfd, &recvBuffer[0], n);
    }
    return n;
}

int Sock_Task(sockfd_t sockfd)
{
    return Receive(sockfd);
}

int Sock_Close(sockfd_t sockfd)
{
    LOG_INFO("socket:%d", sockfd);
    if(0 <= sockfd)
    {
        if(0 > close(sockfd))
        {
            LOG_ERROR("failed to close socked");
        }
    }
    return 0;
}
