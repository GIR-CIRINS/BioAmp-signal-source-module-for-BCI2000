#include "SerialLib.h"
#include <vector>


namespace SerialLibrary
{

	/*
	* Get serial port names
	*/

	vector<string> SerialLib::getSerialPortNames()
	{

		HKEY phkResult;
		LPCWSTR lpSubKey = (LPCWSTR)"HARDWARE\\DEVICEMAP\\SERIALCOMM\\";
		vector<string>  returnArray;

		if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, (LPCSTR)lpSubKey, 0, KEY_READ, &phkResult) == ERROR_SUCCESS) {
			bool hasMoreElements = true;
			DWORD keysCount = 0;
			char valueName[256];
			DWORD valueNameSize;
			DWORD enumResult;
			while (hasMoreElements) {
				valueNameSize = 256;
				enumResult = RegEnumValueA(phkResult, keysCount, valueName, &valueNameSize, NULL, NULL, NULL, NULL);
				if (enumResult == ERROR_SUCCESS) {
					keysCount++;
				}
				else if (enumResult == ERROR_NO_MORE_ITEMS) {
					hasMoreElements = false;
				}
				else {
					hasMoreElements = false;
				}
			}
			if (keysCount > 0) {
				returnArray.resize(keysCount);

				char lpValueName[256];
				DWORD lpcchValueName;
				BYTE lpData[256];
				DWORD lpcbData;
				DWORD result;
				for (DWORD i = 0; i < keysCount; i++) {
					lpcchValueName = 256;
					lpcbData = 256;
					result = RegEnumValueA(phkResult, i, lpValueName, &lpcchValueName, NULL, NULL, lpData, &lpcbData);
					if (result == ERROR_SUCCESS) {
						returnArray[i].append((char*)lpData);
					}
				}
			}
			CloseHandle(phkResult);
		}
		return returnArray;
	}


	/*
	* Port opening.
	*
	* In 2.2.0 added useTIOCEXCL (not used in Windows, only for compatibility with _nix version)
	*/
	HANDLE SerialLib::openPort(string portName) 
	{

		HANDLE hComm = NULL;
		string prefix = "\\\\.\\";
		string portFullName = prefix + portName;

		char* CopyPortName = new char[portFullName.size() + 1];
		strcpy_s(CopyPortName, (unsigned int)(portFullName.size() + 1), portFullName.c_str());

		//string stemp = string(portFullName.begin(), portFullName.end());
		//LPCSTR swportFullName = stemp.c_str();

		hComm = CreateFile((LPCSTR)CopyPortName,
			GENERIC_READ | GENERIC_WRITE, //access ( read and write)
			0, //(share) 0:cannot share the COM port
			NULL, //security  (None) 
			OPEN_EXISTING, // creation : open_existing
			FILE_ATTRIBUTE_NORMAL,  
			NULL); // no templates file for COM port...

				//since 2.3.0 ->
		if (hComm != INVALID_HANDLE_VALUE) 
		{
			DCB* dcb = new DCB();
			if (!GetCommState(hComm, dcb)) 
			{
				CloseHandle(hComm);//since 2.7.0
				hComm = (HANDLE)ERR_INCORRECT_SERIAL_PORT;//(-4)Incorrect serial port
			}
			delete dcb;
		}
		else 
		{
			DWORD errorValue = GetLastError();
			if (errorValue == ERROR_ACCESS_DENIED) {
				hComm = (HANDLE)ERR_PORT_BUSY;//(-1)Port busy
			}
			else if (errorValue == ERROR_FILE_NOT_FOUND) {
				hComm = (HANDLE)ERR_PORT_NOT_FOUND;//(-2)Port not found
			}
		}
		//<- since 2.3.0
		return hComm;//since 2.4.0 changed to jlong
	};


	/*
	* Setting serial port params.
	*
	* In 2.6.0 added flags (not used in Windows, only for compatibility with _nix version)
	*/
	bool SerialLib::setParams(HANDLE portHandle, int baudRate, int byteSize, int stopBits, int parity, bool setRTS, bool setDTR, int flags) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		DCB* dcb = new DCB();
		bool returnValue = false;
		if (GetCommState(hComm, dcb)) 
		{
			dcb->BaudRate = baudRate;
			dcb->ByteSize = byteSize;
			dcb->StopBits = stopBits;
			dcb->Parity = parity;

			//since 0.8 ->
			if (setRTS == true) 
			{
				dcb->fRtsControl = RTS_CONTROL_ENABLE;
			}
			else 
			{
				dcb->fRtsControl = RTS_CONTROL_DISABLE;
			}
			if (setDTR == false) 
			{
				dcb->fDtrControl = DTR_CONTROL_DISABLE;
			}
			else 
			{				
				dcb->fDtrControl = DTR_CONTROL_ENABLE;
			}
			dcb->fOutxCtsFlow = FALSE;
			dcb->fOutxDsrFlow = FALSE;
			dcb->fDsrSensitivity = FALSE;
			dcb->fTXContinueOnXoff = TRUE;
			dcb->fOutX = FALSE;
			dcb->fInX = FALSE;
			dcb->fErrorChar = FALSE;
			dcb->fNull = FALSE;
			dcb->fAbortOnError = FALSE;
			dcb->XonLim = 2048;
			dcb->XoffLim = 512;
			dcb->XonChar = (char)17; //DC1
			dcb->XoffChar = (char)19; //DC3
									  //<- since 0.8

			if (SetCommState(hComm, dcb)) 
			{

				//since 2.1.0 -> previously setted timeouts by another application should be cleared
				COMMTIMEOUTS* lpCommTimeouts = new COMMTIMEOUTS();
				lpCommTimeouts->ReadIntervalTimeout = 0;
				lpCommTimeouts->ReadTotalTimeoutConstant = 0;
				lpCommTimeouts->ReadTotalTimeoutMultiplier = 0;
				lpCommTimeouts->WriteTotalTimeoutConstant = 0;
				lpCommTimeouts->WriteTotalTimeoutMultiplier = 0;
				if (SetCommTimeouts(hComm, lpCommTimeouts)) {
					returnValue = true;
				}
				delete lpCommTimeouts;
				//<- since 2.1.0
			}
		}
		delete dcb;
		return returnValue;
	}

	/*
	* PurgeComm
	*/
	bool SerialLib::purgePort(HANDLE portHandle, int flags) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		DWORD dwFlags = (DWORD)flags;
		return (PurgeComm(hComm, dwFlags) ? true : false);
	}

	/*
	* Port closing
	*/
	bool  SerialLib::closePort(HANDLE portHandle) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		return (CloseHandle(hComm) ? true : false);
	}

	/*
	* Set events mask
	*/
	bool SerialLib::setEventsMask(HANDLE portHandle, int mask) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		DWORD dwEvtMask = (DWORD)mask;
		return (SetCommMask(hComm, dwEvtMask) ? true : false);
	}



	/*
	* Get events mask
	*/
	int  SerialLib::getEventsMask(HANDLE portHandle) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		DWORD lpEvtMask;
		if (GetCommMask(hComm, &lpEvtMask)) 
		{
			return (int)lpEvtMask;
		}
		else 
		{
			return -1;
		}
	}

	/*
	* Read data from port
	* portHandle - port handle
	* byteCount - count of bytes for reading
	*/
	void SerialLib::readBytes(HANDLE portHandle, vector<unsigned char> &lpBuffer, int byteCount)
	{

		HANDLE hComm = (HANDLE)portHandle;
		DWORD lpNumberOfBytesTransferred;
		DWORD lpNumberOfBytesRead;
		OVERLAPPED* overlapped = new OVERLAPPED();
		BOOL fWaitingOnRead = FALSE;
		lpBuffer.resize(byteCount);
		//byte* lpBuffer2 = (byte*)malloc(byteCount*sizeof(byte));
		overlapped->hEvent = CreateEventA(NULL, true, false, NULL);
		fWaitingOnRead = ReadFile(hComm, &lpBuffer[0], (DWORD)byteCount, &lpNumberOfBytesRead, overlapped);
		if (!fWaitingOnRead)
			if (GetLastError() == ERROR_IO_PENDING) {
				if (WaitForSingleObject(overlapped->hEvent, INFINITE) == WAIT_OBJECT_0) { //wait until the object is signaled -> This does not mean that it was completed successfully, just that it was completed.
					if (GetOverlappedResult(hComm, overlapped, &lpNumberOfBytesTransferred, false)) {
						fWaitingOnRead = true;
					}
				}
			}
		CloseHandle(overlapped->hEvent);
		delete overlapped;
	}

	/*
	* Write data to port
	* portHandle - port handle
	* buffer - byte array for sending
	*/
	bool  SerialLib::writeBytes(HANDLE portHandle, vector<BYTE>& buffer, int lenght) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		DWORD lpNumberOfBytesTransferred;
		DWORD lpNumberOfBytesWritten;
		OVERLAPPED* overlapped = new OVERLAPPED();
		bool returnValue = false;
		overlapped->hEvent = CreateEventA(NULL, true, false, NULL);
		if (WriteFile(hComm, &buffer[0], (DWORD)lenght, &lpNumberOfBytesWritten, overlapped)) 
		{
			returnValue = true;
		}
		else if (GetLastError() == ERROR_IO_PENDING) 
		{
			if (WaitForSingleObject(overlapped->hEvent, INFINITE) == WAIT_OBJECT_0) 
			{
				if (GetOverlappedResult(hComm, overlapped, &lpNumberOfBytesTransferred, false)) 
				{
					returnValue = true;
				}
			}
		}
		CloseHandle(overlapped->hEvent);
		delete overlapped;
		return returnValue;
	}

	/*
	* Get bytes count in serial port buffers (Input and Output)
	*/
	vector<int> SerialLib::getBuffersBytesCount(HANDLE portHandle) 
	{
		HANDLE hComm = (HANDLE)portHandle;
		vector<int> returnValues(2);
		returnValues[0] = -1;
		returnValues[1] = -1;
		DWORD lpErrors;
		COMSTAT* comstat = new COMSTAT();
		if (ClearCommError(hComm, &lpErrors, comstat)) 
		{
			returnValues[0] = (int)comstat->cbInQue;
			returnValues[1] = (int)comstat->cbOutQue;
		}
		else 
		{
			returnValues[0] = -1;
			returnValues[1] = -1;
		}
		delete comstat;
		return returnValues;
	}

}