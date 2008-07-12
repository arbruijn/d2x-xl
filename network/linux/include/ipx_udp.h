/* $Id: ipx_udp.h,v 1.4 2003/03/13 00:20:21 btb Exp $ */
/*
 *
 * FIXME: add description
 *
 */

#ifndef _IPX_UDP_H
#define _IPX_UDP_H

#include "ipx_drv.h"

#define UDP_BASEPORT 283


extern struct ipx_driver ipx_udp;
extern unsigned char ipx_MyAddress[10];
extern unsigned char ipx_ServerAddress [10];
extern unsigned char ipx_LocalAddress [10];
extern int udpBasePort [2];
extern int udpClientPort;
extern int bHaveLocalAddress;

int UDPGetMyAddress();

#endif
