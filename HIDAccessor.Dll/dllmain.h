#pragma once
#include "stdafx.h"

#define DllExport   __declspec( dllexport )

// Handle and message offset
std::thread* _readerThread = NULL;
HWND _readerHwnd = 0;
unsigned int _readerMessageOffset = 0;
HANDLE _readerHHid = NULL;
BOOL _killReaderThread = FALSE;

/// Get HID path
///
/// According to MSDN the given path does not change between computer restarts
BOOL DllExport hid_get_path( // Returns True on success
	_In_ unsigned int vid, // Vendor ID
	_In_ unsigned int pid, // Process ID
	_In_ int interfaceNumber,  // Interface number
	_Out_writes_z_(outDevicePathLen) wchar_t* outDevicePath, // Device path is written here
	_In_ rsize_t outDevicePathLen // Device path length as characters (not bytes!)
) {
	BOOL res;
	int deviceIndex = 0;

	// Detail data (device path)
	SP_DEVICE_INTERFACE_DETAIL_DATA *devInterfaceDetailData = NULL;

	// Interface data
	SP_DEVICE_INTERFACE_DATA devInterfaceData;
	devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	// Info set
	GUID interfaceClassGuid = { 0x4d1e55b2, 0xf16f, 0x11cf,{ 0x88, 0xcb, 0x00, 0x11, 0x11, 0x00, 0x00, 0x30 } };
	HDEVINFO hDevInfoSet = SetupDiGetClassDevs(&interfaceClassGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

	while (true) {
		DWORD devicePathSize = 0;

		res = SetupDiEnumDeviceInterfaces(hDevInfoSet,
			NULL,
			&interfaceClassGuid,
			deviceIndex++,
			&devInterfaceData);

		if (!res) {
			// No more devices
			break;
		}

		// Get device path string size
		SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
			&devInterfaceData,
			NULL,
			0,
			&devicePathSize,
			NULL);

		// Unable to get device path size
		if (!devicePathSize) {
			continue;
		}

		// Allocate memory for devicePath
		devInterfaceDetailData = (SP_DEVICE_INTERFACE_DETAIL_DATA*) malloc(devicePathSize);
		devInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		// Get the devicePath
		res = SetupDiGetDeviceInterfaceDetail(hDevInfoSet,
			&devInterfaceData,
			devInterfaceDetailData,
			devicePathSize,
			NULL,
			NULL);

		// Get failed, continue
		if (!res) {
			free(devInterfaceDetailData);
			continue;
		}
		
		// Compare the device path with given vid, pid and interfaceNumber
		wchar_t compare[512];
		wsprintf(compare, L"\\\\?\\hid#vid_%04x&pid_%04x&mi_%02x", vid, pid, interfaceNumber);
		int compareResult = wmemcmp(devInterfaceDetailData->DevicePath, compare, wcslen(compare));

		// Comparsion returns equal
		if (compareResult == 0) {
			wcscpy_s(outDevicePath, outDevicePathLen, devInterfaceDetailData->DevicePath);
			free(devInterfaceDetailData);
			return 1;
		}
		free(devInterfaceDetailData);
	}
	return 0;
}

/// Hid open
HANDLE DllExport hid_open( // Returns opened HID handle (it's file handle) or zero on failure
	_In_ LPWSTR path // Path to open (get using hid_get_path)
) {
	return CreateFile(path,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		0, //FILE_FLAG_OVERLAPPED,
		0);
}

/// Hid write buffer to hid file handle
DWORD DllExport hid_write( // Returns bytes written (zero on failure
	_In_ HANDLE hHid, // Handle to hid (get using hid_open)
	_In_reads_bytes_opt_(bufferSize) LPCVOID lpBuffer, // Write buffer pointer
	_In_ DWORD bufferSize // Write buffer size
) {
	DWORD bytesWritten = 0;
	if (!WriteFile(hHid, lpBuffer, bufferSize, &bytesWritten, NULL)) {
		return 0;
	}
	return bytesWritten;
}

/// Hid read buffer (synchronously)
///
/// To read hid device continuosly use hid_register_reader()
DWORD DllExport hid_read( // Returns Bytes read (zero on failure)
	_In_ HANDLE hHid, // Handle to hid (get using hid_open)
	_Out_writes_bytes_(nNumberOfBytesToRead) __out_data_source(FILE) LPVOID lpBuffer, // Read buffer pointer
	_In_ DWORD nNumberOfBytesToRead // Read buffer size
) {
	DWORD bytesRead = 0;
	if (!ReadFile(hHid, lpBuffer, nNumberOfBytesToRead, &bytesRead, NULL)) {
		return 0;
	}
	return bytesRead;
}

void _readerLoop() {
	// TODO: Do we have to instead reopen the handle on demand? Does it close in murky ways?
	using namespace std::chrono_literals;
	int bytes = 0;
	while (true) {
		BYTE buffer[1024];
		// This is pretty rudimentary, and bad way but normally 
		// you never kill this thread anyway
		if (_killReaderThread) {
			return;
		}
		bytes = hid_read(_readerHHid, buffer, 1024);
		if (!bytes) {
			std::this_thread::sleep_for(2s);
			continue;
		}
		if (_readerHwnd) {
			SendMessage(_readerHwnd, _readerMessageOffset, bytes, (LPARAM)&buffer[0]);
		}
	}
}

/// Register reader (replaces any existing reader)
/// 
/// Reads the data continuosly in own std::thread, SendMessage()s to your listener
bool DllExport hid_register_reader( // Returns true on success
	_In_ LPWSTR path, // Device path, get using hid_get_path()
	_In_ HWND readerHwnd, // listener WIN32 window handle
	_In_ int messageOffset // Message offset for your listener
) {
	_readerHwnd = readerHwnd;
	_readerMessageOffset = messageOffset;
	_killReaderThread = false;

	if (_readerThread == NULL) {
		_readerHHid = hid_open(path);
		if (_readerHHid) {
			_readerThread = new std::thread(_readerLoop);
			return TRUE;
		}
	}
	else {
		return TRUE;
	}
	return FALSE;
}

/// Hid unregister reader
void DllExport hid_unregister_reader()
{
	_readerHwnd = 0;
	_readerMessageOffset = 0;
	if (_readerHHid) {
		CloseHandle(_readerHHid); // TODO: Care to check result?
		_readerHHid = NULL;
	}
	_killReaderThread = true;
}

/// Close handle
BOOL DllExport hid_close(
	_In_ HANDLE hHid // Handle to hid (get using hid_open)
) {
	return CloseHandle(hHid);
}