#ifndef __NETMENU_H
#define __NETMENU_H

void NetworkEntropyOptions (void);
void NetworkMonsterballOptions (void);
int32_t NetworkBrowseGames (void);
int32_t NetworkGetGameParams (int32_t bAutoRun);
int32_t NetworkSelectPlayers (int32_t bAutoRun);
void InitNetGameMenu (CMenu& menu, int32_t i);
int32_t NetworkFindGame (void);
int32_t NetworkGetIpAddr (bool bServer = false, bool bUDP = true);
void ShowNetGameInfo (int32_t choice);
void ShowExtraNetGameInfo (int32_t choice);
char* XMLGameInfo (void);
int32_t NetworkStartPoll (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState);
int32_t NetworkEndLevelPoll3 (CMenu& menu, int32_t& key, int32_t nCurItem, int32_t nState);

#define AGI	activeNetGames [choice]
#define AXI activeExtraGameInfo [choice]

#endif //__NETMENU_H
