#define UNICODE
#define WIN32_LEAN_AND_MEAN
#define STRICT

#include <iostream>
#include <string>
#include <new>
#include <cstring>

using namespace std;

#include <winsock2.h>
#include <windows.h>


bool Stop=false;
SOCKET UDPSocket = INVALID_SOCKET;
enum ProgramMode { Talk, Listen };


void PrintUsage(void)
{
	cout << "  USAGE:" << endl;
	
	cout << "    Listen mode:" << endl;
	cout << "      udpspeed PORT_NUMBER" << endl;
	cout << endl;
	cout << "    Talk mode:" << endl;
	cout << "      updspeed TARGET_HOST PORT_NUMBER" << endl;
	cout << endl;
	cout << "    ie:" << endl;
	cout << "      Listen Mode:   udpspeed www 342" << endl;
	cout << "      Listen Mode:   udpspeed 10.200.67.1 950" << endl;
	cout << "      Send Mode:     udpspeed 1920" << endl;
	cout << endl;
}


bool VerifyPort(const string &PortString, unsigned long int &PortNumber)
{
for(size_t i = 0; i < PortString.length() ; i++) {
	if(!isdigit(PortString[i])) {
		cout << "  Invalid port: " << PortString << endl;
		cout << "  Ports are specified by numerals only." << endl;
		return false;
		}
	}
PortNumber = atol(PortString.c_str());
if(PortString.length() > 5 || PortNumber > 65535 || PortNumber == 0) {
	cout << "  Invalid port: " << PortString << endl;
	cout << "  Port must be in the range of 1-65535" << endl;
	return false;
	}
	return true;
}


bool InitializeWinsock(void)
{
	WSADATA wsaData;
	WORD wVersionRequested = MAKEWORD(2,2);
	
if(WSAStartup(wVersionRequested, &wsaData)) {
	cout << "Could not initialize Winsock 2.2.";
	return false;
	}
if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
	cout << "Required version of Winsock (2.2) not available.";
	return false;
	}
return true;
}


BOOL ControlHandler(DWORD dwCtrlType)
{
Stop = true;
closesocket(UDPSocket);
return TRUE;
}


bool InitializeOptions(const int &argc, char **argv, enum ProgramMode &Mode, string &TargetHostString, long unsigned int &PortNumber)
{
if(!SetConsoleCtrlHandler((PHANDLER_ROUTINE)ControlHandler,TRUE)) {
	cout << "  Could not add control handler." << endl;
	return false;
	}
if(!InitializeWinsock())
	return false;

string PortString = "";

if(2 == argc) {
	Mode = Listen;
	PortString = argv[1];
	}
else if(3 == argc) {
	Mode = Talk;
	TargetHostString = argv[1];
	PortString = argv[2];
	}
else {
	PrintUsage();
	return false;
	}
if(!VerifyPort(PortString,PortNumber))
	return false;
cout.setf(ios::fixed,ios::floatfield);
cout.precision(0);
return true;
}


void Cleanup(void)
{
// if the program was aborted, flush cout and print a final goodbye
if(Stop) {
	cout.flush();
	cout << endl << "  Stopping." << endl;
	}
// if the socket is still open, close it
if(INVALID_SOCKET != UDPSocket)
	closesocket(UDPSocket);
// shut down winsock
WSACleanup();
// remove the console control handler
SetConsoleCtrlHandler((PHANDLER_ROUTINE)ControlHandler,FALSE);
}



int main(int argc, char **argv)
{
	cout << endl << "udpspeed 1.0 - UDP speed tester" << endl << "Copyright (c) 2002, Shawn Halayka" << endl << endl;

	// set up a mode variable to store whether we are in talk or listen mode
ProgramMode Mode;
// a string to hold the target host's name / IP string
string TargetHostString = "";
// the port number to talk or listen on
long unsigned int PortNumber = 0;

// buffer to hold the message being passed to the other end
const long unsigned int TXBufferSize = 1450; // 1400-1450 magic #?
char TXBuffer[1450];
memset(TXBuffer,'\0',TXBufferSize);

// buffer to hold the message being received from the other end
const long unsigned int RXBufferSize = 8196;
char RXBuffer[8196];


// initialize winsock and all of the program's options
if(!InitializeOptions(argc, argv, Mode, TargetHostString, PortNumber)) {
	Cleanup();
	return 0;
	}

// if talk mode, go into talk mode
if(Talk == Mode) {
	struct sockaddr_in their_addr;
	struct hostent *he;

	he=gethostbyname(TargetHostString.c_str());

	if(NULL == he) {
		cout << "Could not resolve target host." << endl;
		Cleanup();
		return 0;
		}

	their_addr.sin_family = AF_INET;
	their_addr.sin_port = htons((unsigned short int)PortNumber);
	their_addr.sin_addr = *((struct in_addr *)he->h_addr);
	memset(&(their_addr.sin_zero), '\0', 8);

	if(INVALID_SOCKET == (UDPSocket = socket(AF_INET, SOCK_DGRAM, 0))) {
		cout << "  Could not allocate a new socket." << endl;
		Cleanup();
		return 0;
		}

	cout << "  Sending on port " << PortNumber << " - CTRL+C to exit." << endl << endl;

	while(!Stop) {
		if(SOCKET_ERROR == (sendto(UDPSocket, TXBuffer, TXBufferSize, 0, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)))) {
			if(!Stop) cout << " (TX ERR)" << endl;
			break;
			}
		}
	}
// else, go into listen mode
else if(Listen == Mode) {
	struct sockaddr_in my_addr;
	struct sockaddr_in their_addr;
	int addr_len;

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons((unsigned short int)PortNumber);
	my_addr.sin_addr.s_addr = INADDR_ANY;
	memset(&(my_addr.sin_zero), '\0', 8);

	addr_len = sizeof(struct sockaddr);

	if(INVALID_SOCKET == (UDPSocket = socket(AF_INET, SOCK_DGRAM, 0))) {
		cout << "  Could not allocate a new socket." << endl;
		Stop=true;
		}

	if(SOCKET_ERROR == bind(UDPSocket, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))) {
		cout << "  Could not bind socket to port " << PortNumber << "." << endl;
		Stop=true;
		}

	cout << "  Listening on UDP port " << PortNumber << " - CTRL+C to exit." << endl << endl;

DWORD StartLoopTicks = 0;
DWORD EndLoopTicks = 0;
DWORD ElapsedLoopTicks = 0;
QLONG TotalElapsedTicks = 0;
QLONG TotalBytesReceived = 0;
QLONG LastReportedAtTicks = 0;
QLONG LastReportedTotalBytesReceived = 0;
double RecordBPS = 0;
DWORD TempBytesReceived = 0;

	while(!Stop) {
		StartLoopTicks = GetTickCount();
		if(SOCKET_ERROR == (TempBytesReceived = recvfrom(UDPSocket, RXBuffer, RXBufferSize, 0, (struct sockaddr *) &their_addr,&addr_len))) {
			if(!Stop) cout << endl << "  Listening on UDP port " << PortNumber << " - CTRL+C to exit." << endl;
			}
		else {
			TotalBytesReceived += TempBytesReceived;
			}
		EndLoopTicks = GetTickCount();
		if(EndLoopTicks < StartLoopTicks)
			ElapsedLoopTicks = MAXDWORD - StartLoopTicks + EndLoopTicks;
		else
			ElapsedLoopTicks = EndLoopTicks - StartLoopTicks;
		TotalElapsedTicks+=ElapsedLoopTicks;
		if(TotalElapsedTicks>=LastReportedAtTicks+1000) {
			QLONG BytesSentReceivedBetweenReports = TotalBytesReceived - LastReportedTotalBytesReceived;
			double BytesPerSecond = (double)BytesSentReceivedBetweenReports /  (  ( (double)TotalElapsedTicks - (double)LastReportedAtTicks ) / (double)1000.0f  );

			if(BytesPerSecond > RecordBPS)
				RecordBPS = BytesPerSecond;
			LastReportedAtTicks = TotalElapsedTicks;
			LastReportedTotalBytesReceived = TotalBytesReceived;
			cout << "  " << BytesPerSecond << " BPS, Record: " << RecordBPS << " BPS" << endl;
			}
		}
	cout << endl;
	cout << "  Record: " << RecordBPS << " BPS" << endl;
	}
Cleanup();
return 0;
}