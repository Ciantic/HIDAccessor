// HIDAccessor.Test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "dllmain.h"

WCHAR szClassName[] = L"TestHidAccessorWin32";
#define MESSAGE_OFFSET (WM_USER + 10)

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	BYTE* buffer = (BYTE*)lParam;
	switch (message) {
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case MESSAGE_OFFSET:
		wprintf(L"Got from device (first three bytes): %02x %02x %02x", buffer[0], buffer[1], buffer[2]);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	HWND hwnd;
	MSG messages;
	WNDCLASSEX wincl;

	wincl.hInstance = hInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = WndProc;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

	if (!RegisterClassEx(&wincl))
		return 0;

	hwnd = CreateWindowEx(0,
		szClassName,
		szClassName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		544,
		375,
		HWND_DESKTOP,
		NULL,
		hInstance,
		NULL
	);

	ShowWindow(hwnd, SW_HIDE);
	TCHAR devicePath[1024];

	// Ergodox Ez: 0xFEED, 0x1307 and Raw HID interface number 1 
	// (when rules.mk has RAW_ENABLE = 1)
	BOOL res = hid_get_path(0xfeed, 0x1307, 1, devicePath, 1024);
	if (!res) {
		fprintf(stderr, "Failure! Path not opened\n");
		std::cin.get();
		return 1;
	}
	wprintf(L"Found, with device path %s\n", devicePath);

	hid_register_reader(devicePath, hwnd, MESSAGE_OFFSET);

	HANDLE hhid = hid_open(devicePath);
	if (!hhid) {
		fprintf(stderr, "Failure! Can't open the path\n");
		std::cin.get();
		return 1;
	}

	BYTE buffer[33] = { 0x0, 0x80 };
	int wroteBytes = hid_write(hhid, buffer, sizeof(buffer));
	if (!wroteBytes) {
		fprintf(stderr, "Failure! Nothing written\n");
		std::cin.get();
		return 1;
	}


	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	hid_close(hhid);


	return messages.wParam;
}



int main()
{
	return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOW);
}

