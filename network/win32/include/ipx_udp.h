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
extern uint8_t ipx_MyAddress[10];
extern uint8_t ipx_ServerAddress [10];
extern uint8_t ipx_LocalAddress [10];
extern int32_t udpBasePorts [2];

int32_t UDPGetMyAddress();

#endif
