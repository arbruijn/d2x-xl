#ifndef __NETMENU_H
#define __NETMENU_H

void NetworkEntropyOptions (void);
void NetworkMonsterballOptions (void);
int NetworkBrowseGames (void);
int NetworkGetGameParams (int bAutoRun);
int NetworkSelectPlayers (int bAutoRun);
void InitNetgameMenu (CMenu& menu, int i);
int NetworkFindGame (void);
int NetworkGetIpAddr (void);
void ShowNetGameInfo (int choice);
void ShowExtraNetGameInfo (int choice);
int NetworkStartPoll (CMenu& menu, int& key, int nCurItem, int nState);
int NetworkEndLevelPoll3 (CMenu& menu, int& key, int nCurItem, int nState);

#define AGI	activeNetGames [choice]
#define AXI activeExtraGameInfo [choice]

#endif //__NETMENU_H
