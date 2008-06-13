/* $Id: 
,v 1.2 2003/10/10 09:36:35 btb Exp $ */
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


#ifndef _STATE_H
#define _STATE_H

int StateSaveAll (int betweenLevels, int secret_save, char *filename_override);
int StateRestoreAll (int in_game, int secret_restore, char *filename_override);
int StateSaveAllSub (const char *filename, const char *desc, int betweenLevels);
int StateRestoreAllSub (const char *filename, int multi, int secret_restore);
int StateGetSaveFile (char *fname, char * dsc, int multi);
int StateGetRestoreFile (const char *fname, int multi);
void SetPosFromReturnSegment (int bRelink);


#endif /* _STATE_H */
