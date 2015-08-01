/* $Id: ipx_udp.h,v 1.1 2003/10/05 22:27:01 btb Exp $ */
/*
 *
 * FIXME: add description
 *
 */

#ifndef _IPX_UDP_H
#define _IPX_UDP_H

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
