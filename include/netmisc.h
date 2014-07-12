/*
THE COMPUTER CODE CONTAINED HEREIN IS THE SOLE PROPERTY OF PARALLAX
SOFTWARE CORPORATION ("PARALLAX").  PARALLAX, IN DISTRIBUTING THE CODE TO
END-USERS, AND SUBJECT TO ALL OF THE TERMS AND CONDITIONS HEREIN, GRANTS A
ROYALTY-FREE, PERPETUAL LICENSE TO SUCH END-USERS FOR USE BY SUCH END-USERS
IN USING, DISPLAYING,  AND CREATING DERIVATIVE WORKS THEREOF, SO LONG AS
SUCH USE, DISPLAY OR CREATION IS FOR NON-COMMERCIAL, ROYALTY OR REVENUE
FREE PURPOSES.  IN NO EVENT SHALL THE END-USER USE THE COMPUTER CODE
CONTAINED HEREIN FOR REVENUE-BEARING PURPOSES.  THE END-USER UNDERSTANDS
AND AGREES TO THE TERMS HEREIN AND ACCEPTS THE SAME BY USE OF THIS FILE.
COPYRIGHT 1993-1999 PARALLAX SOFTWARE CORPORATION.  ALL RIGHTS RESERVED.
*/

#ifndef _NETMISC_H
#define _NETMISC_H

#include "multi.h"
#include "network.h"
#include "network_lib.h"

// Returns a checksum of a block of memory.
uint16_t CalcSegmentCheckSum (void);

// Finds the difference between block1 and block2.  Fills in
// diff_buffer and returns the size of diff_buffer.
extern int32_t netmisc_find_diff(void *block1, void *block2, int32_t block_size, void *diff_buffer);

// Applies diff_buffer to block1 to create a new block1.  Returns the
// final size of block1.
extern int32_t netmisc_apply_diff(void *block1, void *diff_buffer, int32_t diff_size);

void BEReceiveNetPlayerInfo (uint8_t *data, tNetPlayerInfo *info);
void BEReceiveNetPlayersPacket (uint8_t *data, CAllNetPlayersInfo *pinfo);
void BESendNetPlayersPacket (uint8_t *server, uint8_t *node);
void BESendSequencePacket (tPlayerSyncData playerSyncData, uint8_t *server, uint8_t *node, uint8_t *netAddress);
void BEReceiveSequencePacket (uint8_t *data, tPlayerSyncData *playerSyncData);
void BESendNetGamePacket (uint8_t *server, uint8_t *node, uint8_t *netAddress, int32_t liteFlag);
void BEReceiveNetGamePacket (uint8_t *data, CNetGameInfo *netgame, int32_t liteFlag);
void BESendExtraGameInfo (uint8_t *server, uint8_t *node, uint8_t *netAddress);
void BEReceiveExtraGameInfo (uint8_t *data, tExtraGameInfo *extraGameInfo);
void BESwapObject (CObject *obj);

#if defined(WORDS_BIGENDIAN) || defined(__BIG_ENDIAN__)

#define ReceiveNetPlayerInfo BEReceiveNetPlayerInfo
#define ReceiveNetPlayersPacket BEReceiveNetPlayersPacket
#define SendNetPlayersPacket BESendNetPlayersPacket
#define SendSequencePacket BESendSequencePacket
#define ReceiveSequencePacket BEReceiveSequencePacket
#define SendNetGamePacket BESendNetGamePacket
#define ReceiveNetGamePacket BEReceiveNetGamePacket
#define SendExtraGameInfo BESendExtraGameInfo
#define ReceiveExtraGameInfo BEReceiveExtraGameInfo
#define SendMissingObjFrames BESendMissingObjFrames
#define ReceiveMissingObjFrames BEReceiveMissingObjFrames
#define SwapObject BESwapObject

// some mac only routines to deal with incorrectly aligned network structures

#define SendBroadcastNetPlayersPacket() \
	BESendNetPlayersPacket(NULL, NULL)
#define SendInternetPlayerSyncData(playerSyncData, server, node) \
	BESendSequencePacket(playerSyncData, server, node, NULL)
#define SendBroadcastPlayerSyncData(playerSyncData) \
	BESendSequencePacket(playerSyncData, NULL, NULL, NULL)
#define SendFullNetGamePacket(server, node, netAddress) \
	BESendNetGamePacket(server, node, netAddress, 0)
#define SendLiteNetGamePacket(server, node, netAddress) \
	BESendNetGamePacket(server, node, netAddress, 1)
#define SendInternetFullNetGamePacket(server, node) \
	BESendNetGamePacket(server, node, NULL, 0)
#define SendInternetLiteNetGamePacket(server, node) \
	BESendNetGamePacket(server, node, NULL, 1)
#define SendBroadcastFullNetGamePacket() \
	BESendNetGamePacket(NULL, NULL, NULL, 0)
#define SendBroadcastLiteNetGamePacket() \
	BESendNetGamePacket(NULL, NULL, NULL, 1)
#define ReceiveFullNetGamePacket(data, netgame) \
	BEReceiveNetGamePacket(data, netgame, 0)
#define ReceiveLiteNetGamePacket(data, netgame) \
	BEReceiveNetGamePacket(data, netgame, 1)

#define SendExtraGameInfoPacket(server, node, netAddress) \
	BESendExtraGameInfo(server, node, netAddress)
#define SendInternetExtraGameInfoPacket(server, node) \
	BESendExtraGameInfo(server, node, NULL)
#define SendBroadcastExtraGameInfoPacket() \
	BESendExtraGameInfo(NULL, NULL, NULL)
#define ReceiveExtraGameInfoPacket(data, _extraGameInfo) \
	BEReceiveExtraGameInfo(data, _extraGameInfo); AddPlayerLoadout(false)

#define SendBroadcastMissingObjFramesPacket() \
	BESendMissingObjFrames(NULL, NULL, NULL)
#define ReceiveMissingObjFramesPacket(data, missingObjFrames) \
	BEReceiveMissingObjFrames((uint8_t *) data, missingObjFrames);
#define SendInternetMissingObjFramesPacket(server, node) \
	BESendMissingObjFrames(server, node, NULL)

#else

#define ReceiveNetPlayersPacket(data, pinfo) \
	memcpy (pinfo, data, int32_t (netPlayers [0].Size ()))
#define SendNetPlayersPacket(server, node) \
	networkThread.Send((uint8_t *)&netPlayers [0], int32_t (netPlayers [0].Size ()), server, node)
#define SendBroadcastNetPlayersPacket() \
	IPXSendBroadcastData((uint8_t *)&netPlayers [0], int32_t (netPlayers [0].Size ()))

#define SendSequencePacket(playerSyncData, server, node, netAddress) \
	networkThread.Send((uint8_t *)&playerSyncData, sizeof(tPlayerSyncData), server, node, netAddress)
#define SendInternetPlayerSyncData(playerSyncData, server, node) \
	networkThread.Send((uint8_t *)&playerSyncData, sizeof(tPlayerSyncData), server, node)
#define SendBroadcastPlayerSyncData(playerSyncData) \
	IPXSendBroadcastData((uint8_t *)&playerSyncData, sizeof(tPlayerSyncData))

#define SendFullNetGamePacket(server, node, netAddress) \
	networkThread.Send((uint8_t *)&netGameInfo.m_info, netgame->Size (), server, node, netAddress)
#define SendLiteNetGamePacket(server, node, netAddress) \
	networkThread.Send((uint8_t *)&netGameInfo.m_info, sizeof(tNetGameInfoLite), server, node, netAddress)
#define SendInternetFullNetGamePacket(server, node) \
	networkThread.Send((uint8_t *)&netGameInfo.m_info, int32_t (netGameInfo.Size ()), server, node)
#define SendInternetLiteNetGamePacket(server, node) \
	networkThread.Send((uint8_t *)&netGameInfo.m_info, sizeof(tNetGameInfoLite), server, node)
#define SendBroadcastFullNetGamePacket() \
	IPXSendBroadcastData((uint8_t *)&netGameInfo.m_info, int32_t (netGameInfo.Size ()))
#define SendBroadcastLiteNetGamePacket() \
	IPXSendBroadcastData((uint8_t *)&netGameInfo.m_info, sizeof(tNetGameInfoLite))
#define ReceiveFullNetGamePacket(data, netgame) \
	memcpy (&(netgame)->m_info, data, (netgame)->Size ())
#define ReceiveLiteNetGamePacket(data, netgame) \
	memcpy (&(netgame)->m_info, data, sizeof(tNetGameInfoLite))

#define SendExtraGameInfoPacket(server, node, netAddress) \
	networkThread.Send((uint8_t *) (extraGameInfo + 1), sizeof(tExtraGameInfo), server, node, netAddress)
#define SendInternetExtraGameInfoPacket(server, node) \
	networkThread.Send((uint8_t *) (extraGameInfo + 1), sizeof(tExtraGameInfo), server, node)
#define SendBroadcastExtraGameInfoPacket() \
	IPXSendBroadcastData((uint8_t *) (extraGameInfo + 1), sizeof(tExtraGameInfo))
#define ReceiveExtraGameInfoPacket(data, _extraGameInfo) \
	memcpy ((uint8_t *)(_extraGameInfo), data, sizeof(tExtraGameInfo)); AddPlayerLoadout (false)

#define SendInternetXMLGameInfoPacket(xmlGameInfo, server, node) \
	networkThread.Send((uint8_t *) xmlGameInfo, (int32_t) strlen(xmlGameInfo) + 1, server, node)

#define SwapObject(obj)

#endif

#endif /* _NETMISC_H */
