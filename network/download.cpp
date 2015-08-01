// A file download subsystem


#include "download.h"

/**
Download a file

Pass the URL of the file to url

To specify an update function that is called after each buffer is read, pass a
pointer to that function as the third parameter. If no update function is
desired, then let the third parameter default to null.
*/
bool Download::download(char *url, bool reload, void (*update)(unsigned long, unsigned long))
{
    ofstream fout;              // output stream
    unsigned char buf[BUF_SIZE];// input buffer
    unsigned long numrcved;     // number of bytes read
    unsigned long filelen;      // length of the file on disk
    HINTERNET hIurl, hInet;     // internet handles
    unsigned long contentlen;   // length of content
    unsigned long len;          // length of contentlen
    unsigned long total = 0;    // running total of bytes received
    char header[80];            // holds Range header

    try
    {
        if(!ishttp(url))
            throw DLExc("Must be HTTP url");

        /*
        Open the file spcified by url.
        The open stream will be returned in fout. If reload is true, then any
        preexisting file will be truncated. The length of any preexisting file
        (after possible truncation) is returned.
        */
        filelen = openfile(url, reload, fout);

        // See if internet connection is available
        if(InternetAttemptConnect(0) != ERROR_SUCCESS)
            throw DLExc("Can't connect");

        // Open internet connection
        hInet = InternetOpen("downloader", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if(hInet == NULL)
            throw DLExc("Can't open connection");

        // Construct header requesting range of data
        sprintf(header, "Range:bytes=%d-", filelen);

        // Open the URL and request range
        //hIurl = InternetOpenUrl(hInet, url, header, -1, INTERNET_FLAG_NO_CACHE_WRITE, 0);
        hIurl = InternetOpenUrl(hInet, url, header, strlen(header), INTERNET_FLAG_NO_CACHE_WRITE, 0);
        if(hIurl == NULL)
            throw DLExc("Can't open url");

        // Confirm that HTTP/1.1 or greater is supported
        if(!httpverOK(hIurl))
            throw DLExc("HTTP/1.1 not supported");

        // Get content length
        len = sizeof contentlen;
        if(!HttpQueryInfo(hIurl, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &contentlen, &len, NULL))
            throw DLExc("File or content length not found");

        // If existing file (if any) is not complete, then finish downloading
        if(filelen != contentlen && contentlen)
        {
            do
            {
                // Read a buffer of info
                if(!InternetReadFile(hIurl, &buf, BUF_SIZE, &numrcved))
                    throw DLExc("Error occurred during download");

                // Write buffer to disk
                fout.write((const char *) buf, numrcved);
                if(!fout.good())
                    throw DLExc("Error writing file");

                // update running total
                total += numrcved;

                // Call update function, if specified
                if(update && numrcved > 0)
                    update(contentlen + filelen, total + filelen);
            } while (numrcved > 0);
        }
        else
        {
            if(update)
                update(filelen, filelen);
        }
    }
    catch (DLExc)
    {
        fout.close();
        InternetCloseHandle(hIurl);
        InternetCloseHandle(hInet);

        // rethrow the exception for use by the caller
        throw;
    }

    fout.close();
    InternetCloseHandle(hIurl);
    InternetCloseHandle(hInet);

    return true;
}


// Return true if HTTP version of 1.1 or greater
bool Download::httpverOK(HINTERNET hIurl)
{
    char str[80];
    unsigned long len = 79;

    if(!HttpQueryInfo(hIurl, HTTP_QUERY_VERSION, &str, &len, NULL))
        return false;

    // First, check major version number
    char *p = strchr(str, '/');
    p++;
    if(*p == '0')
        return false;       // can't use HTTP 0.x

    // Now, find start of minor HTTP version number
    p = strchr(str, '.');
    p++;

    // convert to int
    int minorVerNum = atoi(p);

    if(minorVerNum > 0)
        return true;

    return false;
}

// Extract the filename from the URL.
// Return false if the filename cannot be found
bool Download::getfname(char *url, char *fname)
{
    // Find last slash /
    char *p = strrchr(url, '/');

    // Copy filename afther the last slash
    if(p && (strlen(p) < MAX_FILENAME_SIZE))
    {
        p++;
        strcpy(fname, p);
        return true;
    }
    else
    {
        return false;
    }
}

/*
Open the output file, initialize the output stream, and return the file's
length. If reload is true, first truncate any preexisting file
*/
unsigned long Download::openfile(char *url, bool reload, ofstream &fout)
{
    char fname[MAX_FILENAME_SIZE];

    if(!getfname(url, fname))
        throw DLExc("File name error");

    if(!reload)
        fout.open(fname, ios::binary | ios::out | ios::app | ios::ate);
    else
        fout.open(fname, ios::binary | ios::out | ios::trunc);

    if(!fout)
        throw DLExc("Can't open output file");

    // get current file length
    return fout.tellp();
}

// Confirm that the URL specifies HTTP
bool Download::ishttp(char *url)
{
    char str[5] = "";

    // get the first four characters from the URL
    strncpy(str, url, 4);

    // convert to lowercase
    for(char *p = str; *p; p++)
        *p = tolower(*p);

    return !strcmp("http", str);
}
