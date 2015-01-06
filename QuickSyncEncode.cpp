#include "QuickSyncEncode.h"
#include "QueueExtraDataVideo.h"
#include <math.h>

QuickSyncEncode::QuickSyncEncode() : mfxEnc(NULL), externParams(NULL), inSurface(NULL)
{
}

QuickSyncEncode::~QuickSyncEncode()
{

}


//Inicia a MFXVideoSession e MFXVideoEncode com os dados existentes no parametro encodeParameters
//Precisa-se chamar a  QuickSyncEncode::initParameters antes ou o método retornará MFX_ERR_INVALID_VIDEO_PARAM
mfxStatus QuickSyncEncode::initSession(queue_t * encodedQueue, queue_t* decodedQueue)
{
	if(!mfxVideoParams)
	{
		return MFX_ERR_INVALID_VIDEO_PARAM;
	}

	mfxStatus sts = MFX_ERR_NONE;

	sts = QuickSyncProcessor::initSession(decodedQueue, encodedQueue);

	if(MFX_ERR_NONE != sts && MFX_WRN_PARTIAL_ACCELERATION != sts)
	{
		return sts;
	}

	// Initialize the Media SDK encoder
	mfxEnc = new MFXVideoENCODE(*mfxSession);
	sts = mfxEnc->Init(mfxVideoParams);
	
	//if(MFX_ERR_NONE != sts)
	//{
	//	return sts;
	//}

	// Retrieve video parameters selected by encoder.
    // - BufferSizeInKB parameter is required to set bit stream buffer size
	par = new mfxVideoParam();
    sts = mfxEnc->GetVideoParam(par);
    
	if(MFX_ERR_NONE != sts)
	{
		return sts;
	}

	syncp = new mfxSyncPoint();

	ySize = mfxVideoParams->mfx.FrameInfo.CropW * mfxVideoParams->mfx.FrameInfo.CropH;  //Baseia-se no tamanho original do frame para determinar as posições de U e V no buffer de entrada
	uvSize = ySize / 4;

	inSurface = initializeFrameSurface(mfxVideoParams);

	//Inicializa o Pitch do frame de entrada que será sempre o mesmo, demais inicializações são feitas na thread de encode
	inSurface->Data.Pitch = mfxVideoParams->mfx.FrameInfo.Width;	

	mfxBS = new mfxBitstream();
	mfxBS->MaxLength = par->mfx.BufferSizeInKB * 1000;
	mfxBS->Data = (mfxU8*) queue_malloc(mfxBS->MaxLength);
	MSDK_CHECK_POINTER(mfxBS->Data, MFX_ERR_MEMORY_ALLOC);

    printf("Video encode session initialized\n");

	return sts;
}

void QuickSyncEncode::endSession()
{
	while(this->isRunning())
	{
		Sleep(10);
	}

	if(mfxEnc)
	{
		mfxEnc->Close();
	}
	
	delete mfxBS;
	mfxBS = NULL;
	delete inSurface;
	inSurface = NULL;
	delete mfxEnc;
	mfxEnc = NULL;
	delete mfxSession;
	mfxSession = NULL;
	delete par;
	par = NULL;
	delete syncp;
	syncp = NULL;
	delete mfxVideoParams;
	mfxVideoParams = NULL;
	delete externParams;
	externParams = NULL;
}

//MÉTODO ANTIGO DE ENCODE FRAME POR FRAME
//mfxStatus QuickSyncEncode :: encode (unsigned char* inFrame, unsigned char** outFrame, int* outFrameSize, bool* keyFrame)
//{
//	mfxStatus sts = MFX_ERR_NONE;
//
//	//prepara o buffer de entrada em uma estrutura mfxFrameSurface1
//	//método só funciona para frames de entrada em YV12
//	inSurface->Data.Y = inFrame;
//    inSurface->Data.U = inSurface->Data.Y + ySize;
//	inSurface->Data.V = inSurface->Data.U + uvSize;
//
//	//Colocando o tamanho do buffer de saída como zero, 
//	//a libmfx interpretará os dados já existentes no buffer como lixo 
//	//e reutilizará o mesmo espaço de memória, evitando realocação
//	
//	mfxBS->DataLength = 0;
//
//	sts = mfxEnc->EncodeFrameAsync(NULL, inSurface, mfxBS, syncp);
//
//	if(sts == MFX_ERR_NONE)
//	{
//		for (;;)
//		{		
//			if (MFX_ERR_NONE < sts && !(*syncp)) // Repeat the call if warning and no output
//			{
//				printf("!syncp\n");
//				if (MFX_WRN_DEVICE_BUSY == sts)
//				{
//					printf("MFX_WRN_DEVICE_BUSY\n");
//					Sleep(1); // Wait if device is busy, then repeat the same call            
//				}
//			}
//			else if (MFX_ERR_NONE <= sts && *syncp)                 
//			{
//				sts = MFX_ERR_NONE; // Ignore warnings if output is available
//				break;
//			}
//			else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
//			{
//				printf("MFX_ERR_NOT_ENOUGH_BUFFER\n");
//				// Allocate more bitstream buffer memory here if needed...
//				break;                
//			}
//			else
//			{
//				printf("waiting sync\n");
//			}
//		}
//
//		// process errors            
//		if (MFX_ERR_MORE_DATA == sts)
//		{
//			printf("QuickSyncEncode : ENCODE : Error MFX_ERR_MORE_DATA\n");
//		}
//		else if (MFX_ERR_NONE != sts)
//		{
//			printf("QuickSyncEncode : ENCODE : Error %d\n", sts);
//		}
//		
//		if(MFX_ERR_NONE == sts)
//		{
//			sts = mfxSession->SyncOperation(*syncp, MSDK_ENC_WAIT_INTERVAL); // Synchronize. Wait until encoded frame is ready
//			memcpy(*outFrame, mfxBS->Data, mfxBS->DataLength);
//			*outFrameSize = mfxBS->DataLength;
//			*keyFrame = mfxBS->FrameType == (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR);
//		}
//	}
//
//	return sts;
//}

void * QuickSyncEncode::encodeThread(void * param)
{
	this->_isRunning = true;

	//variavel de validação do andamento da decodificação pela libmfx
	mfxStatus sts = MFX_ERR_NONE;

	//booleana controladora da thread, valor modificado na QVideoNet no momento de parar a thread
	bool * encodeThreadRun;
	encodeThreadRun = (bool*) param;

	//estrutura para manter o extraData dos frames a cada retirada da fila
	QueueExtraDataVideo * extraData = NULL;

	//cria consumidor na fila de frames codificados
	queue_consumer_t * consumer = queue_registerConsumer(this->inputQueue);

	if (!consumer) 
	{
		printf("Erro ao criar consumidor da fila\n");
	}

	//Variaveis de entrada e saída da queue
	uint32_t timestamp, inbufSize;
	uint8_t * inbuf;

	while(*encodeThreadRun)
	{
		queue_free(consumer);
		
		if(queue_dequeueCondWaitTime(consumer,&inbuf,&inbufSize,&timestamp,(QueueExtraData**)&extraData, 2000) != E_OK)
		{
			continue;
		}
		if(inbufSize == 0)
		{
			queue_free(consumer);
			continue;
		}

		//prepara o buffer de entrada em uma estrutura mfxFrameSurface1
		//método só funciona para frames de entrada em YV12
		inSurface->Data.Y = inbuf;
		inSurface->Data.U = inSurface->Data.Y + ySize;
		inSurface->Data.V = inSurface->Data.U + uvSize;
		
		//Colocando o tamanho do buffer de saída como zero, 
		//a libmfx interpretará os dados já existentes no buffer como lixo 
		//e reutilizará o mesmo espaço de memória, evitando realocação
		mfxBS->DataLength = 0;

		sts = mfxEnc->EncodeFrameAsync(NULL, inSurface, mfxBS, syncp);

		for (;;)
		{		
			if (MFX_ERR_NONE < sts && !(*syncp)) // Repeat the call if warning and no output
			{
				printf("!syncp\n");
				if (MFX_WRN_DEVICE_BUSY == sts)
				{
					printf("MFX_WRN_DEVICE_BUSY\n");
					Sleep(1); // Wait if device is busy, then repeat the same call            
				}
			}
			else if (MFX_ERR_NONE <= sts && *syncp)                 
			{
				sts = MFX_ERR_NONE; // Ignore warnings if output is available
				break;
			}
			else if (MFX_ERR_NOT_ENOUGH_BUFFER == sts)
			{
				printf("MFX_ERR_NOT_ENOUGH_BUFFER\n");
				// Allocate more bitstream buffer memory here if needed...
				break;                
			}
			else
			{
				printf("waiting sync\n");
			}
		}
		
		// process errors            
		if (MFX_ERR_MORE_DATA == sts)
		{
			printf("QuickSyncEncode : ENCODE : Error MFX_ERR_MORE_DATA\n");
		}
		else if (MFX_ERR_NONE != sts)
		{
			printf("QuickSyncEncode : ENCODE : Error %d\n", sts);
		}
		
		if(MFX_ERR_NONE == sts)
		{
			sts = mfxSession->SyncOperation(*syncp, MSDK_ENC_WAIT_INTERVAL); // Synchronize. Wait until encoded frame is ready
			if(!extraData)
			{
				extraData = new QueueExtraDataVideo();
			}

			extraData->setIsIFrame(mfxBS->FrameType == (MFX_FRAMETYPE_I | MFX_FRAMETYPE_REF | MFX_FRAMETYPE_IDR));
			uint8_t* bufToQueue = (uint8_t*) queue_malloc(mfxBS->DataLength);
			memcpy(bufToQueue, mfxBS->Data, mfxBS->DataLength);
			queue_enqueue(outputQueue, bufToQueue, mfxBS->DataLength, timestamp, (QueueExtraData*)extraData);
			
			delete extraData;
			extraData = NULL;
		}
	}

	this->_isRunning = false;

	return NULL;

}

mfxStatus QuickSyncEncode::validateParameters()
{
	mfxStatus sts = MFX_ERR_NONE;
	if(mfxVideoParams)
	{
		sts = mfxEnc->Query(mfxVideoParams, mfxVideoParams);
		MSDK_IGNORE_MFX_STS(sts, MFX_WRN_INCOMPATIBLE_VIDEO_PARAM);
		if(sts != MFX_ERR_NONE)
		{
			printf("INVALID ENCODE PARAMETERS, error %d\n", sts);
		}
	}

	return sts;
}


//Inicializa e retorna uma framesurface
mfxFrameSurface1* QuickSyncEncode::initializeFrameSurface(mfxVideoParam* par)
{
	mfxFrameSurface1* frameSurface = new mfxFrameSurface1;
	memset(frameSurface, 0, sizeof(mfxFrameSurface1));
	
	memcpy(&(frameSurface->Info), &(par->mfx.FrameInfo), sizeof(mfxFrameInfo));
	
	return frameSurface;
}

//Inicializa os atributos mfxVideoParams e mfxVppParams
//Seta os valores recebidos como parametro para os devidos campos nas estruturas da mfxlib
void QuickSyncEncode::initParameters(QuickSyncParams newParameters)
{
	if(mfxVideoParams)
	{
		delete mfxVideoParams;
		mfxVideoParams = NULL;
	}

	mfxVideoParams = new mfxVideoParam();

	//Parametros para video converencia, lowlatency
	mfxVideoParams->AsyncDepth = 1;
	mfxVideoParams->mfx.GopRefDist = 1;
	mfxVideoParams->mfx.NumRefFrame = 1;

	IOParameters inPar = newParameters.getInputParameters();
	IOParameters outPar = newParameters.getOutputParameters();

	//Parametros de saída do encode

	convertFrameRate(outPar.fps, &(mfxVideoParams->mfx.FrameInfo.FrameRateExtN), &(mfxVideoParams->mfx.FrameInfo.FrameRateExtD));
	mfxVideoParams->mfx.CodecId                    = outPar.codecId;
	mfxVideoParams->mfx.TargetKbps                 = outPar.bitrate;
	mfxVideoParams->mfx.RateControlMethod          = MFX_RATECONTROL_VBR;
	mfxVideoParams->mfx.FrameInfo.FourCC           = outPar.fourCC;
	mfxVideoParams->mfx.FrameInfo.ChromaFormat     = outPar.chromaFormat;
	mfxVideoParams->mfx.FrameInfo.PicStruct        = outPar.picStruct;
	mfxVideoParams->mfx.FrameInfo.CropX            = outPar.cropX;
	mfxVideoParams->mfx.FrameInfo.CropY            = outPar.cropY;
	mfxVideoParams->mfx.FrameInfo.CropW            = outPar.width;
	mfxVideoParams->mfx.FrameInfo.CropH            = outPar.height;
	mfxVideoParams->mfx.TargetUsage				   = outPar.preset;
	mfxVideoParams->IOPattern					   = MFX_IOPATTERN_IN_SYSTEM_MEMORY;
	mfxVideoParams->mfx.EncodedOrder			   = 0;
	mfxVideoParams->mfx.FrameInfo.AspectRatioH	   = 1;
	mfxVideoParams->mfx.FrameInfo.AspectRatioW	   = 1;
	mfxVideoParams->mfx.GopPicSize				   = outPar.gopSize;  //deve ser o mesmo gopsize usado na ffmpeg no videomed na hora de gravar o arquivo


	// width must be a multiple of 16 
	// height must be a multiple of 16 in case of frame picture and a multiple of 32 in case of field picture
	mfxVideoParams->mfx.FrameInfo.Width = MSDK_ALIGN16(mfxVideoParams->mfx.FrameInfo.CropW);
	mfxVideoParams->mfx.FrameInfo.Height = (MFX_PICSTRUCT_PROGRESSIVE == mfxVideoParams->mfx.FrameInfo.PicStruct)? 
		MSDK_ALIGN16(mfxVideoParams->mfx.FrameInfo.CropH) : MSDK_ALIGN32(mfxVideoParams->mfx.FrameInfo.CropH);

	memset(&extCodingOption, 0, sizeof(mfxExtCodingOption));

	externParams = (mfxExtBuffer**) malloc(sizeof(mfxExtBuffer*));

	reinterpret_cast<mfxExtBuffer*>(&extCodingOption)->BufferId = MFX_EXTBUFF_CODING_OPTION;
	reinterpret_cast<mfxExtBuffer*>(&extCodingOption)->BufferSz = sizeof(extCodingOption);
	extCodingOption.MaxDecFrameBuffering = 1;
	externParams[0] = (mfxExtBuffer*) &extCodingOption;

	mfxVideoParams->ExtParam = externParams;
	mfxVideoParams->NumExtParam = (mfxU16) 1;
}


//Método de conversão do frame, foi deixado para possível uso futuro, não apagar
////Converte frame em formato YUV420/Y12 para YUV420/N12 utilizando intel QuickSync
////Parametros
//// @src					Frame em Y12
//// @dest				Frame vazio a ser preenchido com o resultado da conversão N12
//mfxStatus QuickSyncEncode::convertFrameWithVPP(mfxFrameSurface1* src, mfxFrameSurface1* dest)
//{
//	mfxStatus sts = MFX_ERR_NONE;
//	for (;;)
//	{
//		sts = mfxVpp->RunFrameVPPAsync(src, dest, NULL, vppSyncp); 
//		if (MFX_ERR_NONE < sts && !(*vppSyncp)) // repeat the call if warning and no output
//		{
//			if (MFX_WRN_DEVICE_BUSY == sts)
//				Sleep(1); // wait if device is busy
//		}
//		else if (MFX_ERR_NONE < sts && *vppSyncp)                 
//		{
//			sts = MFX_ERR_NONE; // ignore warnings if output is available                                    
//			break;
//		}
//		else
//			break; // not a warning               
//	} 
//
//	// process errors            
//	if (MFX_ERR_MORE_DATA == sts)
//	{
//		printf("QuickSyncEncode : VPP : Error MFX_ERR_MORE_DATA\n");
//	}
//	else if (MFX_ERR_NONE != sts)
//	{
//		printf("QuickSyncEncode : VPP : Error %d\n", sts);
//		return sts;
//	}
//
//	//	printf("vpp end sts: %d\n", sts);
//
//	return sts;
//}

