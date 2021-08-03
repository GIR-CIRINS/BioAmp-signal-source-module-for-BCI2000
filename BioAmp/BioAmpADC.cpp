
#include "BioAmpADC.h"
#include "BCIStream.h"
#include "BCIEvent.h"
#include "ThreadUtils.h"

using namespace std;

#define AMP_API_INVALID_DEVICE_HANDLE (0)
//Initializing static members from BioAmpLib
int BioAmpLibrary::BioAmpLib::channelsNumber{ 8 };
HANDLE BioAmpLibrary::BioAmpLib::devicePortHandle{ AMP_API_INVALID_DEVICE_HANDLE };
string BioAmpLibrary::BioAmpLib::devicePortName{ "" };

// Make the source filter known to the framework.
RegisterFilter( BioAmpADC, 1 ); // ADC filters must be registered at location "1" in order to work.

BioAmpADC::BioAmpADC()
: mFileDescriptor(AMP_API_INVALID_DEVICE_HANDLE), // Each plain data member should appear in the initializer list.
  mOpenPort(false),
  mNumberOfChannels(EEG_CHANNELS_8),
  mSampleBlockSize(SAMPLEBLOCKSIZE),
  mSetConfig(false),
  mUse8Channels(true)
{   //Initializing the gains for all 8/16 channels with value = 1
    for (int channel = 0; channel < mNumberOfChannels; channel++)
    {
        mChannelGain[channel] = 1;
    }
}

BioAmpADC::~BioAmpADC()
{
  // The destructor is where to make sure that all acquired resources are released.
  // Memory deallocation (calling delete[] on a NULL pointer is OK).
   
}

void
BioAmpADC::OnPublish()
{
  // Declare any parameters that the filter needs....

 BEGIN_PARAMETER_DEFINITIONS

    "Source:Signal%20Properties string PortName= auto "
     "// This software can automatically detect connection",

    "Source:Signal%20Properties int SourceCh= 8 %"
       "// number of digitized and stored channels",

    "Source:Signal%20Properties int SampleBlockSize= 8 "
       "// number of samples transmitted at a time",

    "Source:Signal%20Properties float SamplingRate= 250 %"
       "// sample rate",

    "Source:Signal%20Properties list SourceChGain= 1 auto "
       " // physical units per raw A/D unit",

    "Source:Signal%20Properties list SourceChOffset= 1 auto "
       " // raw A/D offset to subtract, typically 0",

    "Source:Signal%20Properties list ChannelNames= 1 auto "
       " // names of amplifier channels",

    "Source:Signal%20Properties int MyAmpSetting= 3 "
       " // some user setting specific to your amp",

     "Source:Signal%20Properties list ChannelsGain= 1 auto 24 1"
     " // The channel gain can be 1 2 4 6 8 12 24(Default)",

     "Source:Signal%20Properties list SourceChList= 1 auto "
     " // The default source channel list is 1 2 3 4 5 6 7 8",

 END_PARAMETER_DEFINITIONS

  // ...and likewise any state variables.

  // IMPORTANT NOTE ABOUT BUFFEREDADC AND STATES:
  // * BCI2000 States, or "state variables", are additional data channels stored alongside signal data,
  //   with a resolution of one value per source signal sample.
  // * A State is much like a source signal channel, but differs from a source signal channel in the
  //   following respects:
  //   + You choose the number of bits used to represent the State's value, up to 64.
  //   + Unlike source signals, States are transmitted through all stages of processing, and their values
  //     may be modified during processing, allowing all parts of the system to store state information in
  //     data files.
  //   + When loading a BCI2000 data file for analysis, States appear separately, with names, which is
  //     typically more useful for trigger information than being just another channel in the signal.
  //   + States may be set synchronously from inside a filter's Process() handler, or asynchronously using
  //     a "bcievent" interface.
  //   + If your amplifier provides a digital input, or another kind of "trigger" information, it makes sense
  //     to store this information in form of one or more States. From a user perspective, it is probably most
  //     useful if physically distinct amplifier input sockets correspond to States, and single bits to certain
  //     lines or values communicated through such a socket.
  //   + If the amplifier API asynchronously reports trigger information through a callback mechanism, you
  //     should register a callback that uses the "bcievent" interface to set states asynchronously.
  //     This example provides a "MyAsyncTriggers" "event state", and a sample callback function.
  //   + If the amplifier API sends trigger information in the same way as it does for signal data, you should
  //     use a "State channel" to represent it in your source module. This example provides a "MySyncTriggers"
  //     state, and writes to it when acquiring data.

  BEGIN_STREAM_DEFINITIONS
    "BioAmpADCSyncTriggers 8 0", // <name> <bit width> <initial value>
  END_STREAM_DEFINITIONS

  BEGIN_EVENT_DEFINITIONS
    "BioAmpADCAsyncTriggers 8 0", // <name> <bit width> <initial value>
  END_EVENT_DEFINITIONS
}

// For asynchronous trigger notification, register this callback with the amplifier API.
static void
TriggerCallback( void* pData, int trigger )
{
  reinterpret_cast<BioAmpADC*>( pData )->OnTrigger( trigger );
}

void
BioAmpADC::OnTrigger( int trigger )
{
  // The following line will create a time-stamped entry into an event queue.
  // Once the next block of data arrives, the queued trigger value will be applied
  // to the BioAmpADCAsyncTriggers state variable at the sample location that
  // corresponds to the time stamp.
  bcievent << "BioAmpADCAsyncTriggers " << trigger;
}
/**********************************************************************************************************************************************************************************/
void
BioAmpADC::OnAutoConfig()
{
    // The user has pressed "Set Config" and some parameters may be set to "auto",
    // indicating that they should be set from the current amplifier configuration.
    // In this handler, we behave as if any parameter were set to "auto", and the
    // framework will transparently make sure to preserve user-defined values.

    // Device availability (or connection parameters) may have changed, so close
    // and reopen the connection to the device before proceeding.	
    if (mOpenPort)
    {
        BioAmpLibrary::BioAmpLib::closeConnection();
    }
    if (!mOpenPort)
    {
        if (Parameter("PortName") != "auto")
        {
            portName = Parameter("PortName");
        }
        else
        {
            portName = BioAmpLibrary::BioAmpLib::getBioAmpPortName();
            if (portName != "")
            {
                mOpenPort = true;
            }
            mFileDescriptor = BioAmpLibrary::BioAmpLib::getDeviceHandle();
        }
    }
    if (mOpenPort == false)
    {   /*Enters here when connection is not possible*/
        bciout << "DEVICE NOT AVAILABLE" << endl;
        return;
    }
    
    //********************************************************************************************
    //**************If Parameter SourceCh is not explicitly specified byt the user****************
    //********************************************************************************************
    if (Parameter("SourceCh") == "auto") 
    {
        /*SourceCh SET automatically to 8 Channels*/
        mNumberOfChannels = 8;
    }
    else 
    {
        /*SourceCh SET to the number of channels manually specified in the Source Parameters*/
        mNumberOfChannels = ActualParameter("SourceCh");
    }

    Parameter("SourceCh")->SetNumValues(mNumberOfChannels);

    //********************************************************************************************
    //************** if the channels list is auto Number of channels from 1 to 8******************
    //********************************************************************************************
    if (Parameter("SourceChList") == "auto") 
    {
        Parameter("SourceChList")->SetNumValues(mNumberOfChannels);
        for (int ch = 0; ch < mNumberOfChannels; ch++) 
        {
            Parameter("SourceChList")(ch) = ch + 1;
        }
    }
    else if (Parameter("SourceChList") != "auto") 
    {
        /*SourceChList is SET manually, for setting it automatically, Please set the value to <auto>*/
        if (Parameter("SourceChList")->NumValues() != mNumberOfChannels)
        {
            bciout << "Channel Names should have " << mNumberOfChannels << " entries." << endl;
        }        
    }

    for (int ch = 0; ch < mNumberOfChannels; ch++) 
    {
        mSourceChannelList.push_back(Parameter("SourceChList")(ch) - 1);
    }

    //*******************************************************************************************
    //************************************* Set Sampling Rate ***********************************
    //*******************************************************************************************
    if (Parameter("SamplingRate") == "auto") 
    {
        Parameter("SamplingRate") = SAMPLING_RATE;                                          /*set the Sampling Rate automatically*/
    }
    else if (Parameter("SamplingRate") != "auto") 
    {
        mSampleRate = ActualParameter("SamplingRate").ToNumber();
        switch (mSampleRate)
        {
            case SAMPLING_RATE:
                Parameter("SamplingRate") = SAMPLING_RATE;
                break;
            case SAMPLING_RATE500:
                Parameter("SamplingRate") = SAMPLING_RATE500;
                break;
            case SAMPLING_RATE1K:
                Parameter("SamplingRate") = SAMPLING_RATE1K;
                break;
            case SAMPLING_RATE2K:
                Parameter("SamplingRate") = SAMPLING_RATE2K;
                break;
        }
    }

    //*******************************************************************************************
    //****************************** Set sample block size **************************************
    //*******************************************************************************************
    if (Parameter("SampleBlockSize") != "auto")
    {
        mSampleBlockSize = ActualParameter("SampleBlockSize").ToNumber();
        Parameter("SampleBlockSize") = mSampleBlockSize;
    }
    else 
    {
        mSampleBlockSize = SAMPLEBLOCKSIZE;
        Parameter("SampleBlockSize") = mSampleBlockSize;
    }
    
    
	//********************************************************************************************
    //**************************** Set Channel Names *********************************************
    //********************************************************************************************
    if (Parameter("ChannelNames") == "auto") 
    {
        Parameter("ChannelNames")->SetNumValues(mNumberOfChannels);
        for (int ch = 0; ch < mNumberOfChannels; ch++) 
        {
            Parameter("ChannelNames")(ch) = "CHANNEL " + to_string(int(Parameter("SourceChList")(ch)));
        }       
    }
    else if (Parameter("ChannelNames") != "auto") 
    {
        if (Parameter("ChannelNames")->NumValues() != mNumberOfChannels)
        {
            //if the user set channels' name and doesn't set all channels 
            bcierr << "number of channel names must match number of source channels (" << mNumberOfChannels << ")" << endl;
            return;
        }
        else 
        {
            for (int ch = 0; ch < mNumberOfChannels; ch++)
            {
                mChannelNames.push_back(Parameter("ChannelNames")(ch));
            }
        }
    }

    //********************************************************************************************
    //**************************** Set Channel gains *********************************************
    //********************************************************************************************
    if (Parameter("ChannelsGain") != "auto")
    {
        if (Parameter("ChannelsGain")->NumValues() != mNumberOfChannels)
        {
            bcierr << "Please specify gain for all channels" << endl;
            return;
        }
        else 
        {
            for (int ch = 0; ch < mNumberOfChannels; ch++) 
            {
                mChannelGain[mSourceChannelList[ch]] = Parameter("ChannelsGain")(ch);
            }
        }
    }
    else if (Parameter("ChannelsGain") == "auto") 
    {        
        Parameter("ChannelsGain")->SetNumValues(mNumberOfChannels);
        for (int ch = 0; ch < mNumberOfChannels; ch++) 
        {
            Parameter("ChannelsGain")(ch) = DEFAULT_CHANNEL_GAIN;
            mChannelGain[mSourceChannelList[ch]] = Parameter("ChannelsGain")(ch);
        }
    }
    
    int possiblegains[] = { GAIN_1,GAIN_2,GAIN_4,GAIN_6,GAIN_8,GAIN_12,DEFAULT_CHANNEL_GAIN };
    bool correctGains = false;

    //Check if the Channel Gains are Reasonable
    for (int ch = 0; ch < mNumberOfChannels; ch++) 
    {
        correctGains = false;

        for (int i = 0; i < SIZE_OF_POSSIBLE_GAINS; i++) 
        {
            if (Parameter("ChannelsGain")(ch) == possiblegains[i]) 
            {
                correctGains = true;
                break;
            }
        }
        //Check if there are Impossible/Incorrect Channel Gains
        if (correctGains == false)
        {
            bcierr << "the gain for channel " << mSourceChannelList[ch] + 1 << " is out of range, the possibles values for gain are 1,2,3,6,8,12,24" << endl;
            return;
        }
    }
    //********************************************************************************************
    //****************************** Set source channel gain *************************************
    //********************************************************************************************

    Parameter("SourceChGain")->SetNumValues(mNumberOfChannels);
    mSourceChannelGains.resize(mNumberOfChannels);
    double gainFactor;
    if (Parameter("SourceChGain") != "auto") 
    {
        for (int ch = 0; ch < mNumberOfChannels; ch++)
        {
            mSourceChannelGains[ch] = (float)Parameter("SourceChGain")(ch);
            gainFactor = (double)(0.0000000217 * (mSourceChannelGains[ch]));//*((Parameter("ChannelsGain")(ch)) / mChannelGain[mSourceChannelList[ch]]);
            Parameter("SourceChGain")(ch) << gainFactor << "V";
        }
        //bcierr << "Error, Source channel gain will be Calculated automatically. This value can be changed by changing ChannelsGain" << endl << "Please set the value to <auto>" << endl;
        //return;
    }
    else
    {
        for (int ch = 0; ch < mNumberOfChannels; ch++)
        {
            gainFactor = (double)CONVERSION_FACTOR * ((Parameter("ChannelsGain")(ch)) / mChannelGain[mSourceChannelList[ch]]);
            Parameter("SourceChGain")(ch) << gainFactor << "V";
        }
    }
}

void
BioAmpADC::OnPreflight( SignalProperties& Output ) const
{
    State("BioAmpADCSyncTriggers");
    SignalType sigType = SignalType::int32;
    int samplesPerBlock = ActualParameter("SampleBlockSize");
    int numberOfSignalChannels = ActualParameter("SourceCh");
    int samplingRate = ActualParameter("SamplingRate");

    Output = SignalProperties(numberOfSignalChannels, samplesPerBlock, sigType);
    bciout << "Number of Signal Channels = " << numberOfSignalChannels << endl;
    bciout << "Sample Block Size  = " << samplesPerBlock << endl;
    bciout << "Sampling Rate = " << samplingRate << endl;
}

void
BioAmpADC::OnInitialize( const SignalProperties& Output )
{      

	BioAmpLibrary::BioAmpLib::startBioAmp(portName, EEG_CHANNELS_8, mNumberOfChannels, mSampleRate, 24);

}

void
BioAmpADC::OnStartAcquisition()
{
  // This method is called from the acquisition thread once the system is initialized.
  // You should use this space to start up your amplifier using its API.  Any initialization
  // here is done in the acquisition thread, so non-thread-safe APIs should work if initialized here.

    BioAmpLibrary::BioAmpLib::startStreaming();
    if (!BioAmpLibrary::BioAmpLib::synchronizeBioAmp())
    {
    	bciout << "couldn't synchronize device" << endl;
    }

}

void
BioAmpADC::DoAcquire( GenericSignal& Output )
{
  // Now we're acquiring a single SampleBlock of data in the acquisition thread, which is stored
  // in an internal buffer until the main thread is ready to process it.

  // Internally, BufferedADC writes this data to a buffer, then locks a mutex and pushes the data
  // into the GenericSignal output in the main thread.  The size of this buffer is parameterized by
  // "SourceBufferSize" declared by BufferedADC, which will be interpreted as follows:
  // * When set to a naked number, the number will be rounded to the nearest integer, and specify
  //   the number of BCI2000 data blocks (cf the SampleBlockSize parameter). The buffer's
  //   duration in time will vary with changes to the SamplingRate and SampleBlockSize parameters.
  // * When set to a number followed with an SI time unit, the buffer's size will be automatically
  //   chosen to match the specified time duration as close as possible. By default, the value is 2s.
  //   SI time units must be appended without white space, and consist of
  //   a valid SI prefix (such as m for milli=1e-3, mu for micro=1e-6, k for kilo=1e3),
  //   followed with a lowercase s (for seconds).

  // Keep in mind that even though we're writing this data from another thread, the main thread
  // cannot continue without data, so be sure this is done in a timely manner
  // or the system will not be able to perform in real-time.

  // IMPORTANT NOTES ABOUT BUFFERING

  // Ideally, the BCI2000 processing time ("Roundtrip time") is always shorter than the physical
  // duration of a sample block, i.e. every single block of data has been processed before its
  // successor arrives. In that ideal case, buffering makes no difference, because the main thread will
  // always wait for the acquisition thread to deliver the next block of data into the internal
  // buffer, and copy it from there immediately.

  // If, on average, processing takes longer than the physical duration of a sample block, buffering
  // will not help to improve things either, because the buffer will be filled faster than
  // it is being emptied, and thus it will overflow after a certain time. Depending on buffering strategy,
  // buffer overflow may be dealt with by looping (i.e., overwriting data that has not been delivered yet),
  // or by blocking (not accepting any new data before there is space in the buffer).
  // For scientific purposes -- as opposed to, e.g., entertainment applications -- silently omitting
  // data is not an option, so BufferedADC will use the blocking strategy, and deliver all data blocks,
  // but delayed by the physical duration of its buffer.

  // So the _only_ situation in which buffering is actually useful is for cases when processing is not
  // able to keep up with data acquisition for short amounts of time. Typical examples are lengthy 
  // computations that happen from time to time, such as updating a classifier matrix, or initialization
  // work in algorithm implementations that are not factored out into initialization and update operations
  // (otherwise, you would just do lengthy operations inside Initialize()).
  // In such cases, you should set the SourceBufferSize parameter to an estimate of the longest expected
  // delay.
		
    

    if (BioAmpLibrary::BioAmpLib::readRawData(mSampleBlockSize, rawDataBuffer))
    {
        chData = BioAmpLibrary::BioAmpLib::getChannelsData(rawDataBuffer, mSampleBlockSize);
    }


    if (mUse8Channels){

        for (int channel = 0; channel < mNumberOfChannels; channel++){

            for (int sample = 0; sample < Output.Elements(); sample++){

                Output(channel, sample) = chData.outputData[channel][sample];
            }

        }
    }

   
}

void
BioAmpADC::OnStopAcquisition()
{
  // This method is called from the acquisition thread just before it exits.  Use this method
  // to shut down the amplifier API (undoing whatever was done in OnStartAcquisition).
  // Immediately after this returns, the system will go into an un-initialized state and
  // OnHalt will be called in the main thread.

    mOpenPort = false;
    BioAmpLibrary::BioAmpLib::stopStreaming(mFileDescriptor);
    BioAmpLibrary::BioAmpLib::closeConnection();    
    bciout << "Process Stopped Acquiring Data!! Please check your connection." << endl << "Possible Solution: Disconnect & reconnect your device" << endl;
}

