
#pragma once
#include "SerialLib.h"
#include <Windows.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <stdlib.h>

using namespace SerialLibrary;

struct channelsData
{
	vector< vector<int> > outputData;
	vector<int> BlockNum;
};

/* Macros Definitions (Constants)*/
#define BAUD_RATE							921600					//Baud Rate for OpenBCI (115200bps)
#define EEG_CHANNELS_8						8						//Use 8 channels
#define EEG_CHANNELS_16						16						//Use 16 channels 
#define BUFFERSIZE							33						//Buffer Size 
#define SAMPLEBLOCKSIZE						125						//Sample Block Size  
#define THREE_DOLLAR_SIGN					3						//Count Three Dollar Dign
#define READ_BUFFER_SIZE					5000
#define SAMPLING_RATE						250						//Default Sampling Rate 
#define SAMPLING_RATE500					500					
#define SAMPLING_RATE1K						1000					
#define SAMPLING_RATE2K						2000					 
#define DATA_BLOCK_SIZE						255						//Data Block Size 
#define CONVERSION_FACTOR					0.0000000217			//Conversion Factor for Gain 
#define SIZE_OF_POSSIBLE_GAINS				7						//7 number for possible gains 
#define VALID_HANDSHAKE_TIME				2000					//Valid Handshake Time (Iterations)
#define CHANNEL_SETTINGS					9						//Total Number of Channel Settings 

/*Possible Gains*/
#define GAIN_1								1
#define GAIN_2								2
#define GAIN_4								4
#define GAIN_6								6
#define GAIN_8								8
#define GAIN_12								12

/*ENABLE-DISABLE CHANNEL SETTINGS ACCESS*/
#define ENABLE_CHANNEL_SETTINGS_ACCESS		('x')
#define DISABLE_CHANNEL_SETTINGS_ACCESS		('X')

#define DEFAULT_CHANNEL_GAIN				24						//Default Channel Gain
#define NUM_COM_PORTS						21						//Total Number of COM Ports 

// All Channel Commands (send to the device) for turning the channels on
#define CHANNEL_1_ON						('!')
#define CHANNEL_2_ON						('@')
#define CHANNEL_3_ON						('#')
#define CHANNEL_4_ON						('$')
#define CHANNEL_5_ON						('%')
#define CHANNEL_6_ON						('^')
#define CHANNEL_7_ON						('&')
#define CHANNEL_8_ON						('*')
#define CHANNEL_9_ON						('Q')
#define CHANNEL_10_ON						('W')
#define CHANNEL_11_ON						('E')
#define CHANNEL_12_ON						('R')
#define CHANNEL_13_ON						('T')
#define CHANNEL_14_ON						('Y')
#define CHANNEL_15_ON						('U')
#define CHANNEL_16_ON						('I')

/*Commmands to send to configurate sample*/
#define SAMPLE_RATE_2kHZ					('A')
#define SAMPLE_RATE_1kHZ					('S')
#define SAMPLE_RATE_500HZ					('F')
#define SAMPLE_RATE_250HZ					('G') 

/*INPUT_TYPE_SET (Select the ADC Channel input source)*/
#define ADSINPUT_NORMAL						('0')				// default
#define ADSINPUT_SHORTED					('1')
#define ADSINPUT_BIAS_MEAS					('2')
#define ADSINPUT_MVDD						('3')
#define ADSINPUT_TEMP						('4')
#define ADSINPUT_TESTSIG					('5')
#define ADSINPUT_BIAS_DRP					('6')
#define ADSINPUT_BIAS_DRN					('7')

/*BIAS_SET (Select to include the channel input in BIAS generation)*/
#define REMOVE_FORM_BIAS					('0')				//default
#define INCLUDE_IN_BIAS						('1')

/*SRB2_SET*/
#define DISCONNECT_INPUT_FROM_SRB2			('0')
#define CONNECT_INPUT_TO_SRB2				('1')				//default

/*SRB1_SET*/
#define DISCONNECT_ALL_N_INPUTS_FROM_SRB1	('0')				//default
#define CONNECT_ALL_N_INPUTS_TO_SRB1		('1')

using namespace std;

namespace BioAmpLibrary
{
	class BioAmpLib
	{
		public:
			//BioAmpLib();
			static string getBioAmpPortName();
			static HANDLE getDeviceHandle();
			static bool checkDeviceHandshake(HANDLE devPortHandle);
			static void streamingData(HANDLE devPortHandle);
			static bool resetBioAmp(HANDLE devPortHandle);
			static bool stopStreaming(HANDLE devPortHandle);
			static bool startStreaming();
			static bool setSampleRate(HANDLE devPortHandle, int samplingRate);
			static bool setChannelsConfiguration(HANDLE devPortHandle, int nChannels, int activeChannels, int gain);
			static bool setDefaultConfiguration(HANDLE devPortHandle);
			static bool synchronizeBioAmp();
			static bool startBioAmp(string COMport, int nChannels, int nActiveChannels, int sampleRate, int gain);
			static bool closeConnection();
			static bool readRawData(int nSamples, vector<BYTE>& rawDataBuffer);
			static bool readRawDataBySample(vector<BYTE>& rawDataBuffer);
			/*función para convertir los datos crudos en los datos por canal*/
			static struct channelsData getChannelsData(vector<BYTE>& rawDataBuffer, int nSamples);
			static vector<int> getChannelsDataBySample(vector<BYTE>& rawDataBuffer);

		private:
			static bool writeByte(HANDLE devPortHandle, BYTE byte); 
			static int channelsNumber;			/*8 channels for 1 bioAmp, 16 channels for 2 ...in the future not so far ;) */
			static HANDLE devicePortHandle;
			static string devicePortName;

	};

}




