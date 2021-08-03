////////////////////////////////////////////////////////////////////////////////
// Authors: Laboratorio de Ingeniería en Rehabilitación
//          e Investigaciones Neuromusculares y Sensoriales (LIRINS) - FIUNER
// Description: BioAmpADC header
////////////////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_BIOAMPADC_H  // makes sure this header is not included more than once
#define INCLUDED_BIOAMPADC_H

#include "BufferedADC.h"
#include "compat/Win32Defs.h"
#include "Clock.h"
#include "bioAmpLib.h"


class BioAmpADC : public BufferedADC
{
	public:
	BioAmpADC();
	~BioAmpADC();
	void OnPublish() override;
	void OnAutoConfig() override;
	void OnPreflight( SignalProperties& Output ) const override;
	void OnInitialize( const SignalProperties& Output ) override;
	void OnStartAcquisition() override;
	void DoAcquire( GenericSignal& Output ) override;
	void OnStopAcquisition() override;

	void OnTrigger( int );												// for asynchronous triggers only

	private:
	string portName;
	HANDLE mFileDescriptor;												//File Descriptor 

	/*Flags*/
	bool mOpenPort;														//Flag to check whether the port is open or not
	bool mSetConfig;													//Flag to indicate if user set the configuration or not
	bool mUse8Channels;													//Flag to check whether using 8 channel mode or 16 channel mode 
	/*Buffers*/
	vector<BYTE> rawDataBuffer;
	int mChannelGain[EEG_CHANNELS_8];									//Array to store the Gains for all 8 channels
	std::vector<int> mSourceChannelList;								//Vector to store the Source Channels in form of a List 
	std::vector<int> mChannelNames;										//Vector to store the Source Channels names in form of a List 
	std::vector<float> mSourceChannelGains;

	/**/
	int mSampleBlockSize;												//Records the sample block size
	int mSampleRate;
	int mNumberOfChannels;												//Stores the number of channels
	struct channelsData chData;
};

#endif // INCLUDED_BIOAMPADC_H
