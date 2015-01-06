#define _WINSOCKAPI_     /* Prevent inclusion of winsock.h in windows.h */

#include "QuickSyncDecode.h"

QuickSyncDecode::QuickSyncDecode() : mfxDec(NULL), frameComplete(false), decodeTimer(frameComplete), workingSurfaces(NULL), numberOfWorkingSurfaces(0), workingSurfacesBuffer(NULL), auxUV(NULL)
{
	
}

QuickSyncDecode::~QuickSyncDecode()
{
	
}

//Inicia a MFXVideoSession
mfxStatus QuickSyncDecode::initSession(queue_t * encodedQueue, queue_t* decodedQueue)
{
	mfxStatus sts = MFX_ERR_NONE;

	sts = QuickSyncProcessor::initSession(encodedQueue, decodedQueue);

	syncp = new mfxSyncPoint();

	mfxVideoParams = new mfxVideoParam();
	memset(mfxVideoParams, 0, sizeof(mfxVideoParams));
	mfxVideoParams->mfx.CodecId = MFX_CODEC_AVC;
    mfxVideoParams->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
	mfxVideoParams->AsyncDepth = 1;
	mfxVideoParams->mfx.GopRefDist = 1;
	mfxVideoParams->mfx.NumRefFrame = 1;

	mfxDec = new MFXVideoDECODE(*mfxSession);

	mfxBS = new mfxBitstream();
	mfxBS->MaxLength = 1920 * 1080 * 12;
	mfxBS->DataLength = 0;
	mfxBS->Data = new mfxU8[mfxBS->MaxLength];
	mfxBS->DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;

	printf("Video decode session initialized\n");

	return sts;
}

void QuickSyncDecode::endSession()
{
	while(this->isRunning())
	{
		Sleep(10);
	}
	if(mfxDec)
	{
		mfxDec->Close();
	}
	
	delete mfxDec;
	mfxDec = NULL;
	delete mfxBS->Data;
	mfxBS->Data = NULL;
	delete mfxBS;
	mfxBS = NULL;
	delete mfxVideoParams;
	mfxVideoParams = NULL;
	delete syncp;
	syncp = NULL;
	delete par;
	par = NULL;
	delete workingSurfacesBuffer;
	workingSurfacesBuffer = NULL;
	for(int i = 0; i < numberOfWorkingSurfaces; i++)
	{
		delete workingSurfaces[i];
		workingSurfaces[i] = NULL;
	}
	delete workingSurfaces;
	workingSurfaces = NULL;
	delete auxUV;
	auxUV = NULL;
}


//Antiga thread de decode
//mfxStatus QuickSyncDecode::decode(unsigned char* inFrame, int inFrameSize, unsigned char** outFrame, int* outFrameSize, mfxBitstream * pBS)
//{
//	mfxStatus sts = MFX_ERR_NONE;
//	int workingSurfacesIndex = 0;
//	static int f  = 0;
//
//	if(receiveFrom264File)
//	{
//		mfxBS = pBS;
//		mfxBS->DataFlag = MFX_BITSTREAM_COMPLETE_FRAME;
//	}
//	else
//	{
//		memmove(mfxBS->Data, mfxBS->Data + mfxBS->DataOffset, mfxBS->DataLength);
//		mfxBS->DataOffset = 0;
//		memcpy(mfxBS->Data + mfxBS->DataLength, inFrame, inFrameSize);
//		mfxBS->DataLength += inFrameSize;
//	}
//	
//		
//	if(frameComplete)
//	{
//		mfxBS->TimeStamp = (mfxU64)(decodeTimer.Commit() * 90000);
//	}
//	
//
//	while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts)          
//	{
//		if (MFX_WRN_DEVICE_BUSY == sts)
//		{
//			Sleep(1); // just wait and then repeat the same call to DecodeFrameAsync
//			if (frameComplete)
//			{
//				//in low latency mode device busy lead to increasing of latency, printing is optional since 15 ms s more than threshold
//				printf("Warning : latency increased due to MFX_WRN_DEVICE_BUSY\n");
//			}
//		}
//		else if (MFX_ERR_MORE_DATA == sts || frameComplete )
//		{
//			//Termina o loop, retorna o valor do sts para que seja enviado um novo buffer
//			break;	
//		}
//		if (MFX_ERR_MORE_SURFACE == sts || (MFX_ERR_NONE == sts && frameComplete))
//		{
//			workingSurfacesIndex = getFreeSurfaceIndex(workingSurfaces, numberOfWorkingSurfaces);
//			if(workingSurfacesSize == MFX_ERR_NOT_FOUND)
//			{
//				printf("DECODE ERROR: Falta de Working Frames\n");
//			}
//		}        
//
//		sts = mfxDec->DecodeFrameAsync(mfxBS, workingSurfaces[workingSurfacesIndex], &outSurface, syncp);
//
//		// ignore warnings if output is available, 
//		// if no output and no action required just repeat the same call
//		if (MFX_ERR_NONE < sts && syncp) 
//		{
//			printf("IGNORE MFX_DECODE_WARNING : %d\n", sts);
//			sts = MFX_ERR_NONE;
//		}
//		
//
//		if (MFX_ERR_NONE == sts)
//		{
//			sts = mfxSession->SyncOperation(*syncp, MSDK_DEC_WAIT_INTERVAL);
//			frameComplete = true;
//		}          
//
//		if (MFX_ERR_NONE == sts)
//		{                
//			decodeTimer.Commit();
//		}
//
//	} //while processing
//
//	static int k = 0;
//	k++;
//
//	if(frameComplete)
//	{
//		//convertFrameFromNV12toYV12(outSurface);
//		if(outSurface)
//		{
//			if(recordYUVFile && k < 500)
//			{
//				WriteRawFrame(outSurface, outputYUVFile);
//			}
//			else if(recordYUVFile && k == 500)
//			{
//				fclose(outputYUVFile);
//			}
//
//			*outFrameSize = ySize * 12 / 8;
//			memcpy(*outFrame, outSurface->Data.Y, *outFrameSize);
//			outSurface = NULL;
//		}
//		frameComplete = false;
//	}
//
//	return sts;
//}

void * QuickSyncDecode::decodeThread(void * param)
{
	//variavel de validação do andamento da decodificação pela libmfx
	mfxStatus sts = MFX_ERR_NONE;

	//booleana controladora da thread, valor modificado na QVideoNet no momento de parar a thread
	bool * decodeThreadRun;
	decodeThreadRun = (bool*) param;

	//estrutura para manter o extraData dos frames a cada retirada da fila
	QueueExtraData * extraData = NULL;

	//cria consumidor na fila de frames codificados
	queue_consumer_t * consumer = queue_registerConsumer(this->inputQueue);

	if (!consumer) 
	{
		printf("Erro ao criar consumidor da fila\n");
	}

	//Variaveis de entrada e saída da queue
	uint32_t timestamp, inbufSize;
	uint8_t * inbuf;

	_isRunning = true;

	//Primeiro laço: Recupera os parametros de codificação
	while(*decodeThreadRun)
	{
		//libera o consumidor antes de pegar o próximo frame
		queue_free(consumer);

		//tira frame codificado da fila que a NetRecv preenche
		if(queue_dequeueCondWaitTime(consumer, &inbuf, &inbufSize, &timestamp, &extraData, 2000) != E_OK)
		{
			continue;
		}
		if(inbufSize == 0)
		{
			queue_free(consumer);
			continue;
		}

		memmove(mfxBS->Data, mfxBS->Data + mfxBS->DataOffset, mfxBS->DataLength);
		mfxBS->DataOffset = 0;
		memcpy(mfxBS->Data + mfxBS->DataLength, inbuf, inbufSize);
		mfxBS->DataLength += inbufSize;

		sts = mfxDec->DecodeHeader(mfxBS, mfxVideoParams);

		if(MFX_ERR_MORE_DATA == sts)
		{
			continue;
		}
		if(MFX_ERR_NONE == sts)
		{
			break;
		}
	}

	std::cout << "\nH264 HEADER FOUND\n";

	//Inicia a decodeSession e estruturas de auxilio da decodificação
	if(*decodeThreadRun)
	{
		this->initDecodeSession();
	}

	//Controla o array de WorkingSurfaces
	int workingSurfacesIndex = 0;

	//Segundo laço :  Decodificação
	while (*decodeThreadRun && (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts))          
	{
		if (MFX_WRN_DEVICE_BUSY == sts)
		{
			Sleep(1); // just wait and then repeat the same call to DecodeFrameAsync
			
			//in low latency mode device busy lead to increasing of latency, printing is optional since 15 ms s more than threshold
			printf("Warning : latency increased due to MFX_WRN_DEVICE_BUSY\n");
		}

		if (MFX_ERR_MORE_DATA == sts)
		{
			//libera o consumidor antes de pegar o próximo frame
			queue_free(consumer);

			//tira frame codificado da fila que a NetRecv preenche
			while(queue_dequeueCondWaitTime(consumer, &inbuf, &inbufSize, &timestamp, &extraData, 2000) != E_OK)
			{
				continue;
			}
			if(inbufSize == 0)
			{
				queue_free(consumer);
				continue;
			}
			
			memmove(mfxBS->Data, mfxBS->Data + mfxBS->DataOffset, mfxBS->DataLength);
			memcpy(mfxBS->Data + mfxBS->DataLength, inbuf, inbufSize);
			mfxBS->DataOffset = 0;
			mfxBS->DataLength += inbufSize;
			mfxBS->TimeStamp = (mfxU64)(decodeTimer.Commit() * 90000);
		}

		if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
		{
			workingSurfacesIndex = getFreeSurfaceIndex(workingSurfaces, numberOfWorkingSurfaces);
			if(MFX_ERR_NOT_FOUND == workingSurfacesIndex)
			{
				printf("DECODE ERROR: Falta de Working Frames\n");
			}
		}        
		
		sts = mfxDec->DecodeFrameAsync(mfxBS, workingSurfaces[workingSurfacesIndex], *outSurface, syncp);

		// ignore warnings if output is available, 
		// if no output and no action required just repeat the same call
		if (MFX_ERR_NONE < sts && syncp) 
		{
		//	printf("IGNORE MFX_DECODE_WARNING : %d\n", sts);
			sts = MFX_ERR_NONE;
		}

		if (MFX_ERR_NONE == sts)
		{
			sts = mfxSession->SyncOperation(*syncp, MSDK_DEC_WAIT_INTERVAL);

			if(MFX_ERR_NULL_PTR == sts || MFX_ERR_UNKNOWN == sts)
			{
			//	printf("IGNORING : MFX_ERR_NULL_PTR on sync operation\n");
				sts = MFX_ERR_MORE_DATA;
			}

		}          
		
		if (MFX_ERR_NONE == sts)
		{   
			queue_enqueue(this->outputQueue, (uint8_t*) outSurface, sizeof(mfxFrameSurface1***), timestamp, extraData);
			outSurface = (mfxFrameSurface1***) queue_malloc(sizeof(mfxFrameSurface1***));
			*outSurface = (mfxFrameSurface1**) malloc(sizeof(mfxFrameSurface1**));
		}
	}
	_isRunning = false;
	queue_free(consumer);
	queue_unregisterConsumer(&consumer);
	return NULL;
}

//Inicia a decodeSession e estruturas de auxilio da decodificação
mfxStatus QuickSyncDecode::initDecodeSession()
{
	// Initialize the Media SDK decode
	mfxStatus sts = mfxDec->Init(mfxVideoParams);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	mfxDec->QueryIOSurf(mfxVideoParams, &allocRequest);

	memcpy(&(allocRequest.Info), &(mfxVideoParams->mfx.FrameInfo), sizeof(mfxFrameInfo));
	allocRequest.Type = MFX_MEMTYPE_EXTERNAL_FRAME | MFX_MEMTYPE_FROM_DECODE; 
	allocRequest.Type |= MFX_MEMTYPE_SYSTEM_MEMORY;

	numberOfWorkingSurfaces = allocRequest.NumFrameSuggested;

	// Retrieve video parameters selected by encoder.
	// - BufferSizeInKB parameter is required to set bit stream buffer size
	par = new mfxVideoParam();
	sts = mfxDec->GetVideoParam(par);
	MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

	//Baseia-se no tamanho original do frame para determinar as posições de U e V no buffer de entrada
	ySize = mfxVideoParams->mfx.FrameInfo.CropW * mfxVideoParams->mfx.FrameInfo.CropH;  
	uvSize = ySize / 4;

	//Aloca superfices auxiliares utilizadas pelo processo de decode
	surfacesSize = mfxVideoParams->mfx.FrameInfo.Width * mfxVideoParams->mfx.FrameInfo.Height * 12 / 8;
	workingSurfacesBuffer = (mfxU8 *)new mfxU8[surfacesSize * numberOfWorkingSurfaces];

	workingSurfaces = new mfxFrameSurface1*[numberOfWorkingSurfaces];
	for(int i = 0; i < numberOfWorkingSurfaces; i++)
	{
		workingSurfaces[i] = new mfxFrameSurface1;
		memset(workingSurfaces[i], 0, sizeof(mfxFrameSurface1));
		memcpy(&(workingSurfaces[i]->Info), &(mfxVideoParams->mfx.FrameInfo), sizeof(mfxFrameInfo));
		workingSurfaces[i]->Data.Y = &workingSurfacesBuffer[surfacesSize * i];
		workingSurfaces[i]->Data.U = workingSurfaces[i]->Data.Y + ySize;
		workingSurfaces[i]->Data.V = workingSurfaces[i]->Data.U + 1;
		workingSurfaces[i]->Data.Pitch = mfxVideoParams->mfx.FrameInfo.CropW;
	}

	//superficie de saida inicializada, será desalocada externamente pela aplicação
	outSurface = (mfxFrameSurface1***) queue_malloc(sizeof(mfxFrameSurface1***));
	*outSurface = (mfxFrameSurface1**) malloc(sizeof(mfxFrameSurface1**));
	

	//Parametros para otimizar a decodificação de low latency
	mfxVideoParams->AsyncDepth = 1;


	//Superficie auxiliar utilizada em caso de conversão de NV12 -> YV12 via software
	auxUV = new mfxU8[2*uvSize];
}


void QuickSyncDecode::convertFrameFromNV12toYV12(mfxFrameSurface1* nv12Surface)
{
	int i, j;
	for(j = 0, i = 0; j < 2* uvSize - 1; i++, j++)
	{
		auxUV[i] = nv12Surface->Data.U[j];
		auxUV[uvSize + i] = nv12Surface->Data.U[++j];
	}

	memcpy(nv12Surface->Data.U, auxUV, 2* uvSize);
}



//mfxStatus QuickSyncDecode::WriteSection(mfxU8* plane, mfxU16 factor, mfxU16 chunksize, mfxFrameInfo *pInfo, mfxFrameData *pData, mfxU32 i, mfxU32 j, FILE* fSink)
//{
//    if(chunksize != fwrite( plane + (pInfo->CropY * pData->Pitch / factor + pInfo->CropX)+ i * pData->Pitch + j,
//                            1,
//                            chunksize,
//                            fSink))
//        return MFX_ERR_UNDEFINED_BEHAVIOR;
//    return MFX_ERR_NONE;
//}
//
//mfxStatus QuickSyncDecode::WriteRawFrame(mfxFrameSurface1 *pSurface, FILE* fSink)
//{
//    mfxFrameInfo *pInfo = &pSurface->Info;
//    mfxFrameData *pData = &pSurface->Data;
//    mfxU32 i, j, h, w;
//    mfxStatus sts = MFX_ERR_NONE;
//
//    for (i = 0; i < pInfo->CropH; i++)
//        sts = WriteSection(pData->Y, 1, pInfo->CropW, pInfo, pData, i, 0, fSink);
//
//    h = pInfo->CropH / 2;
//    w = pInfo->CropW;
//    for (i = 0; i < h; i++)
//        for (j = 0; j < w; j += 2)
//            sts = WriteSection(pData->UV, 2, 1, pInfo, pData, i, j, fSink);
//    for (i = 0; i < h; i++)
//        for (j = 1; j < w; j += 2)
//            sts = WriteSection(pData->UV, 2, 1, pInfo, pData, i, j, fSink);
//
//    return sts;
//}

