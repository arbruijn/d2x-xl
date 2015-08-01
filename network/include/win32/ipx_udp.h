/*
 *
 * FIXME: add description
 *
 */

#ifndef _IPX_UDP_H
#define _IPX_UDP_H

#define D2XUDP						"D2XUDP"
#define UDP_BASEPORT				28342
#define PORTSHIFT_TOLERANCE	0x100
#define MAX_BRDINTERFACES		16

extern struct ipx_driver ipx_udp;
extern unsigned char ipx_MyAddress[10];

int UDPGetMyAddress();

#endif
