#ifndef QUICKSYNCENCODEPARAMS_H
#define QUICKSYNCENCODEPARAMS_H

#include <mfxstructures.h>

typedef struct s_IOParameters {
	float fps;
	int width;
	int height;
	int bitrate;
	int cropX;
	int cropY;
	int gopSize;
	
	int codecId;      //MFX_CODEC_AVC , MFX_CODEC_MPEG2, MFX_CODEC_VC1    
	
	int preset;		  //range: 0 (undefined) 1 (BestQuality) to 4 (balanced) to 7 (BestSpeed)
	
	int fourCC;		  //MFX_FOURCC_NV12, MFX_FOURCC_YV12, MFX_FOURCC_YUY2, MFX_FOURCC_RGB3, MFX_FOURCC_RGB4, MFX_FOURCC_P8, MFX_FOURCC_P8_TEXTURE

	int picStruct;    //MFX_PICSTRUCT_PROGRESSIVE, MFX_PICSTRUCT_UNKNOWN, MFX_PICSTRUCT_FIELD_TFF, MFX_PICSTRUCT_FIELD_BFF

	int chromaFormat; //MFX_CHROMAFORMAT_MONOCHROME, MFX_CHROMAFORMAT_YUV420, 
					  //MFX_CHROMAFORMAT_YUV422, MFX_CHROMAFORMAT_YUV444, MFX_CHROMAFORMAT_YUV400, 
				      //MFX_CHROMAFORMAT_YUV411, MFX_CHROMAFORMAT_YUV422H, MFX_CHROMAFORMAT_YUV422V   
} IOParameters;

class QuickSyncParams {

public:
	QuickSyncParams();
	~QuickSyncParams();

	IOParameters getInputParameters();
	IOParameters getOutputParameters();

	void setInputParameters(IOParameters newParameters);
	void setOutputParameters(IOParameters newParameters);

	void setInputParameters(float fps, int width, int height, int preset = 7, int fourCC = MFX_FOURCC_YV12, int bitrate = 0, 
							int codecId = MFX_CODEC_AVC , int gopsize = 12, int cropX = 0, int cropY = 0,
							int picStruct = MFX_PICSTRUCT_PROGRESSIVE, int chromaFormat = MFX_CHROMAFORMAT_YUV420);

	void setOutputParameters(float fps, int width, int height, int preset = 7, int fourCC = MFX_FOURCC_NV12, int bitrate = 0, 
							 int codecId = MFX_CODEC_AVC, int gopsize = 12, int cropX = 0, int cropY = 0,
							 int picStruct = MFX_PICSTRUCT_PROGRESSIVE, int chromaFormat = MFX_CHROMAFORMAT_YUV420);

private:
	int calculateDefaultBitrate(mfxU32 nCodecId, mfxU32 nTargetUsage, mfxU32 nWidth, mfxU32 nHeight, mfxF64 dFrameRate);
	IOParameters inputParameters;
	IOParameters outputParameters;
};


#endif //QUICKSYNCENCODEPARAMS_H