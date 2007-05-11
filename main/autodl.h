#ifndef __autodl_h
#define autodl_h

int SetDlTimeout (int i);
int MaxDlTimeout (void);
int GetDlTimeout (void);
int GetDlTimeoutSecs (void);
int NetworkUpload (ubyte *data);
int NetworkDownload (ubyte *data);
int DownloadMission (char *pszMission);
void CleanUploadDests (void);

extern int bDownloading [];
extern int iDlTimeout;

#endif //__autodl_h
