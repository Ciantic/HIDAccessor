# HIDAccessor.dll

Windows 64bit DLL for accessing raw HID device (writing/reading)

MIT Licensed, see LICENSE.txt, created by Jari Pennanen, 2018.

The DLL is in the repository for convenience.

## Example usage

See example usage with AutoHotkey_L (unicode) [HIDExample.ahk](./HIDExample.ahk) which writes bytes to Ergodox Ez keyboard and reads bytes back. Also the tests project can be useful example.

## Exported functions

```c++
/// Get HID path
///
/// According to MSDN the device path does not change between computer restarts
BOOL DllExport hid_get_path( // Returns True on success
	_In_ unsigned int vid, // Vendor ID
	_In_ unsigned int pid, // Process ID
	_In_ int interfaceNumber,  // Interface number
	_Out_writes_z_(outDevicePathLen) wchar_t* outDevicePath, // Device path is written here
	_In_ rsize_t outDevicePathLen // Device path length as characters (not bytes!)
)

/// Hid open
HANDLE DllExport hid_open( // Returns opened HID handle (it's file handle) or zero on failure
	_In_ LPWSTR path // Path to open (get using hid_get_path)
)

/// Hid write buffer to hid file handle
DWORD DllExport hid_write( // Returns bytes written (zero on failure
	_In_ HANDLE hHid, // Handle to hid (get using hid_open)
	_In_reads_bytes_opt_(bufferSize) LPCVOID lpBuffer, // Write buffer pointer
	_In_ DWORD bufferSize // Write buffer size
)

/// Hid read buffer (synchronously)
///
/// To read buffer continuosly use hid_register_reader()
DWORD DllExport hid_read( // Returns Bytes read (zero on failure)
	_In_ HANDLE hHid, // Handle to hid (get using hid_open)
	_Out_writes_bytes_(nNumberOfBytesToRead) __out_data_source(FILE) LPVOID lpBuffer, // Read buffer pointer
	_In_ DWORD nNumberOfBytesToRead // Read buffer size
)

/// Hid register reader (replaces any existing reader)
/// 
/// Reads the data continuosly in own std::thread, SendMessage()s to your listener
bool DllExport hid_register_reader( // Returns true on success
	_In_ LPWSTR path, // Device path, get using hid_get_path()
	_In_ HWND readerHwnd, // listener WIN32 window handle
	_In_ int messageOffset // Message offset for your listener
)

/// Hid unregister reader
void DllExport hid_unregister_reader()

/// Close handle
BOOL DllExport hid_close(
	_In_ HANDLE hHid // Handle to hid (get using hid_open)
)

```