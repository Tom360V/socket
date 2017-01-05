/*

*/

#include <errno.h>
#include <stdio.h>
#include <stdint.h>
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
//  #define errno WSAGetLastError()
  #define ioctl ioctlsocket
#else
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <netdb.h>
  #include <sys/ioctl.h>
#endif

#include "clientSocket.h"
#include "mysocket.h"

/******************************************************************************
 * Private Function Prototypes
 *****************************************************************************/
int     clientSocket_connectIP  (IPAddress_t ip, uint16_t port);
int     clientSocket_connectHost(const char *host, uint16_t port);
uint8_t clientSocket_connected  ();
size_t  clientSocket_write      (uint8_t);
size_t  clientSocket_writeMulti (const uint8_t *buf, size_t size);
int     clientSocket_available  ();
int     clientSocket_read       ();
int     clientSocket_readMulti  (uint8_t *buf, size_t size);
int     clientSocket_peek       ();
void    clientSocket_flush      ();
void    clientSocket_stop       ();

/******************************************************************************
 * Private Variable
 *****************************************************************************/
const Client_t clientSock =
{
    &clientSocket_connectIP,
    &clientSocket_connectHost,
    &clientSocket_connected,
    &clientSocket_write,
    &clientSocket_writeMulti,
    &clientSocket_available,
    &clientSocket_read,
    &clientSocket_readMulti,
    &clientSocket_peek,
    &clientSocket_flush,
    &clientSocket_stop
};

static int clientSockfd;


/******************************************************************************
 * Private Functions
 *****************************************************************************/
#define LOG_ERROR(...)     \
                        perror(__FUNCTION__);                           \
                        printf("ERROR::%s:%s: ", __FUNCTION__, strerror(errno) );    \
                        printf(__VA_ARGS__);                            \
                        printf("\r\n");                                 \
                        fflush(stdout);

#if 0
#define LOG_INFO(...)     \
                        printf("INFO ::%s: ",__FUNCTION__);   \
                        printf(__VA_ARGS__);                \
                        printf("\r\n");                     \
                        fflush(stdout);
#else
#define LOG_INFO(...)
#endif


/******************************************************************************
 * Function implementations
 *****************************************************************************/
//MyClient::MyClient(void)
void clientSocket_init(void)
{
    if(0 > Sock_Init(NULL))
    {
        LOG_ERROR("error starting ethernet interface!");
    }
    clientSockfd = -1;
}

/******************************************************************************
 * Function implementations
 *****************************************************************************/

void clientSocket_destroy(void)
{
    LOG_INFO("DESCTRUCTOR, close socket: %d", clientSockfd);
    clientSocket_stop();
}

int clientSocket_connectIP(IPAddress_t ip, uint16_t port)
{
    return 0;
}

int clientSocket_connectHost(const char *host, uint16_t port)
{

    clientSockfd = Sock_SetupClient(host, port);
    if(0 > clientSockfd)
    {
        return eSOCK_INVALID_FD;
    }

    return 1;
}

uint8_t clientSocket_connected()
{
    LOG_INFO("connected %d", clientSockfd >= 0);
    return (clientSockfd >= 0);
}

size_t clientSocket_writeMulti(const uint8_t *buf, size_t size)
{
    int n = Sock_Send(clientSockfd, (uint8_t*)buf, size);
    if (0 > n )
    {
        LOG_ERROR("send failed, send[%d] socket:%d", n, clientSockfd);
    }
    LOG_INFO("send %d bytes to server, socket:%d", n, clientSockfd);
    return n;
}

int clientSocket_available()
{
    /*if(0 > Sock_validateSocket(clientSockfd))
    {
        return eSOCK_INVALID_FD;
    }
    */
    return Sock_DataAvailable(clientSockfd);
}

int clientSocket_read()
{
    char buf;
    if(0 > Sock_ReadByte(clientSockfd, &buf))
    {
        LOG_ERROR("no data available?");
        return -1;
    }
    return buf&0xFF;
}

void clientSocket_stop()
{
    Sock_Close(clientSockfd);
    clientSockfd = -1;
}

size_t clientSocket_write(uint8_t data)
{
    LOG_INFO("N O T   I M P L E M E N T E D");
    return 0;
}

int clientSocket_readMulti(uint8_t *buf, size_t size)
{
    LOG_INFO("N O T   I M P L E M E N T E D");
    return 0;
}

int clientSocket_peek()
{
    LOG_INFO("N O T   I M P L E M E N T E D");
    return 0;
}

void clientSocket_flush()
{
    LOG_INFO("N O T   I M P L E M E N T E D");
}
