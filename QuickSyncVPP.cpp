#include "QuickSyncVPP.h"

QuickSyncVPP::QuickSyncVPP() : mfxVpp(NULL), inSurface(NULL), outSurface(NULL)
{

}

QuickSyncVPP::~QuickSyncVPP()
{

}

void QuickSyncVPP::initParameters(QuickSyncParams newParameters)
{
	if(mfxVideoParams)
		delete mfxVideoParams;

	mfxVideoParams = new mfxVideoParam();

	////Parametros para video converencia, lowlatency
	//mfxVideoParams->AsyncDepth = 1;
	//mfxVideoParams->mfx.GopRefDist = 1;
	//mfxVideoParams->mfx.NumRefFrame = 1;

	IOParameters inPar = newParameters.getInputParameters();
	IOParameters outPar = newParameters.getOutputParameters();

	convertFrameRate(inPar.fps, &(mfxVideoParams->vpp.In.FrameRateExtN), &(mfxVideoParams->vpp.In.FrameRateExtD));
	mfxVideoParams->vpp.In.FourCC				   = inPar.fourCC;
	mfxVideoParams->vpp.In.ChromaFormat			   = inPar.chromaFormat;
	mfxVideoParams->vpp.In.PicStruct			   = inPar.picStruct;
	mfxVideoParams->vpp.In.CropX				   = inPar.cropX;
	mfxVideoParams->vpp.In.CropY				   = inPar.cropY;
	mfxVideoParams->vpp.In.CropW				   = inPar.width;
	mfxVideoParams->vpp.In.CropH				   = inPar.height;
	mfxVideoParams->vpp.In.AspectRatioH			   = 1;
	mfxVideoParams->vpp.In.AspectRatioW			   = 1;

	 // width must be a multiple of 16 
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture  
    mfxVideoParams->vpp.In.Width  = MSDK_ALIGN16(mfxVideoParams->vpp.In.CropW); 
    mfxVideoParams->vpp.In.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfxVideoParams->vpp.In.PicStruct)?
                                    MSDK_ALIGN16(mfxVideoParams->vpp.In.CropH) : MSDK_ALIGN32(mfxVideoParams->vpp.In.CropH);


	convertFrameRate(outPar.fps, &(mfxVideoParams->vpp.Out.FrameRateExtN), &(mfxVideoParams->vpp.Out.FrameRateExtD));
	mfxVideoParams->vpp.Out.FourCC				   = outPar.fourCC;
	mfxVideoParams->vpp.Out.ChromaFormat		   = outPar.chromaFormat;
	mfxVideoParams->vpp.Out.PicStruct			   = outPar.picStruct;
	mfxVideoParams->vpp.Out.CropX				   = outPar.cropX;
	mfxVideoParams->vpp.Out.CropY				   = outPar.cropY;
	mfxVideoParams->vpp.Out.CropW				   = outPar.width;
	mfxVideoParams->vpp.Out.CropH				   = outPar.height;
	mfxVideoParams->IOPattern					   = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
	mfxVideoParams->vpp.Out.AspectRatioH		   = 1;
	mfxVideoParams->vpp.Out.AspectRatioW		   = 1;

	 // width must be a multiple of 16 
    // height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture  
    mfxVideoParams->vpp.Out.Width  = MSDK_ALIGN16(mfxVideoParams->vpp.Out.CropW); 
    mfxVideoParams->vpp.Out.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfxVideoParams->vpp.Out.PicStruct)?
                                     MSDK_ALIGN16(mfxVideoParams->vpp.Out.CropH) : MSDK_ALIGN32(mfxVideoParams->vpp.Out.CropH);

	mfxVideoParams->IOPattern					   = MFX_IOPATTERN_IN_SYSTEM_MEMORY | MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

}

//Inicia a MFXVideoSession e MFXVideoVPP com os dados existentes no parametro encodeParameters
//Precisa-se chamar a  QuickSyncVPP::initParameters antes ou o método retornará MFX_ERR_INVALID_VIDEO_PARAM
mfxStatus QuickSyncVPP::initSession(queue_t * encodedQueue, queue_t* decodedQueue)
{
	if(!mfxVideoParams)
	{
		return MFX_ERR_INVALID_VIDEO_PARAM;
	}

 	mfxStatus sts = MFX_ERR_NONE;

	sts = QuickSyncProcessor::initSession(decodedQueue, encodedQueue);

	if(MFX_WRN_PARTIAL_ACCELERATION == sts)
	{
		sts = MFX_ERR_NONE;
	}

	if(MFX_ERR_NONE != sts)
	{
		std::cout << "Erro ao iniciar sessão vpp";
		return sts;
	}

	// Initialize the Media SDK vpp
	mfxVpp = new MFXVideoVPP(*mfxSession);
	sts = mfxVpp->Init(mfxVideoParams);
	
	if(MFX_WRN_PARTIAL_ACCELERATION == sts)
	{
		sts = MFX_ERR_NONE;
	}

	if(MFX_ERR_NONE != sts)
	{
		std::cout << "Erro ao iniciar sessão vpp";
		return sts;
	}

	inSurface = new mfxFrameSurface1;
	outSurface = new mfxFrameSurface1;

	memset(inSurface, 0, sizeof(mfxFrameSurface1));
	memset(outSurface, 0, sizeof(mfxFrameSurface1));

	memcpy(&(inSurface->Info), &(mfxVideoParams->vpp.In), sizeof(mfxFrameInfo));
	memcpy(&(outSurface->Info), &(mfxVideoParams->vpp.Out), sizeof(mfxFrameInfo));

	int width = mfxVideoParams->vpp.In.CropW;
    int height = mfxVideoParams->vpp.In.CropH;

	inYSize = width * height;
	inSurface->Data.Pitch = width;

	width = mfxVideoParams->vpp.Out.CropW;
	height = mfxVideoParams->vpp.Out.CropH;

	outYSize = width * height;
	outSurface->Data.Pitch = width;

	syncp = new mfxSyncPoint();

	
	//Alocação de memória de acordo com os exemplos prevendo um processo assíncrono

	//memset(&allocRequest, 0, sizeof(mfxFrameAllocRequest)*2);

	//sts = mfxVpp->QueryIOSurf(mfxVideoParams, allocRequest);
	//
	//if(sts != MFX_ERR_NONE)
	//{
	//	std::cout << "Erro ao iniciar superficies vpp";
	//	return sts;
	//}

	//numberOfInSurfaces = allocRequest[0].NumFrameSuggested;
	//numberOfOutSurfaces = allocRequest[1].NumFrameSuggested;

	//// Alocação de superfícies de entrada

 //   // - Width and height of buffer must be aligned, a multiple of 32 
 //   // - Frame surface array keeps pointers all surface planes and general frame info
 //   mfxU16 width = (mfxU16)MSDK_ALIGN32(mfxVideoParams->vpp.In.Width);
 //   mfxU16 height = (mfxU16)MSDK_ALIGN32(mfxVideoParams->vpp.In.Height);
 //   mfxU8  bitsPerPixel = 12;  // NV12 format is a 12 bits per pixel format
 //   mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
 //   surfaceBuffersIn = (mfxU8 *)new mfxU8[surfaceSize * numberOfInSurfaces];

	//inSurfaces = new mfxFrameSurface1*[numberOfInSurfaces];
	//for (int i = 0; i < numberOfInSurfaces; i++)
 //   {       
 //       inSurfaces[i] = new mfxFrameSurface1;
 //       memset(inSurfaces[i], 0, sizeof(mfxFrameSurface1));
 //       memcpy(&(inSurfaces[i]->Info), &(mfxVideoParams->vpp.In), sizeof(mfxFrameInfo));
 //       inSurfaces[i]->Data.Y = &surfaceBuffersIn[surfaceSize * i];
 //       inSurfaces[i]->Data.U = inSurfaces[i]->Data.Y + width * height;
 //       inSurfaces[i]->Data.V = inSurfaces[i]->Data.U + 1;
 //       inSurfaces[i]->Data.Pitch = width;
	//}

	////Alocação de superfícies de saída
	//width = (mfxU16)MSDK_ALIGN32(mfxVideoParams->vpp.Out.Width);
 //   height = (mfxU16)MSDK_ALIGN32(mfxVideoParams->vpp.Out.Height);
 //   surfaceSize = width * height * bitsPerPixel / 8;
 //   mfxU8* surfaceBuffersOut = (mfxU8 *)new mfxU8[surfaceSize * numberOfOutSurfaces];

	//mfxFrameSurface1** outSurfaces = new mfxFrameSurface1*[numberOfOutSurfaces];
	//for (int i = 0; i < numberOfOutSurfaces; i++)
 //   {       
 //       outSurfaces[i] = new mfxFrameSurface1;
 //       memset(outSurfaces[i], 0, sizeof(mfxFrameSurface1));
 //       memcpy(&(outSurfaces[i]->Info), &(mfxVideoParams->vpp.Out), sizeof(mfxFrameInfo));
 //       outSurfaces[i]->Data.Y = &surfaceBuffersOut[surfaceSize * i];
 //       outSurfaces[i]->Data.U = outSurfaces[i]->Data.Y + width * height;
 //       outSurfaces[i]->Data.V = outSurfaces[i]->Data.U + 1;
 //       outSurfaces[i]->Data.Pitch = width;
 //   }

	return sts;
}

//Aplica o vpp ao parametro inputFrame e devolve o resultado no parametro outputFrame
//Ambos devem ser pré alocados
mfxStatus QuickSyncVPP::applySingleFrameVpp(uint8_t* inputFrame, uint8_t* outputFrame)
{
	this->_isRunning = true;

	inSurface->Data.Y = inputFrame;
	inSurface->Data.U = inSurface->Data.Y + inYSize; //inYSize;
	inSurface->Data.V = inSurface->Data.U + 1;

	outSurface->Data.Y = outputFrame;
	outSurface->Data.U = outSurface->Data.Y + outYSize; //outYSize;
	outSurface->Data.V = outSurface->Data.U + 1;

	mfxStatus sts = MFX_ERR_NONE;
	for (;;)
	{
		sts = mfxVpp->RunFrameVPPAsync(inSurface, outSurface, NULL, syncp); 
		if (MFX_ERR_NONE < sts && !(*syncp)) // repeat the call if warning and no output
		{
			if (MFX_WRN_DEVICE_BUSY == sts)
			{
				Sleep(1); // wait if device is busy
			}
		}
		else if (MFX_ERR_NONE < sts && *syncp)                 
		{
			sts = MFX_ERR_NONE; // ignore warnings if output is available                                    
			break;
		}
		else
		{
			break; // not a warning               
		}
	} 

	// process errors            
	if (MFX_ERR_MORE_DATA == sts)
	{
		printf("QuickSyncEncode : VPP : Error MFX_ERR_MORE_DATA\n");
	}
	else if (MFX_ERR_NONE != sts)
	{
		printf("QuickSyncEncode : VPP : Error %d\n", sts);
		this->_isRunning = false;
		return sts;
	}

	mfxSession->SyncOperation(*syncp, MSDK_VPP_WAIT_INTERVAL);

	this->_isRunning = false;

	return sts;
}

void QuickSyncVPP::endSession()
{
	while(this->_isRunning)
	{
		Yield();
	}

	if(mfxVpp)
	{
		mfxVpp->Close();
	}


	delete mfxVpp;
	mfxVpp = NULL;
	delete mfxSession;
	mfxSession = NULL;
	delete syncp;
	syncp = NULL;
	delete mfxVideoParams;
	mfxVideoParams = NULL;
	delete inSurface;
	inSurface = NULL;
	delete outSurface;
	outSurface = NULL;
}