#ifndef __NETMENU_H
#define __NETMENU_H

int NetworkBrowseGames (void);
int NetworkGetGameParams (int bAutoRun);
int NetworkSelectPlayers (int bAutoRun);
void InitNetgameMenu (newmenu_item *m, int i);
int NetworkFindGame (void);
int NetworkGetIpAddr (void);

#endif //__NETMENU_H
