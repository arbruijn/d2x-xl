#ifndef __BANLIST_H
#define __BANLIST_H

//-----------------------------------------------------------------------------

int AddPlayerToBanList (char *szPlayer);
int LoadBanList (void);
int SaveBanList (void);
int FindPlayerInBanList (char *szPlayer);
void FreeBanList (void);

//-----------------------------------------------------------------------------

#endif //__BANLIST_H
