#include "bioAmpLib.h"
#include "BCIStream.h"
#include <fstream>

namespace BioAmpLibrary
{
	

	string BioAmpLib::getBioAmpPortName()
	{
		
		vector<string> portNames;
		portNames = SerialLib::getSerialPortNames();

		if (portNames.size() == 0)
		{
			bciout << "PortNames not found" << endl;
		}
		else
		{
			for (int i = 0; i < portNames.size(); i++)
			{
				devicePortHandle = SerialLib::openPort(portNames[i]);

				if (devicePortHandle == INVALID_HANDLE_VALUE)
				{					
					bciout << "PORT " << portNames[i] << " NO Connection!!!" << endl;
					continue;
				}
				else
				{
					bciout << "PORT " << portNames[i] << " Connected Successfully!!" << endl;
				}

				if (!SerialLib::setParams(devicePortHandle, BAUD_RATE, 8, ONESTOPBIT, NOPARITY, RTS_CONTROL_DISABLE, DTR_CONTROL_DISABLE, 0))
				{
					bciout << "Error during configuration" << endl;
				}
				Sleep(100);

				/*If a thread uses PurgeComm to flush an output buffer, the deleted characters are not transmitted.
					To empty the output buffer while ensuring that the contents are transmitted,
					call the FlushFileBuffers function (a synchronous operation).
					Note, however, that FlushFileBuffers is subject to flow control but not to write time-outs,
					and it will not return until all pending write operations have been transmitted.*/

				if (!SerialLib::purgePort(devicePortHandle, PURGE_RXCLEAR | PURGE_TXCLEAR))
				{
					
					bciout << "Error while purging the port" << endl;
				}
				Sleep(100);

				//Handshake
				if (!checkDeviceHandshake(devicePortHandle))
				{
					bciout << "NO Handshake" << endl;
				}
				else
				{					
					bciout << "Handshake done!!" << endl;
					devicePortName = portNames[i];
				}


			}
		}

		if (!SerialLib::closePort(devicePortHandle))
		{
			bciout << "Warning!! Couldn't close the port" << endl;
		}

		return devicePortName;
	}

	HANDLE BioAmpLib::getDeviceHandle()
	{
		return devicePortHandle;
	}

	bool BioAmpLib::checkDeviceHandshake(HANDLE devPortHandle)
	{
		bool succesfullHandshake = false;
		stopStreaming(devPortHandle);
		resetBioAmp(devPortHandle);

		vector<int>bytesAvailable(2); //input and output bytes available

		while (bytesAvailable[0] == 0)
		{
			bytesAvailable = SerialLib::getBuffersBytesCount(devPortHandle);
			Sleep(2);
		}

		vector<BYTE> handshakeBuffer(bytesAvailable[0]);

		SerialLib::readBytes(devPortHandle, handshakeBuffer, bytesAvailable[0]);

		string handshakeWord(handshakeBuffer.begin(), handshakeBuffer.end());
		if (handshakeWord.compare("$$$"))
		{
			succesfullHandshake = true;
		}
		else
		{
			bciout << "error during Handshake" << endl;
		}

		return succesfullHandshake;
	}

	bool BioAmpLib::writeByte(HANDLE devPortHandle, BYTE byte)
	{
		vector<BYTE> buffer;
		buffer.push_back(byte);
		bool byteWritten = false;

		byteWritten = SerialLib::writeBytes(devPortHandle, buffer, buffer.size());

		if (!byteWritten)
		{
			bciout << "error writting byte" << endl;
		}

		return byteWritten;
	}

	void BioAmpLib::streamingData(HANDLE devPortHandle)
	{
	}

	bool BioAmpLib::resetBioAmp(HANDLE devPortHandle)
	{
		bool charWritten = false;

		if (writeByte(devPortHandle, 'v'))
		{
			charWritten = true;
			//bciout << "Streaming Stopped" << endl;
			bciout << "BioAmp reset" << endl;
		}
		else
		{
			bciout << "error during reset" << endl;
		}

		return charWritten;
	}

	bool BioAmpLib::stopStreaming(HANDLE devPortHandle)
	{
		bool charWritten = false;

		if (writeByte(devPortHandle, 's'))
		{
			charWritten = true;
			//bciout << "Streaming Stopped" << endl;
			bciout << "Streaming Stopped" << endl;
		}
		else
		{
			bciout << "error stoping streaming" << endl;
		}

		return charWritten;
	}

	bool BioAmpLib::startStreaming()
	{
		bool charWritten = false;

		if (writeByte(devicePortHandle, 'b'))
		{
			charWritten = true;
			
			bciout << "Streaming Started" << endl;
		}
		else
		{
			bciout << "error starting streaming" << endl;
		}

		return charWritten;
	}

	bool BioAmpLib::setSampleRate(HANDLE devPortHandle, int samplingRate)
	{
		vector<BYTE> buffer;
		bool charWritten = false;

		switch (samplingRate)
		{
		case 250:
			buffer.push_back(SAMPLE_RATE_250HZ);
			break;
		case 500:
			buffer.push_back(SAMPLE_RATE_500HZ);
			break;
		case 1000:
			buffer.push_back(SAMPLE_RATE_1kHZ);
			break;
		case 2000:
			buffer.push_back(SAMPLE_RATE_2kHZ);
			break;
		}

		charWritten = SerialLib::writeBytes(devPortHandle, buffer, buffer.size());

		if (charWritten)
		{			
			bciout << "BioAmp sample rate set to: " << samplingRate << endl;
		}
		else
		{
			bciout << "error during setting sample rate" << endl;
		}

		return charWritten;
	}

	bool BioAmpLib::setChannelsConfiguration(HANDLE devPortHandle, int nChannels, int activeChannels, int gain)
	{
		char* buffer;
		int gain_factor = 6;
		channelsNumber = nChannels;
		
		//turn on and configurate channels
		for (int channel = 0; channel < activeChannels; channel++)
		{
			writeByte(devPortHandle, ENABLE_CHANNEL_SETTINGS_ACCESS); /*'x' to begin configuration*/
			Sleep(200);
			string nChannel = to_string(channel + 1);
			buffer = new char[nChannel.length() + 1];
			strcpy_s(buffer, sizeof(buffer), nChannel.c_str());

			writeByte(devPortHandle, *buffer);
			Sleep(200);
			writeByte(devPortHandle, '0');
			Sleep(200);
			
			switch (gain)
			{
			case GAIN_1:
				gain_factor = 0;
				break;
			case GAIN_2:
				gain_factor = 1;
				break;
			case GAIN_4:
				gain_factor = 2;
				break;
			case GAIN_6:
				gain_factor = 3;
				break;
			case GAIN_8:
				gain_factor = 4;
				break;
			case GAIN_12:
				gain_factor = 5;
				break;
			case DEFAULT_CHANNEL_GAIN:
				gain_factor = 6;
				break;
			}

			string channelGain = to_string(gain_factor);
			buffer = new char[channelGain.length() + 1];
			strcpy_s(buffer, sizeof(buffer), channelGain.c_str());

			writeByte(devPortHandle, *buffer);
			Sleep(200);
			writeByte(devPortHandle, ADSINPUT_NORMAL); //input type ADSINPUT_NORMAL (default)
			Sleep(200);
			writeByte(devPortHandle, INCLUDE_IN_BIAS); //bias set
			Sleep(200);
			writeByte(devPortHandle, CONNECT_INPUT_TO_SRB2); //srb2 set
			Sleep(200);
			writeByte(devPortHandle, DISCONNECT_ALL_N_INPUTS_FROM_SRB1); //srb1 set
			Sleep(200);
			writeByte(devPortHandle, DISABLE_CHANNEL_SETTINGS_ACCESS); // 'X' to Disable Channel Setting
			Sleep(200);

			bciout << "Setting for Channel " << channel + 1 << " out of " << channelsNumber << " has been completed." << endl;
			
		}

		/*Uncomment to enable test signal*/
		//writeByte(devPortHandle, '-'); //+-1.875mV, 1Hz

		for (int ch = activeChannels; ch < nChannels; ch++)
		{
			string channelNumber = to_string(ch + 1);
			buffer = new char[channelNumber.length() + 1];
			strcpy_s(buffer, sizeof(buffer), channelNumber.c_str());

			writeByte(devPortHandle, *buffer);
			Sleep(200);
			bciout << "shutting down Channel " << ch + 1 << " of " << nChannels << endl;
			
		}

		return true;

	}

	bool BioAmpLib::setDefaultConfiguration(HANDLE devPortHandle)
	{		
		bool charWritten = false;

		if (writeByte(devPortHandle, 'd'))
		{
			charWritten = true;
			
			bciout << "Set default configuration" << endl;
		}
		else
		{
			bciout << "error in default config" << endl;
		}

		return charWritten;
	}

	bool BioAmpLib::synchronizeBioAmp()
	{
		bool synchronized = false;
		vector<BYTE> dummyBuffer;
		int timeout = 0;

		SerialLibrary::SerialLib::purgePort(devicePortHandle, PURGE_RXCLEAR | PURGE_TXCLEAR);

		while (!synchronized && (timeout < 500))
		{
			SerialLibrary::SerialLib::readBytes(devicePortHandle, dummyBuffer, 1);
			if (dummyBuffer[0] == 0xA0)
			{
				SerialLibrary::SerialLib::readBytes(devicePortHandle, dummyBuffer, 32);
				if (dummyBuffer[31] == 0xC0) {
					synchronized = true;
				}
			}
			Sleep(1);
			++timeout;
		}
		return synchronized;
	}

	bool BioAmpLib::startBioAmp(string COMport, int nChannels, int nActiveChannels, int sampleRate, int gain)
	{
		bool error = false;
		devicePortHandle = SerialLib::openPort(COMport);
		if (!setDefaultConfiguration(devicePortHandle))
		{
			error = true;
		}
		if (!setChannelsConfiguration(devicePortHandle, nChannels, nActiveChannels, gain))
		{
			error = true;
		}
		if (!setSampleRate(devicePortHandle, sampleRate))
		{
			error = true;
		}

		return error;
	}

	bool BioAmpLib::closeConnection()
	{
		bool error = false;
		if (!SerialLib::closePort(devicePortHandle))
		{
			bciout << "couldn't close connection" << endl;
			error = true;
		}
		return error;
	}

	bool BioAmpLib::readRawData(int nSamples, vector<BYTE>& rawDataBuffer)
	{
		bool no_error = true;
		int bufferSize = BUFFERSIZE * nSamples;
		rawDataBuffer.resize(bufferSize);
		vector<int> availableBytes(2);
		int timeout = 0;

		while ((availableBytes[0] < bufferSize) && (timeout < 500))
		{
			availableBytes = SerialLibrary::SerialLib::getBuffersBytesCount(devicePortHandle);
			Sleep(10);
			++timeout;
		}

		if (timeout == 500)
		{
			bciout << "Timeout error during reading" << endl;
			no_error = false;
			return no_error;
		}

		SerialLibrary::SerialLib::readBytes(devicePortHandle, rawDataBuffer, bufferSize);

		return no_error;
	}

	bool BioAmpLib::readRawDataBySample(vector<BYTE>& rawDataBuffer)
	{
		bool no_error = true;
		int bufferSize = BUFFERSIZE;
		rawDataBuffer.resize(bufferSize);
		vector<int> availableBytes(2);
		int timeout = 0;

		while ((availableBytes[0] < bufferSize) && (timeout < 500))
		{
			availableBytes = SerialLibrary::SerialLib::getBuffersBytesCount(devicePortHandle);
			Sleep(10);
			++timeout;
		}

		if (timeout == 500)
		{
			bciout << "Timeout error during reading" << endl;
			no_error = false;
			return no_error;
		}

		SerialLibrary::SerialLib::readBytes(devicePortHandle, rawDataBuffer, bufferSize);

		return no_error;
	}


	struct channelsData BioAmpLib::getChannelsData(vector<BYTE>& rawDataBuffer, int nSamples)
	{
		struct channelsData convertedData;
		convertedData.outputData.resize(channelsNumber, vector<int>(nSamples));
		convertedData.BlockNum.resize(nSamples);
		int ByteCount = 0;
		int auxiliary_sample = 0;
		int channelNumber = 0;

		for (int sample = 0; sample < nSamples; sample++)
		{
			if ((rawDataBuffer[BUFFERSIZE * sample] != 0xA0) || (rawDataBuffer[(BUFFERSIZE * sample) + (BUFFERSIZE - 1)] != 0xC0))
			{
				bciout << "Error in synchronization" << endl;
			}
			//store sample number
			convertedData.BlockNum[sample] = int(rawDataBuffer[1 + (BUFFERSIZE * sample)]);

			//Store data in to mPacket (i.e. Storing the Byte3 to Byte 26 = 24 Bytes for all 8 channels: (24bit / 3 Bytes) for each channel)
			for (int index = 2; index < 26; index++)
			{
				auxiliary_sample |= ((0xFF & rawDataBuffer[index + (BUFFERSIZE * sample)]) << (16 - (ByteCount * 8)));

				ByteCount++;

				if (ByteCount == 3)
				{
					//Check if the integer is negative
					if ((auxiliary_sample & 0x00800000) != 0)
					{
						auxiliary_sample |= 0xFF000000;
					}
					else
					{
						auxiliary_sample &= 0x00FFFFFF;
					}

					convertedData.outputData[channelNumber][sample] = auxiliary_sample;

					channelNumber++;
					ByteCount = 0;
					auxiliary_sample = 0;
				}
			}

			channelNumber = 0;
		}


		return convertedData;
	}

	vector<int> BioAmpLib::getChannelsDataBySample(vector<BYTE>& rawDataBuffer)
	{
		vector<int> channelsData;
		channelsData.resize(channelsNumber);
		int ByteCount = 0;
		int auxiliary_sample = 0;
		int channelNumber = 0;


		if ((rawDataBuffer[0] != 0xA0) || (rawDataBuffer[(BUFFERSIZE - 1)] != 0xC0))
		{
			bciout << "Error in synchronization" << endl;
		}
		//store sample number	

		//Store data in to mPacket (i.e. Storing the Byte3 to Byte 26 = 24 Bytes for all 8 channels: (24bit / 3 Bytes) for each channel)
		for (int index = 2; index < 26; index++)
		{
			auxiliary_sample |= ((0xFF & rawDataBuffer[index]) << (16 - (ByteCount * 8)));

			ByteCount++;

			if (ByteCount == 3)
			{
				//Check if the integer is negative
				if ((auxiliary_sample & 0x00800000) != 0)
				{
					auxiliary_sample |= 0xFF000000;
				}
				else
				{
					auxiliary_sample &= 0x00FFFFFF;
				}

				channelsData[channelNumber] = auxiliary_sample;

				channelNumber++;
				ByteCount = 0;
				auxiliary_sample = 0;
			}
		}

		channelNumber = 0;

		return channelsData;
	}
}
