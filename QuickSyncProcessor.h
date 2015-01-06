#ifndef QUICKSYNCPROCESSOR_H
#define QUICKSYNCPROCESSOR_H

#include <mfxvideo++.h>
#include "queue.h"

#define MSDK_PRINT_RET_MSG(ERR)			{printf("\nReturn on error: error code %d,\t%s\t%d\n\n", ERR, __FILE__, __LINE__);}
#define MSDK_CHECK_RESULT(P, X, ERR)    {if ((X) > (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_POINTER(P, ERR)      {if (!(P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_CHECK_ERROR(P, X, ERR)     {if ((X) == (P)) {MSDK_PRINT_RET_MSG(ERR); return ERR;}}
#define MSDK_IGNORE_MFX_STS(P, X)       {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_BREAK_ON_ERROR(P)          {if (MFX_ERR_NONE != (P)) break;}
#define MSDK_SAFE_DELETE_ARRAY(P)       {if (P) {delete[] P; P = NULL;}}
#define MSDK_ALIGN32(X)					(((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)
#define MSDK_SAFE_RELEASE(X)            {if (X) { X->Release(); X = NULL; }}
#define MSDK_MAX(A, B)			        (((A) > (B)) ? (A) : (B))

#define MSDK_DEC_WAIT_INTERVAL 60000
#define MSDK_ENC_WAIT_INTERVAL 10000
#define MSDK_VPP_WAIT_INTERVAL 60000

typedef mfxI32 mfxIMPL;

#include "QuickSyncParams.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MediaSDKSampleClasses.h"

//Classe base para QuickSyncEncode e QuickSyncDecode
class QuickSyncProcessor {
	
public:
	QuickSyncProcessor();
	~QuickSyncProcessor();

	mfxStatus initSession(queue_t * inputQueue, queue_t* outputQueue);
	virtual void endSession() = 0;

	static bool hasQuickSyncHwAcceleration();

	bool isRunning();
	static mfxStatus convertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD);

protected:

	int getFreeSurfaceIndex(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize);
	
	MFXVideoSession* mfxSession;
	mfxVideoParam* mfxVideoParams;

	mfxBitstream* mfxBS;
	mfxSyncPoint* syncp;

	queue_t * inputQueue;
	queue_t * outputQueue;

	int ySize;
	int uvSize;

	//Após colocar os parametros setados em mfxEncParams na mfxEnc, 
	//usa-se esta variável par para carregar os parametros da mfxEnc, 
	//o qual possuirá os mesmos campos da mfxEncParams mas com alguns 
	//campos preenchidos a mais que serão utilizados (Exemplo da MSDK)
	mfxVideoParam* par;

	bool _isRunning;
};


#endif //QUICKSYNCPROCESSOR_H