#ifndef QUICKSYNCENCODE_H
#define QUICKSYNCENCODE_H

#include "QuickSyncProcessor.h"
#include "Thread.h"


class QuickSyncEncode : QuickSyncProcessor
{
	public:
		QuickSyncEncode();
		~QuickSyncEncode();
		
		mfxStatus initSession(queue_t * encodedQueue, queue_t* decodedQueue);

//		mfxStatus encode(unsigned char* inFrame, unsigned char** outFrame, int* outFrameSize, bool* keyFrame);
		void * encodeThread(void * param);
		void initParameters(QuickSyncParams newParameters);
		void endSession();
		mfxStatus validateParameters();
	private:
		mfxFrameSurface1* initializeFrameSurface(mfxVideoParam* par);
		mfxStatus convertFrameWithVPP(mfxFrameSurface1* src, mfxFrameSurface1* dest);

		MFXVideoENCODE* mfxEnc;
		mfxFrameSurface1* inSurface;
		mfxExtBuffer** externParams;
		mfxExtCodingOption extCodingOption;
		Mutex mutex;

};

#endif //QUICKSYNCENCODE_H