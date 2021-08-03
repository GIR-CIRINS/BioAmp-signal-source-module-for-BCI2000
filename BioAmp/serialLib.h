#pragma once
//Code adapted from Matteo Chiesi - University of Bologna*/
// SerialLib.h

#include <Windows.h>
#include <string>
#include <vector>

using namespace std;



namespace SerialLibrary
{
	class SerialLib
	{
		public:

			/*
			* Class:    SerialLibrary
			* Method:   getSerialPortNames
			*/
			static std::vector<std::string> getSerialPortNames();

			/*
			* Class:	SerialLibrary
			* Method:	openPort
			*/

			static HANDLE openPort(string);

			/*
			* Class:     SerialLibrary
			* Method:    setParams
			*/

			static bool setParams(HANDLE, int, int, int, int, bool, bool, int);

			/*
			* Class:     SerialLibrary
			* Method:    purgePort
			*/

			static bool  purgePort(HANDLE, int);

			/*
			* Class:     SerialLibrary
			* Method:    closePort
			*/

			static bool  closePort(HANDLE);

			/*
			* Class:     SerialLibrary
			* Method:    setEventsMask
			*/
			static bool setEventsMask(HANDLE, int);

			/*
			* Class:     SerialLibrary
			* Method:    getEventsMask
			*/
			static int getEventsMask(HANDLE);

			/*
			* Class:     SerialLibrary
			* Method:    readBytes
			*/
			static void readBytes(HANDLE, vector<BYTE>&, int);

			/*
			* Class:     SerialLibrary
			* Method:    writeBytes
			*/
			static bool writeBytes(HANDLE, vector<BYTE>&, int);

			/*
			* Class:     SerialLibrary
			* Method:    getBuffersBytesCount
			*/
			static vector<int> getBuffersBytesCount(HANDLE);
				
	};
}