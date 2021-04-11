// Header file for downloader.


#ifndef DOWNLOAD_H
#define DOWNLOAD_H

//#include <iostream>
#include <string>
#pragma pack(push)
#pragma pack(8)
#include <windows.h>
#pragma pack(pop)
#include <wininet.h>
#include <fstream>

using namespace std;

const int32_t MAX_ERRMSG_SIZE = 80;
const int32_t MAX_FILENAME_SIZE = 512;
const int32_t BUF_SIZE = 10240;             // 10 KB


// Exception class for donwload errors;
class DLExc
{
private:
    char err[MAX_ERRMSG_SIZE];
public:
    DLExc(char *exc)
    {
        if(strlen(exc) < MAX_ERRMSG_SIZE)
            strcpy(err, exc);
    }

    // Return a pointer to the error message
    const char *geterr()
    {
        return err;
    }
};


// A class for downloading files from the internet
class Download
{
private:
    static bool ishttp(char *url);
    static bool httpverOK(HINTERNET hIurl);
    static bool getfname(char *url, char *fname);
    static uint32_t openfile(char *url, bool reload, ofstream &fout);
public:
    static bool download(char *url, bool reload=false, void (*update)(uint32_t, uint32_t)=NULL);
};


#endif
