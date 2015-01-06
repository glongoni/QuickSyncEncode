#ifndef QUICKSYNCVPP_H
#define QUICKSYNCVPP_H

#include "QuickSyncProcessor.h"

class QuickSyncVPP : QuickSyncProcessor {

public:
	QuickSyncVPP();
	~QuickSyncVPP();

	mfxStatus initSession(queue_t * encodedQueue, queue_t* decodedQueue);
	void endSession();
	void initParameters(QuickSyncParams newParameters);

	mfxStatus applySingleFrameVpp(uint8_t* inputFrame, uint8_t* outputFrame);

private:
	MFXVideoVPP* mfxVpp;

	mfxFrameSurface1* inSurface;
	mfxFrameSurface1* outSurface;

	int inYSize;
	int outYSize;

	//allocRequest[0] = InFramesRequest, 
	//allocRequest[1] = OutFramesRequest
	//mfxFrameAllocRequest allocRequest[2];

	//int numberOfInSurfaces;
	//int numberOfOutSurfaces;

	//int inSurfacesIndex;
	//int outSurfacesIndex;

	//mfxU8* surfaceBuffersIn;
	//mfxFrameSurface1** inSurfaces;

};

#endif //QUICKSYNCVPP_H