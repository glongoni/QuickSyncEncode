#include "QuickSyncParams.h"
#include "MediaSDKSampleClasses.h"
#include <stdlib.h>
#include <memory.h>

QuickSyncParams::QuickSyncParams()
{
	memset(&inputParameters, 0, sizeof(IOParameters));
	memset(&outputParameters, 0, sizeof(IOParameters));
}
QuickSyncParams::~QuickSyncParams()
{

}

IOParameters QuickSyncParams::getInputParameters()
{
	return inputParameters;
}

IOParameters QuickSyncParams::getOutputParameters()
{
	return outputParameters;
}



void QuickSyncParams::setInputParameters( IOParameters newParameters )
{
	memcpy(&inputParameters, &newParameters, sizeof(IOParameters));
}

void QuickSyncParams::setInputParameters( float fps, int width, int height, int preset /*= 7*/, int fourCC /*= MFX_FOURCC_YV12*/, int bitrate /* = 0 */, 
										  int codecId /*= MFX_CODEC_AVC*/, int gopsize /*= 30*/, int cropX /*= 0*/, int cropY /*= 0*/,
										  int picStruct /*= MFX_PICSTRUCT_PROGRESSIVE*/, int chromaFormat /*= MFX_CHROMAFORMAT_YUV420*/ )
{
	inputParameters.fps = fps;
	inputParameters.width = width;
	inputParameters.height = height;
	inputParameters.bitrate = bitrate;
	inputParameters.preset = preset;
	inputParameters.codecId = codecId;
	inputParameters.gopSize = fps;  //gopsize = fps implica em 1 frame I por segundo
	inputParameters.cropX = cropX;
	inputParameters.cropY = cropY;
	inputParameters.fourCC = fourCC;
	inputParameters.picStruct = picStruct;
	inputParameters.chromaFormat = chromaFormat;

	if(bitrate == 0)
		inputParameters.bitrate = calculateDefaultBitrate(codecId, preset, width, height, fps);
	else
		inputParameters.bitrate = bitrate;
}

void QuickSyncParams::setOutputParameters( IOParameters newParameters )
{
	memcpy(&outputParameters, &newParameters, sizeof(IOParameters));
}

void QuickSyncParams::setOutputParameters( float fps, int width, int height, int preset /*= 7*/, int fourCC /*= MFX_FOURCC_NV12*/, int bitrate /* = 0 */, 
										   int codecId /*= MFX_CODEC_AVC*/, int gopsize /*= 30*/, int cropX /*= 0*/, int cropY /*= 0*/, 
										   int picStruct /*= MFX_PICSTRUCT_PROGRESSIVE*/, int chromaFormat /*= MFX_CHROMAFORMAT_YUV420*/ )
{
	outputParameters.fps = fps;
	outputParameters.width = width;
	outputParameters.height = height;
	outputParameters.bitrate = bitrate;
	outputParameters.preset = preset;
	outputParameters.codecId = codecId;
	outputParameters.gopSize = fps;  //gopsize = fps implica em 1 frame I por segundo
	outputParameters.cropX = cropX;
	outputParameters.cropY = cropY;
	outputParameters.fourCC = fourCC;
	outputParameters.picStruct = picStruct;
	outputParameters.chromaFormat = chromaFormat;

	if(bitrate == 0)
		outputParameters.bitrate = calculateDefaultBitrate(codecId, preset, width, height, fps);
	else
		outputParameters.bitrate = bitrate;
}



int QuickSyncParams::calculateDefaultBitrate(mfxU32 nCodecId, mfxU32 nTargetUsage, mfxU32 nWidth, mfxU32 nHeight, mfxF64 dFrameRate)
{    
	PartiallyLinearFNC fnc;
	mfxF64 bitrate = 0;

	switch (nCodecId)
	{
	case MFX_CODEC_AVC : 
		{
			fnc.AddPair(0, 0);
			fnc.AddPair(25344, 225);
			fnc.AddPair(101376, 1000);
			fnc.AddPair(414720, 4000);
			fnc.AddPair(2058240, 5000);            
			break;
		}
	case MFX_CODEC_MPEG2: 
		{
			fnc.AddPair(0, 0);
			fnc.AddPair(414720, 12000);            
			break;        
		}        
	default: 
		{
			fnc.AddPair(0, 0);
			fnc.AddPair(414720, 12000);            
			break;        
		}        
	}   

	mfxF64 at = nWidth * nHeight * dFrameRate / 30.0;    

	if (!at)
		return 0;

	switch (nTargetUsage)
	{
	case MFX_TARGETUSAGE_BEST_QUALITY :
		{
			bitrate = (&fnc)->at(at);
			break;
		}
	case MFX_TARGETUSAGE_BEST_SPEED :
		{
			bitrate = (&fnc)->at(at) * 0.5;
			break;
		}
	case MFX_TARGETUSAGE_BALANCED :
	default:
		{
			bitrate = (&fnc)->at(at) * 0.75;
			break;
		}        
	}    

	return (int)bitrate;
}