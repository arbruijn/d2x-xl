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

#ifndef _NOCFILE_H
#define _NOCFILE_H

#include <stdio.h>
#include <io.h>

#define CFILE FILE

#define CFOpen(file,mode) fopen(file,mode)
#define CFLength(f) filelength( fileno( f ))
#define CFWrite(buf,elsize,nelem,fp) fwrite(buf,elsize,nelem,fp)
#define CFRead(buf,elsize,nelem,fp ) fread(buf,elsize,nelem,fp )
#define CFClose( cfile ) fclose( cfile )
#define CFPutC(c,fp) fputc(c,fp)
#define CFGetC(fp) fgetc(fp)
#define CFSeek(fp,offset,where ) fseek(fp,offset,where )
#define CFTell(fp) ftell(fp)
#define CFGetS(buf,n,fp) fgets(buf,n,fp)

#define CF_READ_MODE "rb"
#define CF_WRITE_MODE "wb"

#endif
