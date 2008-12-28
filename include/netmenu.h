#ifndef __NETMENU_H
#define __NETMENU_H

int NetworkBrowseGames (void);
int NetworkGetGameParams (int bAutoRun);
int NetworkSelectPlayers (int bAutoRun);
void InitNetgameMenu (CMenuItem *m, int i);
int NetworkFindGame (void);
int NetworkGetIpAddr (void);
void ShowNetGameInfo (int choice);
void ShowExtraNetGameInfo (int choice);
int NetworkEndLevelPoll3 (CMenu& menu, int& key, int nCurItem);

#endif //__NETMENU_H
