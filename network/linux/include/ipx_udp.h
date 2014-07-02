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
extern uint8_t ipx_MyAddress[10];
extern uint8_t ipx_ServerAddress [10];
extern uint8_t ipx_LocalAddress [10];
extern int32_t udpBasePorts [2];
extern int32_t udpClientPort;
extern int32_t bHaveLocalAddress;

int32_t UDPGetMyAddress();

#endif
