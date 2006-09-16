// SocketClass.h: interface for the CSocketClass class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __WSOCKET_H
#define __WSOCKET_H

#ifdef _WIN32

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#pragma pack(push,2)
#  include <winsock.h>
#pragma pack(pop)

#include "player.h"

// Konstanten 
#define LOCALHOST "127.0.0.1"

typedef struct tSocketMsg {
   short		msgCode;
   char		*msgText;
   } tSocketMsg;

typedef tSocketMsg * pSocketMsg;

typedef struct tSockServerData {
   UINT           port;		//port number
   SOCKET         socket;	//base socket
   char           localHostName [512];
   SOCKET         channels [MAX_PLAYERS];	//communication sockets
	short				nChannels;
	short				iChannel;
   tSocketMsg     status;
   HWND           hWnd;
   BOOL           bListen;
   BOOL           bAsync;
   } tSockServerData;

typedef tSockServerData	*pSockServerData;
typedef pSockServerData	*ppSockServerData;

#ifdef __cplusplus
extern "C" BEGIN
#endif

// Init / Terminate
short InitWinsock ();
short TerminateWinsock ();

short SockInit (SOCKET hSocket);
short SockEnd ();

// Channel Methods
short SockConnect (LPSTR name, UINT nPort);
short SockClose ();

// Server Methods
short SockListen (UINT nPort);
short SockAccept ();
short CloseChannel (SOCKET hSocket);

UINT GetLocalPort ();
LPSTR GetLocalHostName ();

// Read / Write Buffer with fixed Length
long SockWrite (LPSTR szBuffer, UINT nSize);
long SockRead (LPSTR szBuffer, UINT nLength);

// Data available ?
short SockSendReady (long lTimeout);
short SockRecvReady (long lTimeout);
short SockAcceptReady (long lTimeout);

// Read / Write Buffer with variable Length
long SockLWrite (LPSTR szBuffer, long len);
long SockLRead (LPSTR szBuffer, long maxlen);

// Read / Write Long value
long SockWriteLong (long lval);
long SockReadLong ();

LPSTR GetWinSockMessage (short msgCode);
short SockHandler (UINT socket, LPARAM lEvent);

short FindChannel (SOCKET hChannel);
short SetCurrentChannel (SOCKET hChannel);
SOCKET GetCurrentChannel ();

#ifdef __cplusplus
END
#endif

extern tSockServerData	ssd;

#define CURRENT_CHANNEL (ssd.nChannels ? ssd.channels [ssd.iChannel] : ssd.socket)

#define SWITCH_CHANNEL	if (ssd.nChannels) ssd.iChannel = (ssd.iChannel + 1) % ssd.nChannels

#ifndef WM_SOCKET_NOTIFY
#  define WM_SOCKET_NOTIFY    WM_USER+1
#endif

#endif

#endif /*__WSOCKET_H*/

