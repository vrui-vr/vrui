/***********************************************************************
VruiSoundConfig - Simple Vrui application to configure audio output and
input devices.
Copyright (c) 2022-2024 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <Misc/SizedTypes.h>
#include <Misc/Utility.h>
#include <Misc/StdError.h>
#include <Misc/ConfigurationFile.h>
#include <Threads/FunctionCalls.h>
#include <Threads/WorkerPool.h>
#include <IO/OpenFile.h>
#include <GLMotif/PopupWindow.h>
#include <GLMotif/Margin.h>
#include <GLMotif/RowColumn.h>
#include <GLMotif/Blind.h>
#include <GLMotif/Label.h>
#include <GLMotif/Button.h>
#include <Sound/Config.h>
#include <Sound/WAVFile.h>
#if SOUND_CONFIG_HAVE_ALSA
#include <Sound/Linux/ALSAPCMDevice.h>
#endif
#if SOUND_CONFIG_HAVE_PULSEAUDIO
#include <Sound/Linux/PulseAudio.h>
#endif
#include <Vrui/Vrui.h>
#include <Vrui/Application.h>
#include <Vrui/Internal/Config.h>

class VruiSoundConfig:public Vrui::Application
	{
	/* Embedded classes: */
	private:
	#if SOUND_CONFIG_HAVE_ALSA
	class SoundPlayer:public Threads::FunctionCall<int> // Helper class to play a sound file in a loop in a background thread
		{
		/* Elements: */
		private:
		VruiSoundConfig& app; // Reference to the application
		Sound::ALSAPCMDevice::PCMList::iterator pcmIt; // The output device to be used
		Sound::ALSAPCMDevice* device; // Pointer to the ALSA output device
		volatile bool keepPlaying; // Flag to keep playback going
		bool deleteDevice; // Flag whether to delete the PCM device after playback is done
		
		/* Constructors and destructors: */
		public:
		SoundPlayer(VruiSoundConfig& sApp,Sound::ALSAPCMDevice::PCMList::iterator sPcmIt)
			:app(sApp),pcmIt(sPcmIt),
			 device(0),
			 keepPlaying(true),deleteDevice(true)
			{
			}
		virtual ~SoundPlayer(void)
			{
			if(deleteDevice)
				{
				/* Close the output device: */
				delete device;
				}
			}
		
		/* Methods from class Threads::FunctionCall<int>: */
		void operator()(int parameter)
			{
			/* Play the loaded sound on the selected PCM: */
			char pcmName[40];
			snprintf(pcmName,sizeof(pcmName),"plughw:%d,%d",pcmIt->cardIndex,pcmIt->deviceIndex);
			device=new Sound::ALSAPCMDevice(pcmName,false);
			device->setSoundDataFormat(app.soundFormat);
			
			/* Write to the PCM in tiny chunks: */
			device->setBufferSize(app.soundFramesPerChunk*8,app.soundFramesPerChunk);
			device->prepare();
			device->setStartThreshold(app.soundFramesPerChunk*7);
			char* framePtr=app.soundFrames;
			while(keepPlaying)
				{
				/* Write a chunk: */
				size_t writeSize=Misc::min(app.soundChunkSize,size_t(app.soundFramesEnd-framePtr));
				try
					{
					device->write(framePtr,writeSize/app.soundBytesPerFrame);
					}
				catch(const Sound::ALSAPCMDevice::UnderrunError& err)
					{
					/* Simply restart the PCM: */
					device->prepare();
					}
				framePtr+=writeSize;
				
				/* Loop the sound sample: */
				if(framePtr==app.soundFramesEnd)
					framePtr=app.soundFrames;
				}
			
			/* Stop playing on the PCM: */
			device->drop();
			}
		
		/* New methods: */
		void stop(void) // Stops playback
			{
			keepPlaying=false;
			}
		Sound::ALSAPCMDevice* retrieveDevice(void)
			{
			deleteDevice=false;
			return device;
			}
		};
	
	friend class SoundPlayer;
	#endif
	
	/* Elements: */
	int latency; // Recording/playback latency in ms
	Sound::SoundDataFormat soundFormat; // Format of the sound data to be played
	size_t soundBytesPerFrame; // Size of a sound data frame in bytes
	size_t soundNumFrames; // Number of frames in the audio sample
	char* soundFrames; // Buffer containing the audio sample
	char* soundFramesEnd; // Pointer to end of audio sample buffer
	size_t soundFramesPerChunk; // Number of frames in each playback chunk to achieve a desired latency
	size_t soundChunkSize; // Size of each playback chunk in bytes
	#if SOUND_CONFIG_HAVE_ALSA
	Sound::ALSAPCMDevice::PCMList outputDevices; // List of all ALSA output devices on the system
	GLMotif::Label* playbackDeviceNameLabel; // Label showing the name of the current output device
	GLMotif::PopupWindow* playbackDialog; // Dialog to ask whether the user can hear sound
	Sound::ALSAPCMDevice::PCMList::iterator currentOutputDevice; // Iterator to ALSA output device currently being tested
	SoundPlayer* currentPlayer; // The currently active sound player
	Sound::ALSAPCMDevice* headsetDevice; // The ALSA output device connected to the headset
	std::string headsetDeviceName; // Name of the found ALSA output device
	#endif
	#if SOUND_CONFIG_HAVE_PULSEAUDIO
	Sound::PulseAudio::Context paContext; // A PulseAudio context
	std::vector<Sound::PulseAudio::Context::SourceInfo> paSources; // List of PulseAudio sources
	GLMotif::Label* recordingDeviceNameLabel; // Label showing the name of the current PulseAudio source
	GLMotif::PopupWindow* recordingDialog; // Dialog to ask whether the user can hear the current PulseAudio source
	std::vector<Sound::PulseAudio::Context::SourceInfo>::iterator currentSource; // Iterator to PulseAudio source currently being tested
	Sound::PulseAudio::Source* source; // The PulseAudio source currently being tested
	std::string headsetSourceName; // Name of the found PulseAudio source
	#endif
	bool complete; // Flag whether sound configuration was completed (successfully or otherwise)
	const char* configFileName; // Name of a configuration file to which to write configuration results
	const char* rootSectionName; // Root section to use in given configuration file
	
	/* Private methods: */
	#if SOUND_CONFIG_HAVE_ALSA
	void playbackYesCallback(Misc::CallbackData* cbData); // Called when user can hear sound on the current output device
	void playbackNoCallback(Misc::CallbackData* cbData); // Called when user can not hear sound on the current output device
	void playbackCompleteCallback(Threads::FunctionCall<int>* job); // Callback called when the current output device is done playing the test sound
	void tryNextOutputDevice(void); // Plays the test sound on the next ALSA output device
	#endif
	#if SOUND_CONFIG_HAVE_PULSEAUDIO
	void recordingYesCallback(Misc::CallbackData* cbData); // Called when user can hear recording from the current source
	void recordingNoCallback(Misc::CallbackData* cbData); // Called when user can not hear recording from the current source
	static void recordingDataCallback(Sound::PulseAudio::Source& source,size_t numFrames,const void* frames,void* userData); // Callback called when there is new data available on the current PulseAudio source
	void tryNextSource(void); // Records from the next PulseAudio source
	#endif
	
	/* Constructors and destructors: */
	public:
	VruiSoundConfig(int& argc,char**& argv);
	virtual ~VruiSoundConfig(void);
	};

/********************************
Methods of class VruiSoundConfig:
********************************/

#if SOUND_CONFIG_HAVE_ALSA

void VruiSoundConfig::playbackYesCallback(Misc::CallbackData* cbData)
	{
	/* Remember the ALSA device name of the current output device: */
	headsetDeviceName=currentOutputDevice->name;
	
	/* Retrieve the working sound device: */
	headsetDevice=currentPlayer->retrieveDevice();
	
	/* Stop playback on the current device: */
	currentPlayer->stop();
	
	/* Pop down the playback dialog: */
	Vrui::popdownPrimaryWidget(playbackDialog);
	}

void VruiSoundConfig::playbackNoCallback(Misc::CallbackData* cbData)
	{
	/* Stop playback on the current device: */
	currentPlayer->stop();
	
	/* Go to the next output device: */
	++currentOutputDevice;
	if(currentOutputDevice!=outputDevices.end())
		tryNextOutputDevice();
	else
		{
		/* Audio won't work, but setup is complete anyway: */
		Vrui::popdownPrimaryWidget(playbackDialog);
		Vrui::showErrorMessage("Vrui Sound Configuration","No working ALSA output devices found; audio playback not supported");
		complete=true;
		}
	}

void VruiSoundConfig::playbackCompleteCallback(Threads::FunctionCall<int>* job)
	{
	/* Check if the current ALSA output device is the headset's device, meaning playback testing is over: */
	if(headsetDevice!=0)
		{
		#if SOUND_CONFIG_HAVE_PULSEAUDIO
		
		/* Start recording from the first non-monitor PulseAudio source: */
		currentSource=paSources.begin();
		while(currentSource!=paSources.end()&&currentSource->monitor)
			++currentSource;
		if(currentSource!=paSources.end())
			{
			/* Pop up the recording dialog: */
			Vrui::popupPrimaryWidget(recordingDialog);
			
			/* Start recording: */
			tryNextSource();
			}
		else
			{
			/* Recording won't work, but setup is complete anyway: */
			Vrui::showErrorMessage("Vrui Sound Configuration","No PulseAudio sources found on system; audio recording not supported");
			complete=true;
			}
		
		#else
		
		/* Recording won't work, but setup is complete anyway: */
		Vrui::showErrorMessage("Vrui Sound Configuration","PulseAudio sound library not found on system; audio recording not supported");
		complete=true;
		
		#endif
		}
	}

void VruiSoundConfig::tryNextOutputDevice(void)
	{
	/* Update and show the playback dialog: */
	playbackDeviceNameLabel->setString(currentOutputDevice->name.c_str());
	
	/* Submit a background job to play on the next device: */
	currentPlayer=new SoundPlayer(*this,currentOutputDevice);
	Vrui::submitJob(*currentPlayer,*Threads::createFunctionCall(this,&VruiSoundConfig::playbackCompleteCallback));
	}

#endif

#if SOUND_CONFIG_HAVE_PULSEAUDIO

void VruiSoundConfig::recordingYesCallback(Misc::CallbackData* cbData)
	{
	/* Remember the PulseAudio description of the current PulseAudio source: */
	headsetSourceName=currentSource->description;
	
	/* Stop recording from the current source, and stop playback on the headset device: */
	delete source;
	headsetDevice->drop();
	
	/* Pop down the recording dialog: */
	Vrui::popdownPrimaryWidget(recordingDialog);
	
	/* Sound configuration was successfully completed: */
	Vrui::showErrorMessage("Vrui Sound Configuration","Sound configuration complete and successful!","Yay!");
	complete=true;
	}

void VruiSoundConfig::recordingNoCallback(Misc::CallbackData* cbData)
	{
	/* Stop recording from the current source, and stop playback on the headset device: */
	source->stop();
	delete source;
	headsetDevice->drop();
	
	/* Go to the next non-monitor PulseAudio source: */
	++currentSource;
	while(currentSource!=paSources.end()&&currentSource->monitor)
		++currentSource;
	if(currentSource!=paSources.end())
		tryNextSource();
	else
		{
		/* Recording won't work, but setup is complete anyway: */
		Vrui::popdownPrimaryWidget(recordingDialog);
		Vrui::showErrorMessage("Vrui Sound Configuration","No working PulseAudio sources found; audio recording not supported");
		complete=true;
		}
	}

namespace {

/****************
Helper functions:
****************/

template <class SampleParam>
inline
void
writeLimitedAudio(const Sound::SoundDataFormat& format,size_t numFrames,const void* frames,size_t maxTotalEnergy,Sound::ALSAPCMDevice& outputDevice)
	{
	/* Calculate total "sound energy" in the sound chunk: */
	size_t numSamples=numFrames*format.samplesPerFrame;
	const SampleParam* sPtr=static_cast<const SampleParam*>(frames);
	const SampleParam* framesEnd=sPtr+numSamples;
	long sumS=0;
	long sumS2=0;
	for(;sPtr!=framesEnd;++sPtr)
		{
		long ls(*sPtr);
		sumS+=ls;
		sumS2+=ls*ls;
		}
	long dc=(sumS+long(numSamples)/2L)/long(numSamples);
	long eng=sumS2-(sumS*sumS+long(numSamples)/2L)/long(numSamples);
	
	static int counter=0;
	if(++counter==50)
		{
		std::cout<<dc<<", "<<eng<<std::endl;
		counter=0;
		}
	
	/* Write the recorded audio if its "sound energy" is less than the threshold: */
	try
		{
		outputDevice.write(frames,numFrames);
		}
	catch(const Sound::ALSAPCMDevice::UnderrunError& err)
		{
		/* Just restart the sound device: */
		outputDevice.prepare();
		}
	}

}

void VruiSoundConfig::recordingDataCallback(Sound::PulseAudio::Source& source,size_t numFrames,const void* frames,void* userData)
	{
	/* Access the application object: */
	VruiSoundConfig* thisPtr=static_cast<VruiSoundConfig*>(userData);
	
	// DEBUGGING
	std::cout<<'.'<<std::flush;
	
	#if 0
	
	/* Write the audio data it it's not noise: */
	const Sound::SoundDataFormat& format=source.getFormat();
	
	/* "Sound energy" of white noise at half amplitude: */
	size_t maxNoise=(numFrames*size_t(format.samplesPerFrame)*(size_t(1)<<format.bitsPerSample))/3/2;
	
	if(format.bytesPerSample==1)
		{
		if(!format.signedSamples)
			writeLimitedAudio<Misc::UInt8>(format,numFrames,frames,maxNoise,*thisPtr->headsetDevice);
		}
	else if(format.bytesPerSample==2)
		{
		if(format.signedSamples)
			writeLimitedAudio<Misc::SInt16>(format,numFrames,frames,maxNoise,*thisPtr->headsetDevice);
		else
			writeLimitedAudio<Misc::UInt16>(format,numFrames,frames,maxNoise,*thisPtr->headsetDevice);
		}
	else if(format.bytesPerSample==4)
		{
		if(format.signedSamples)
			writeLimitedAudio<Misc::SInt32>(format,numFrames,frames,maxNoise,*thisPtr->headsetDevice);
		else
			writeLimitedAudio<Misc::UInt32>(format,numFrames,frames,maxNoise,*thisPtr->headsetDevice);
		}
	
	#else
	
	/* Write the recorded audio directly to the output device: */
	try
		{
		thisPtr->headsetDevice->write(frames,numFrames);
		}
	catch(const Sound::ALSAPCMDevice::UnderrunError& err)
		{
		/* Just restart the sound device: */
		thisPtr->headsetDevice->prepare();
		}
	
	#endif
	}

void VruiSoundConfig::tryNextSource(void)
	{
	/* Update and show the recording dialog: */
	recordingDeviceNameLabel->setString(currentSource->description.c_str());
	
	// DEBUGGING
	std::cout<<"Capturing from source "<<currentSource->description<<std::endl;
	std::cout<<"\t"<<currentSource->format.bitsPerSample;
	std::cout<<(currentSource->format.signedSamples?" signed":" unsigned");
	std::cout<<(currentSource->format.sampleEndianness==Sound::SoundDataFormat::LittleEndian?" little-endian":" big-endian");
	std::cout<<" bits per sample"<<std::endl;
	std::cout<<"\t"<<currentSource->format.bytesPerSample<<" bytes per sample"<<std::endl;
	std::cout<<"\t"<<currentSource->format.samplesPerFrame<<" samples per frame"<<std::endl;
	std::cout<<"\t"<<currentSource->format.framesPerSecond<<" frames per second"<<std::endl;
	
	/* Prepare playback on the headset's PCM using the same audio sample format as the source's: */
	headsetDevice->setSoundDataFormat(currentSource->format);
	size_t framesPerChunk=(currentSource->format.framesPerSecond*latency+500)/1000;
	headsetDevice->setBufferSize(framesPerChunk*4,framesPerChunk);
	headsetDevice->prepare();
	headsetDevice->setStartThreshold(framesPerChunk*3);
	
	/* Start recording from the source: */
	source=new Sound::PulseAudio::Source(paContext,currentSource->name.c_str(),currentSource->format,latency);
	source->start(recordingDataCallback,this);
	}

#endif

VruiSoundConfig::VruiSoundConfig(int& argc,char**& argv)
	:Vrui::Application(argc,argv),
	 latency(250)
	 #if SOUND_CONFIG_HAVE_ALSA
	 ,
	 playbackDialog(0),
	 currentPlayer(0),headsetDevice(0)
	 #endif
	 #if SOUND_CONFIG_HAVE_PULSEAUDIO
	 ,
	 paContext(argv[0]),
	 recordingDialog(0),
	 source(0)
	 #endif
	 ,
	 complete(false),configFileName(0)
	{
	/* Parse the command line: */
	const char* soundFileName=0;
	for(int argi=1;argi<argc;++argi)
		{
		if(argv[argi][0]=='-')
			{
			if(strcasecmp(argv[argi],"-latency")==0||strcasecmp(argv[argi],"-l")==0)
				{
				if(argi+1<argc)
					{
					++argi;
					latency=atoi(argv[argi]);
					}
				else
					std::cerr<<"Missing latency value after "<<argv[argi]<<" command line parameter"<<std::endl;
				}
			else
				std::cerr<<"Ignoring unknown command line parameter "<<argv[argi]<<std::endl;
			}
		else if(soundFileName==0)
			soundFileName=argv[argi];
		else if(configFileName==0)
			{
			if(argi+1<argc)
				{
				/* Remember configuration file and root section: */
				configFileName=argv[argi];
				++argi;
				rootSectionName=argv[argi];
				}
			else
				std::cerr<<"No root section name provided for configuration file "<<argv[argi]<<std::endl;
			}
		else
			std::cerr<<"Ignoring extra command line argument "<<argv[argi]<<std::endl;
		}
	if(soundFileName==0)
		throw Misc::makeStdErr(__PRETTY_FUNCTION__,"No sound file name provided");
	
	#if SOUND_CONFIG_HAVE_ALSA
	
	/* Open a sound file: */
	Sound::WAVFile soundFile(IO::openFile(soundFileName));
	soundFormat=soundFile.getFormat();
	soundBytesPerFrame=soundFormat.samplesPerFrame*soundFormat.bytesPerSample;
	soundNumFrames=soundFile.getNumAudioFrames();
	
	/* Load the sound file's contents into a buffer: */
	soundFrames=static_cast<char*>(malloc(soundNumFrames*soundBytesPerFrame));
	soundFile.readAudioFrames(soundFrames,soundNumFrames);
	soundFramesEnd=soundFrames+soundNumFrames*soundBytesPerFrame;
	
	/* Calculate playback chunk sizes to approximate the requested latency: */
	soundFramesPerChunk=(soundFormat.framesPerSecond*latency+500)/1000;
	soundChunkSize=soundFramesPerChunk*soundBytesPerFrame;
	
	/* Enumerate all playback PCM devices on the system: */
	outputDevices=Sound::ALSAPCMDevice::enumeratePCMs(false);
	
	/* Create the playback confirmation dialog: */
	{
	playbackDialog=new GLMotif::PopupWindow("PlaybackDialog",Vrui::getWidgetManager(),"Vrui Sound Configuration");
	
	GLMotif::RowColumn* playback=new GLMotif::RowColumn("Playback",playbackDialog,false);
	playback->setOrientation(GLMotif::RowColumn::VERTICAL);
	playback->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	playback->setNumMinorWidgets(1);
	
	new GLMotif::Label("Label1",playback,"Currently playing on ALSA output device");
	playbackDeviceNameLabel=new GLMotif::Label("PlaybackDeviceNameLabel",playback,"");
	new GLMotif::Blind("Space",playback,0.0f,playbackDeviceNameLabel->getInterior().size[1]);
	new GLMotif::Label("Label2",playback,"Can you hear the sound sample?");
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",playback,false);
	buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	buttonBox->setNumMinorWidgets(1);
	
	GLMotif::Button* yesButton=new GLMotif::Button("YesButton",buttonBox,"Yes!");
	yesButton->getSelectCallbacks().add(this,&VruiSoundConfig::playbackYesCallback);
	
	GLMotif::Button* noButton=new GLMotif::Button("NoButton",buttonBox,"No :(");
	noButton->getSelectCallbacks().add(this,&VruiSoundConfig::playbackNoCallback);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	playback->manageChild();
	}
	
	#endif
	
	#if SOUND_CONFIG_HAVE_PULSEAUDIO
	
	/* Enumerate all PulseAudio sources on the system: */
	paSources=paContext.getSources();
	
	/* Create the recording confirmation dialog: */
	{
	recordingDialog=new GLMotif::PopupWindow("RecordingDialog",Vrui::getWidgetManager(),"Vrui Sound Configuration");
	
	GLMotif::RowColumn* recording=new GLMotif::RowColumn("Recording",recordingDialog,false);
	recording->setOrientation(GLMotif::RowColumn::VERTICAL);
	recording->setPacking(GLMotif::RowColumn::PACK_TIGHT);
	recording->setNumMinorWidgets(1);
	
	new GLMotif::Label("Label1",recording,"Currently recording from PulseAudio source");
	recordingDeviceNameLabel=new GLMotif::Label("RecordingDeviceNameLabel",recording,"");
	new GLMotif::Blind("Space1",recording,0.0f,recordingDeviceNameLabel->getInterior().size[1]);
	new GLMotif::Label("Label2",recording,"Please speak into the microphone");
	new GLMotif::Blind("Space",recording,0.0f,recordingDeviceNameLabel->getInterior().size[1]);
	new GLMotif::Label("Label3",recording,"Can you hear your own voice?");
	
	GLMotif::Margin* buttonMargin=new GLMotif::Margin("ButtonMargin",recording,false);
	buttonMargin->setAlignment(GLMotif::Alignment::RIGHT);
	
	GLMotif::RowColumn* buttonBox=new GLMotif::RowColumn("ButtonBox",buttonMargin,false);
	buttonBox->setOrientation(GLMotif::RowColumn::HORIZONTAL);
	buttonBox->setPacking(GLMotif::RowColumn::PACK_GRID);
	buttonBox->setNumMinorWidgets(1);
	
	GLMotif::Button* yesButton=new GLMotif::Button("YesButton",buttonBox,"Yes!");
	yesButton->getSelectCallbacks().add(this,&VruiSoundConfig::recordingYesCallback);
	
	GLMotif::Button* noButton=new GLMotif::Button("NoButton",buttonBox,"No :(");
	noButton->getSelectCallbacks().add(this,&VruiSoundConfig::recordingNoCallback);
	
	buttonBox->manageChild();
	
	buttonMargin->manageChild();
	
	recording->manageChild();
	}
	
	#endif
	
	#if SOUND_CONFIG_HAVE_ALSA
	
	/* Play the test sound on the first ALSA output device: */
	Vrui::popupPrimaryWidget(playbackDialog);
	currentOutputDevice=outputDevices.begin();
	if(currentOutputDevice!=outputDevices.end())
		tryNextOutputDevice();
	else
		{
		/* Audio won't work, but setup is complete anyway: */
		Vrui::showErrorMessage("Vrui Sound Configuration","No ALSA output devices found on system; audio playback not supported");
		complete=true;
		}
	
	#else
	
	/* Audio won't work, but setup is complete anyway: */
	Vrui::showErrorMessage("Vrui Sound Configuration","ALSA sound library not found on system; audio playback or recording not supported");
	complete=true;
	
	#endif
	}

VruiSoundConfig::~VruiSoundConfig(void)
	{
	if(complete)
		{
		if(configFileName!=0)
			{
			/* Find the directory containing the target configuration file: */
			std::string configDirName;
			#if VRUI_INTERNAL_CONFIG_HAVE_USERCONFIGFILE
			const char* home=getenv("HOME");
			if(home!=0&&home[0]!='\0')
				{
				configDirName=home;
				configDirName.push_back('/');
				configDirName.append(VRUI_INTERNAL_CONFIG_USERCONFIGDIR);
				}
			else
				configDirName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;			
			#else
			configDirName=VRUI_INTERNAL_CONFIG_SYSCONFIGDIR;
			#endif
			
			/* Assemble the full path of the target configuration file: */
			std::string configFilePath=configDirName;
			configFilePath.push_back('/');
			configFilePath.append(configFileName);
			
			/* Check if the target configuration file already exists: */
			if(Misc::doesPathExist(configFilePath.c_str()))
				{
				/* Patch the target configuration file: */
				std::string tagPath="Vrui/";
				tagPath.append(rootSectionName);
				tagPath.push_back('/');
				Misc::ConfigurationFile::patchFile(configFilePath.c_str(),(tagPath+"/SoundContext/deviceName").c_str(),headsetDeviceName.c_str());
				Misc::ConfigurationFile::patchFile(configFilePath.c_str(),(tagPath+"/SoundContext/recordingDeviceName").c_str(),headsetSourceName.c_str());
				}
			else
				{
				/* Write a new configuration file: */
				std::ofstream configFile(configFilePath.c_str());
				configFile<<"Sound configuration file created by VruiSoundConfig"<<std::endl<<std::endl;
				configFile<<"section Vrui"<<std::endl;
				configFile<<"\tsection "<<rootSectionName<<std::endl;
				configFile<<"\t\tsection SoundContext"<<std::endl;
				#if SOUND_CONFIG_HAVE_ALSA
				configFile<<"\t\t\tdeviceName "<<headsetDeviceName<<std::endl;
				#endif
				#if SOUND_CONFIG_HAVE_PULSEAUDIO
				configFile<<"\t\t\trecordingDeviceName "<<headsetSourceName<<std::endl;
				#endif
				configFile<<"\t\tendsection"<<std::endl;
				configFile<<"\tendsection"<<std::endl;
				configFile<<"endsection"<<std::endl;
				}
			}
		else
			{
			/* Output the detected sound configuration: */
			std::cout<<"Enter the following settings into the appropriate configuration file:"<<std::endl<<std::endl;
			std::cout<<"section SoundContext"<<std::endl;
			#if SOUND_CONFIG_HAVE_ALSA
			std::cout<<"\tdeviceName "<<headsetDeviceName<<std::endl;
			#endif
			#if SOUND_CONFIG_HAVE_PULSEAUDIO
			std::cout<<"\trecordingDeviceName "<<headsetSourceName<<std::endl;
			#endif
			std::cout<<"endsection"<<std::endl;
			}
		}
	
	/* Release all resources: */
	#if SOUND_CONFIG_HAVE_ALSA
	free(soundFrames);
	delete playbackDialog;
	#endif
	#if SOUND_CONFIG_HAVE_PULSEAUDIO
	delete recordingDialog;
	#endif
	}

VRUI_APPLICATION_RUN(VruiSoundConfig)
