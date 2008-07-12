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
extern unsigned char ipx_MyAddress[10];
extern unsigned char ipx_ServerAddress [10];
extern unsigned char ipx_LocalAddress [10];
extern int udpBasePorts [2];

int UDPGetMyAddress();

#endif
