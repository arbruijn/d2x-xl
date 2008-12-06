/*
   Datei: WSocket.C
          erweiterte WinSock-Ansteuerung
   Autor: D.Mali
 Version: 2.0
   Stand: 13.6.2000
          Copyright (C) PTV AG Karlsruhe
*/

#include <string.h>
#include "wsocket.h"

// Error Codes / Error Strings

static tSocketMsg socketMsgs [] =
   {
    {WSAEFAULT, "Bad address"},
    {WSAEACCES, "Permission denied"},
    {WSAECONNREFUSED, "Channel refused"},
    {WSAEINTR, "Interrupted function call"},
    {WSAEINVAL, "Invalid argument"},
    {WSAEMFILE, "Too many open files"},
    {WSAEWOULDBLOCK, "Resource temporarily unavailable"},
    {WSAEINPROGRESS, "Operation now in progress"},
    {WSAEALREADY, "Operation already in progress"},
    {WSAENOTSOCK, "Socket operation on non-socket"},
    {WSAEDESTADDRREQ, "Destination address required"},
    {WSAEMSGSIZE, "Message too long"},
    {WSAEPROTOTYPE, "Protocol wrong nType for socket"},
    {WSAENOPROTOOPT, "Bad protocol option"},
    {WSAEPROTONOSUPPORT, "Protocol not supported"},
    {WSAESOCKTNOSUPPORT, "Socket nType not supported"},
    {WSAEOPNOTSUPP, "Operation not supported"},
    {WSAEPFNOSUPPORT, "Protocol family not supported"},
    {WSAEAFNOSUPPORT, "Address family not supported by protocol family"},
    {WSAEADDRINUSE, "Address already in use"},
    {WSAEADDRNOTAVAIL, "Cannot assign requested address"},
    {WSAENETDOWN, "Network is down"},
    {WSAENETUNREACH, "Network is unreachable"},
    {WSAENETRESET, "Network dropped channel on reset"},
    {WSAECONNABORTED, "Software caused channel abort"},
    {WSAECONNRESET, "Channel reset by peer"},
    {WSAENOBUFS, "No buffer space available"},
    {WSAEISCONN, "Socket is already connected"},
    {WSAENOTCONN, "Socket is not connected"},
    {WSAESHUTDOWN, "Cannot send after socket shutdown"},
    {WSAETIMEDOUT, "Channel timed out"},
    {WSAEHOSTDOWN, "Host is down"},
    {WSAEHOSTUNREACH, "No route to host"},
    {WSAEPROCLIM, "Too many processes"},
    {WSASYSNOTREADY, "Network subsystem is unavailable"},
    {WSAVERNOTSUPPORTED, "WINSOCK.DLL version out of range"},
    {WSANOTINITIALISED, "Successful WSAStartup not yet performed"},
    {WSAEDISCON, "Graceful shutdown in progress"},
    {WSAHOST_NOT_FOUND, "Host not found"},
    {WSATRY_AGAIN, "Non-authoritative host not found"},
    {WSANO_RECOVERY, "This is a non-recoverable error"},
    {WSANO_DATA, "Valid name, no data record of requested nType"},
//    {WSA_INVALID_HANDLE, "Specified event CObject handle is invalid"},
//    {WSA_INVALID_PARAMETER, "One or more parameters are invalid"},
//    {WSAINVALIDPROCTABLE, "Invalid procedure table from service provider"},
//    {WSAINVALIDPROVIDER, "Invalid service provider version number"},
//    {WSA_IO_PENDING, "Overlapped operations will complete later"},
//    {WSA_IO_INCOMPLETE, "Overlapped I/O event CObject not in signaled state"},
//    {WSA_NOT_ENOUGH_MEMORY, "Insufficient memory available"},
//    {WSAPROVIDERFAILEDINIT, "Unable to initialize a service provider"},
//    {WSASYSCALLFAILURE, "System call failure"},
//    {WSA_OPERATION_ABORTED, "Overlapped operation aborted"},
    {0, NULL}
   };

static tSockServerData	ssd;
static int sockInstC = 0;

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short SockClose (void);

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

#define SEL_READ     1
#define SEL_WRITE    2
#define SEL_ERROR    4

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

LPSTR GetWinSockMessage (short msgCode)
{
   tSocketMsg *     p;
   static char    defMsgText [80];

for (p = socketMsgs; p->msgText; p++)
   if (p->msgCode == msgCode)
      return p->msgText;
wsprintf (defMsgText, "WinSock Error %d", msgCode);
return defMsgText;
}

								/*---------------------------*/

static short WinSockMsg (short msgCode, LPSTR msgText)
{
ssd.status.msgText = (msgText ? msgText : GetWinSockMessage (msgCode));
return ssd.status.msgCode = msgCode;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short RegisterChannel (SOCKET hChannel)
{
if (ssd.nChannels == MAX_PLAYERS)
   return WinSockMsg (-1, "game is already full");
ssd.channels [ssd.nChannels++] = hChannel;
return 0;
}

								/*---------------------------*/

short FindChannel (SOCKET hChannel)
{
	short	i;

for (i = 0; i < ssd.nChannels; i++)
	if (ssd.channels [i] == hChannel)
		return i;
return -1;
}

								/*---------------------------*/

short UnregisterChannel (SOCKET hChannel)
{
	short	i = FindChannel (hChannel);

if (i < 0)
	return -1;
if (i < --(ssd.nChannels))
	ssd.channels [i] = ssd.channels [ssd.nChannels];
ssd.channels [ssd.nChannels] = INVALID_SOCKET;
if (ssd.iChannel >= ssd.nChannels)
	ssd.iChannel = 0;
return 0;
}

								/*---------------------------*/

short SetCurrentChannel (SOCKET hChannel)
{
	short	i = FindChannel (hChannel);

if (i < 0)
	return -1;
ssd.iChannel = i;
return 0;
}

								/*---------------------------*/

SOCKET GetCurrentChannel (void)
{
return CURRENT_CHANNEL;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short FindServerByChannel (SOCKET socket)
{
return (FindChannel (socket) < 0) ? -1 : 0;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short SockInit (SOCKET socket)
{
   short	i;
   
if (!sockInstC && (i = InitWinsock ()))
   return i;
memset (&ssd, 0, sizeof (ssd));
ssd.socket = INVALID_SOCKET;
for (i = 0; i < MAX_PLAYERS; i++)
	ssd.channels [i] = INVALID_SOCKET;
sockInstC++;
ssd.socket = socket;
return 0;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short SockEnd ()
{
SockClose ();
if (!--sockInstC)
   TerminateWinsock ();
return 0;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short SockSelect (SOCKET socket, long lTimeout, short flags)
{
   fd_set			fds [3], * fdp [3];
   struct timeval timeout;
   short           i;
   int            nError;
   
ssd.status.msgCode = 0;
ssd.status.msgText = "";
timeout.tv_sec = lTimeout / 1000;     // n seconds timeout
timeout.tv_usec = lTimeout % 1000;    // ms
for (i = 0; i < 3; i++, flags >>= 1) {
   FD_ZERO (fds + i);
   if (flags & 1) {
      FD_SET (socket, fds + i);
      fdp [i] = fds + i;
      }
   else
      fdp [i] = NULL;
   }
nError = select (FD_SETSIZE/*socket + 1*/, fdp [0], fdp [1], fdp [2], &timeout);
if (nError == WSAEINTR) // Interrupted ?
   return 0;
if (nError == SOCKET_ERROR) {
   WinSockMsg (WSAGetLastError (), NULL);
	return nError;
	}
for (flags = 0, i = 2; i >= 0; i--, flags <<= 1)
   if (fdp [i] && FD_ISSET (socket, fds + i))
      flags |= 1;
return flags;      
}

/*---------------------------------------------------------------------------+
| SockRecvReady:                                                             |
|    Checks for available data                                               |
| Return Code:                                                               |
|    Data available: 0                                                |
|    otherwise E_PTV_NOTFOUND                                                |
+---------------------------------------------------------------------------*/

short SockRecvReady (long lTimeout)
{
   short  sockRes;

if (ssd.bListen && !ssd.nChannels)
   return -1;
sockRes = SockSelect (CURRENT_CHANNEL, lTimeout, SEL_READ);
if (sockRes == SOCKET_ERROR)
   return ssd.status.msgCode;
if (sockRes == 0)
   return -1;
return 0;
}

/*---------------------------------------------------------------------------+
| SockSendReady:                                                             |
|    Checks whether data transfer possible                                   |
| Return Code:                                                               |
|    Channelect requested: 0                                          |
|    otherwise E_PTV_NOTFOUND                                                |
+---------------------------------------------------------------------------*/


short SockSendReady (long lTimeout)
{
   short  sockRes;
   
if (ssd.bListen && !ssd.nChannels)
   return -1;
sockRes = SockSelect (CURRENT_CHANNEL, lTimeout, SEL_WRITE);
if (sockRes == SOCKET_ERROR)
   return ssd.status.msgCode;
if (sockRes == 0)
   return -1;
return 0;
}

/*---------------------------------------------------------------------------+
| SockAcceptReady:                                                           |
|    Checks for pending connect calls                                        |
| Return Code:                                                               |
|    Channel requested: 0                                             |
|    otherwise E_PTV_NOTFOUND                                                |
+---------------------------------------------------------------------------*/

short SockAcceptReady (long lTimeout)
{
   short  sockRes;

if (!ssd.bListen)
   return WinSockMsg (-1, "socket is not in listening mode");
sockRes = SockSelect (ssd.socket, lTimeout, SEL_READ);
if (sockRes == SOCKET_ERROR)
   return ssd.status.msgCode;
if (sockRes == 0)
   return -1;
return 0;
}

/*---------------------------------------------------------------------------+
| GetSocket:                                                                 |
|    retrieves a socket.                                                     |
+---------------------------------------------------------------------------*/

static short GetSocket (void)
{
   short  sockRes;
   
ssd.status.msgCode = 0;
ssd.status.msgText = "";
if (0 > (long) (ssd.socket = socket (AF_INET, SOCK_STREAM, 0)))
   return WinSockMsg (WSAGetLastError (), NULL);
if (ssd.bAsync && ssd.hWnd &&
    (WSAAsyncSelect (ssd.socket, ssd.hWnd, WM_SOCKET_NOTIFY, FD_ACCEPT | FD_CLOSE) == SOCKET_ERROR))
   return WinSockMsg (WSAGetLastError (), NULL);
return 0;   
}

/*---------------------------------------------------------------------------+
| listen:                                                                    |
|    creates a listening socket on the specified &ssd.                       |
+---------------------------------------------------------------------------*/

short SockListen (UINT port)
{
   struct sockaddr_in server;

if (GetSocket ())
   return ssd.status.msgCode;
// fill in server structure
ZeroMemory (&server, sizeof (server));
server.sin_family = AF_INET;
server.sin_addr.s_addr = htonl (INADDR_ANY);
server.sin_port = htons (port);
// bind address to socket
if (bind (ssd.socket, reinterpret_cast<struct sockaddr*> (&server), sizeof (server)) < 0) {
   closesocket (ssd.socket);
   return WinSockMsg (WSAGetLastError (), NULL);
   }
// get the local Host name
if (gethostname (ssd.localHostName, sizeof (ssd.localHostName)) == SOCKET_ERROR) {
   closesocket (ssd.socket);
   return WinSockMsg (WSAGetLastError (), NULL);
   }
// listen
if (listen (ssd.socket, SOMAXCONN) < 0) {
   closesocket (ssd.socket);
   return WinSockMsg (WSAGetLastError (), NULL);
   }
ssd.port = port;
ssd.bListen = TRUE;
return 0;
}

/*---------------------------------------------------------------------------+
| accept:                                                                    |
|    accept a new Channel channel                                            |
+---------------------------------------------------------------------------*/

short SockAccept (void)
{
   SOCKET      channel;
   struct      sockaddr_in channelAddr;
   int         nLength = sizeof (channelAddr);
   short       sockRes;

if (!ssd.bListen)
   return WinSockMsg (-1, "socket not in listening mode");
if (ssd.nChannels == MAX_PLAYERS)
   return WinSockMsg (-1, "server is full");
ssd.status.msgCode = 0;
ssd.status.msgText = "";
if (0 > (long) (channel = accept (ssd.socket, reinterpret_cast<struct sockaddr*> (&channelAddr), &nLength))) {
   WinSockMsg (WSAGetLastError (), NULL);
   return -1;
   }
return RegisterChannel (channel);
}

/*---------------------------------------------------------------------------+
| CheckIpAddr:                                                               |
|    check whether name contains a numerical IP address in the format        |
|    ###.###.###.### (# e [0,9].                                             |
| return codes:                                                              |
|    0: numerical IP address                                          |
|    -1: generic host name                                         |
+---------------------------------------------------------------------------*/

static short CheckIpAddr (LPSTR name)
{
   short i, j, fieldC, digitC;
	long	h;

for (i = 0, fieldC = 0; name [i]; i++) 
	{
	if (name [i] == ' ')
		continue;
   if (!name [i])
      break;
   if (!isdigit (name [i]))
      return 1;
	h = atol (name + i);
   if (h > 255)
      return 1;
   if (++fieldC > 4)
      return 1;
	while (name [i] == ' ')
		i++;
   if (name [i] && (name [i] != '.'))
      return 1;
   }
return (fieldC == 4) ? 0 : 1;
}

/*---------------------------------------------------------------------------+
| connect:                                                                   |
|    connects to a server on a specified port number.                        |
+---------------------------------------------------------------------------*/

short SockConnect (LPSTR name, UINT port)
{
   struct sockaddr_in   server;
   struct hostent       far * hp;
   
ssd.status.msgCode = 0;
ssd.status.msgText = "";
ssd.socket = INVALID_SOCKET;
if (!(name && *name))
   return WinSockMsg (-1, "Invalid Server Name");
   
ZeroMemory (&server, sizeof (server));
if (!CheckIpAddr (name)) {
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = inet_addr (name);
   }
else {  
   if (!(hp = gethostbyname (name))) 
      return WinSockMsg (WSAGetLastError (), NULL);
   memcpy (&server.sin_addr, &(hp->h_addr), hp->h_length);      
   server.sin_family = hp->h_addrtype;
   }
server.sin_port = htons (port);
   
// create socket
if (GetSocket ())
   return ssd.status.msgCode;

// connect to server.
if (connect (ssd.socket, reinterpret_cast<struct sockaddr*> (&server), sizeof (server)) == SOCKET_ERROR)
   return WinSockMsg (WSAGetLastError (), NULL);
ssd.port = port;
ssd.bListen = FALSE;
return 0;   
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short CloseChannel (SOCKET hChannel)
{
if (UnregisterChannel (hChannel))
   return -1;
closesocket (hChannel);
return 0;
}

/*---------------------------------------------------------------------------+
| close:                                                                     |
|    close the Socket handle                                                 |
+---------------------------------------------------------------------------*/

short SockClose (void)
{
if (ssd.socket == INVALID_SOCKET)
   return WinSockMsg (-1, "SockClose: invalid socket specified");
ssd.status.msgCode = 0;
ssd.status.msgText = "";
while (ssd.nChannels)
   closesocket (ssd.channels [0]);
if (closesocket (ssd.socket) == SOCKET_ERROR)
   return WinSockMsg (WSAGetLastError (), NULL);
ssd.socket = INVALID_SOCKET;
return 0;   
}

/*---------------------------------------------------------------------------+
|                                                                            |
| Read:                                                                      |
|   Reads some bytes from the specified socket into szBuffer of              |
|    size nLength.                                                           |
|                                                                            |
|    recv = 0 Bytes ist eigentlich kein Fehler.                              |
|    Exception auch bei recv = 0 Bytes sinnvoll ? (Fehlercode)               |
+---------------------------------------------------------------------------*/

long SockRead (LPSTR szBuffer, UINT nLength)
{
   long     nGet, nGotten;
   LPSTR    ps;
   SOCKET   socket = CURRENT_CHANNEL;

ssd.status.msgCode = 0;
ssd.status.msgText = "";
for (ps = szBuffer, nGet = nLength; nGet; nGet -= nGotten, ps += nGotten)
   if ((nGotten = recv (socket, ps, nGet, 0)) <= 0)
      return WinSockMsg (WSAGetLastError (), NULL);
return nLength;
}

/*---------------------------------------------------------------------------+
| Write:                                                                     |
|    sends a buffer buf of size size to the specified socket                 |
+---------------------------------------------------------------------------*/

long SockWrite (LPSTR buf, UINT nLength)
{
   long    nSent, nSend;
   LPSTR    ps;
   SOCKET   socket = CURRENT_CHANNEL;

for (ps = buf, nSend = nLength; nSend; nSend -= nSent, ps += nSent)
   if ((nSent = send (socket, ps, nSend, 0)) == SOCKET_ERROR)
      return WinSockMsg (WSAGetLastError (), NULL);
return nLength;
}

/*---------------------------------------------------------------------------+
| WriteLong:                                                                 |
|    Writes a long Value to the Socket                                       |
+---------------------------------------------------------------------------*/

long SockWriteLong (long lval)
 {
   long ltmp = htonl (lval);

return SockWrite ((LPSTR) &ltmp, sizeof (ltmp));
}

/*---------------------------------------------------------------------------+
| ReadLong:                                                                  |
|    Reads a long Value from the Socket                                      |
+---------------------------------------------------------------------------*/

long SockReadLong (void)
 {
   long ltmp, lval;
   
if (SockRead ((LPSTR) &ltmp, sizeof (ltmp)) < 0)
   return ssd.status.msgCode;
lval = ntohl (ltmp);
return lval;
}

/*---------------------------------------------------------------------------+
| SockLWrite: Write variable-length buffer to socket                         |
+---------------------------------------------------------------------------*/

long SockLWrite (LPSTR szBuffer, long len)
{
if (SockWriteLong (len) < 0)
   return ssd.status.msgCode;
return SockWrite (szBuffer, len);
}

/*---------------------------------------------------------------------------+
| SockLRead: read variable-length buffer from socket                         |
| return code: buffer length                                                 |
+---------------------------------------------------------------------------*/

long SockLRead (LPSTR szBuffer, long maxlen)
 {
   long buflen;
   
// get the Buffer Length
buflen = SockReadLong ();
if (maxlen && (buflen > maxlen))
   return WinSockMsg (-1, "SockLRead: Buffer overflow");
// get the Buffer
SockRead (szBuffer, buflen);
return buflen;
}

/*---------------------------------------------------------------------------+
|                                                                            |
+---------------------------------------------------------------------------*/

short SockHandler (UINT socket, LPARAM lEvent)
{
switch (WSAGETSELECTEVENT (lEvent))
   {
   case FD_READ:
      break;
   case FD_WRITE:
      break;
   case FD_OOB:
      break;
   case FD_ACCEPT:
      if (ssd.socket != socket)
         return -1;
      return SockAccept ();
      break;
   case FD_CONNECT:
      break;
   case FD_CLOSE:
      if (ssd.socket != socket)
         return -1;
      return SockClose ();
      break;
   }
return 0;   
}

/*---------------------------------------------------------------------------+
| InitWinsock:                                                               |
|              starts up winsock.dll or wsock32.dll                          |
+---------------------------------------------------------------------------*/

short InitWinsock ()
 {
   WSADATA wsData;
   
   // change 1,1 to 2,0 if using 2.0
   WORD dwVersion = MAKEWORD (2,0);  // winsock version

return (WSAStartup (dwVersion, &wsData) != 0 ? WSAGetLastError () : 0);
}

/*---------------------------------------------------------------------------+
| TerminateWinsock                                                           |
|    CleanUp Winsock DLL                                                     |
| Exceptions:                                                                |
|    throws EWinSockError                                                    |
+---------------------------------------------------------------------------*/

short TerminateWinsock ()
{
G3_SLEEP(50);
G3_SLEEP(50);
return (WSACleanup() == SOCKET_ERROR ? WSAGetLastError () : 0);
}

