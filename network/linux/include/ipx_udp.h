/* $Id: ipx_udp.h,v 1.4 2003/03/13 00:20:21 btb Exp $ */
/*
 *
 * FIXME: add description
 *
 */

#ifndef _IPX_UDP_H
#define _IPX_UDP_H

#include "ipx_drv.h"

#define UDP_BASEPORT 28342

extern struct ipx_driver ipx_udp;
extern CNetworkAddress	ipx_MyAddress;
extern CNetworkAddress	ipx_ServerAddress;
extern CNetworkAddress	ipx_LocalAddress;
extern tPort				udpBasePorts [2];
extern tPort				udpClientPort;
extern int32_t				bHaveLocalAddress;

int32_t UDPGetMyAddress();

#endif
