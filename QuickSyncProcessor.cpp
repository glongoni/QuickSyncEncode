#include "QuickSyncProcessor.h"

QuickSyncProcessor::QuickSyncProcessor() : mfxVideoParams(NULL), _isRunning(false), syncp(NULL), mfxBS(NULL), mfxSession(NULL), par(NULL)
{

}

QuickSyncProcessor::~QuickSyncProcessor()
{
	delete mfxSession;
	mfxSession = NULL;
}

mfxStatus QuickSyncProcessor::initSession(queue_t * inputQueue, queue_t* outputQueue)
{
	this->inputQueue = inputQueue;
	this->outputQueue = outputQueue;

	mfxStatus sts = MFX_ERR_NONE;

    // Initalize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW accelaration if available (on any adapter)
    // - Version 4.0
    mfxIMPL impl = MFX_IMPL_AUTO_ANY;
    mfxVersion ver = {4, 1};
    mfxSession = new MFXVideoSession();
    sts = mfxSession->Init(impl, &ver);

	return sts;
}

bool QuickSyncProcessor::hasQuickSyncHwAcceleration()
{
	mfxStatus sts = MFX_ERR_NONE;

	// Initalize Intel Media SDK session
	// - MFX_IMPL_AUTO_ANY selects HW accelaration if available (on any adapter)
	// - Version 4.0
	mfxIMPL impl = MFX_IMPL_HARDWARE_ANY;
	mfxVersion ver = {0, 1};
	MFXVideoSession* mfxSession = new MFXVideoSession();
	sts = mfxSession->Init(impl, &ver);
	
	mfxSession->Close();

	delete mfxSession;

	if(sts == MFX_ERR_NONE)
		return true;
	else
		return false;
}

mfxStatus QuickSyncProcessor::convertFrameRate(mfxF64 dFrameRate, mfxU32* pnFrameRateExtN, mfxU32* pnFrameRateExtD)
{
	mfxU32 fr; 

	fr = (mfxU32)(dFrameRate + .5);

	if (fabs(fr - dFrameRate) < 0.0001) 
	{
		*pnFrameRateExtN = fr;
		*pnFrameRateExtD = 1;
		return MFX_ERR_NONE;
	}

	fr = (mfxU32)(dFrameRate * 1.001 + .5);

	if (fabs(fr * 1000 - dFrameRate * 1001) < 10) 
	{
		*pnFrameRateExtN = fr * 1000;
		*pnFrameRateExtD = 1001;
		return MFX_ERR_NONE;
	}

	*pnFrameRateExtN = (mfxU32)(dFrameRate * 10000 + .5);
	*pnFrameRateExtD = 10000;

	return MFX_ERR_NONE;    
}

int QuickSyncProcessor::getFreeSurfaceIndex(mfxFrameSurface1** pSurfacesPool, mfxU16 nPoolSize)
{    
	if (pSurfacesPool)
		for (mfxU16 i = 0; i < nPoolSize; i++)
			if (0 == pSurfacesPool[i]->Data.Locked)
				return i;
	return MFX_ERR_NOT_FOUND;
}

bool QuickSyncProcessor::isRunning()
{
	return this->_isRunning;
}
