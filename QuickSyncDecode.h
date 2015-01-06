#ifndef QUICKSYNCDECODE_H
#define QUICKSYNCDECODE_H

#include "QuickSyncProcessor.h"
//#include "sysmem_allocator.h"


class QuickSyncDecode : public QuickSyncProcessor {

	public:
		QuickSyncDecode();
		~QuickSyncDecode();

		mfxStatus initSession(queue_t * encodedQueue, queue_t* decodedQueue);
	
		mfxStatus decode(unsigned char* inFrame, int inFrameSize, unsigned char** outFrame, int* outFrameSize, mfxBitstream * pBS);
		mfxStatus decodeHeader(unsigned char* inFrame, int inFrameSize, mfxBitstream * pBS);
		bool validateParameters();
		void endSession();
		void * decodeThread(void * param);

	private:
		void convertFrameFromNV12toYV12(mfxFrameSurface1* nv12Surface);
		mfxStatus initDecodeSession();

		mfxFrameAllocRequest	allocRequest;
		//MFXFrameAllocator*      frameAllocator;
		//mfxFrameAllocResponse   allocResponse;

		MFXVideoDECODE* mfxDec;
		mfxFrameSurface1** workingSurfaces;
		mfxFrameSurface1*** outSurface;
		mfxU8* auxUV;
		mfxU8* workingSurfacesBuffer;
		int numberOfWorkingSurfaces;
		int surfacesSize;

		CTimeInterval<> decodeTimer;
		bool frameComplete;

		//Métodos de debug para gravação e leitura de arquivos
		//mfxStatus WriteSection(mfxU8* plane, mfxU16 factor, mfxU16 chunksize, mfxFrameInfo *pInfo, mfxFrameData *pData, mfxU32 i, mfxU32 j, FILE* fSink);
		//mfxStatus WriteRawFrame(mfxFrameSurface1 *pSurface, FILE* fSink);
		//mfxStatus ReadBitStreamData(mfxBitstream *pBS, FILE* fSource);


};

#endif   //QUICKSYNCDECODE_H