#ifndef MEDIASDKSAMPLECLASSES
#define MEDIASDKSAMPLECLASSES

#include <windows.h>
#include <cstdio>

//AS CLASSES NESTE ARQUIVO FORAM RETIRADAS DOS EXEMPLOS DA INTEL MEDIA SDK
//TODAS AS CLASSES SÃO UTILIZADAS PARA OTIMIZAR E AUXILIAR OS PROCESSOS DE ENCODE E DECODE

///CLASSES PARA AUXILIAR NA ALOCAÇÂO DE MEMÓRIA




//CLASSES PARA AUXILIAR NO CÁLCULO DO TIMESTAMP DA DECODIFICAÇÃO 

//timeinterval calculation helper

//! Base class for types that should not be assigned.
class no_assign {
	// Deny assignment
	void operator=( const no_assign& );
public:
};

//! Base class for types that should not be copied or assigned.
class no_copy: no_assign {
	//! Deny copy construction
	no_copy( const no_copy& );
public:
	//! Allow default construction
	no_copy() {}
};

template <int tag = 0>
class CTimeInterval : private no_copy
{
	static double g_Freq;
	double       &m_start;
	double        m_own;//reference to this if external counter not required
	//since QPC functions are quite slow it make sense to optionally enable them
	bool         m_bEnable;
	LARGE_INTEGER m_liStart;

public:
	CTimeInterval(double &dRef , bool bEnable = true)
		: m_start(dRef)
		, m_bEnable(bEnable)
	{
		if (!m_bEnable)
			return;
		Initialize();
	}
	CTimeInterval(bool bEnable = true)
		: m_start(m_own)
		, m_bEnable(bEnable)
		, m_own()
	{
		if (!m_bEnable)
			return;
		Initialize();
	}

	//updates external value with current time
	double Commit()
	{
		if (!m_bEnable)
			return 0.0;

		if (0.0 != g_Freq)
		{
			LARGE_INTEGER liEnd;
			QueryPerformanceCounter(&liEnd);
			m_start = ((double)liEnd.QuadPart - (double)m_liStart.QuadPart)  / g_Freq;
		}
		return m_start;
	}
	//lastcomitted value
	double Last()
	{
		return m_start;
	}
	~CTimeInterval()
	{
		Commit();
	}
private:
	void Initialize()
	{
		if (0.0 == g_Freq)
		{
			QueryPerformanceFrequency(&m_liStart);
			g_Freq = (double)m_liStart.QuadPart;
		}
		QueryPerformanceCounter(&m_liStart);
	}
};

template <int tag>double CTimeInterval<tag>::g_Freq = 0.0f;


//CLASSE PARA AUXILIAR NO CÁLCULO DO BITRATE


// A macro to disallow the copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
	TypeName(const TypeName&);               \
	void operator=(const TypeName&)   


//piecewise linear function for bitrate approximation
class PartiallyLinearFNC
{
	mfxF64 *m_pX;
	mfxF64 *m_pY;
	mfxU32  m_nPoints;
	mfxU32  m_nAllocated;

public:
	PartiallyLinearFNC(): m_pX()
		, m_pY()
		, m_nPoints()
		, m_nAllocated()
	{
	};
	~PartiallyLinearFNC(){
		delete []m_pX;
		m_pX = NULL;
		delete []m_pY;
		m_pY = NULL;
	}

	void AddPair(mfxF64 x, mfxF64 y) {
		
		//duplicates searching 
		for (mfxU32 i = 0; i < m_nPoints; i++)
		{
			if (m_pX[i] == x)
				return;
		}
		if (m_nPoints == m_nAllocated)
		{
			m_nAllocated += 20;
			mfxF64 * pnew;
			pnew = new mfxF64[m_nAllocated];
			memcpy(pnew, m_pX, sizeof(mfxF64) * m_nPoints);
			delete [] m_pX;
			m_pX = pnew;

			pnew = new mfxF64[m_nAllocated];
			memcpy(pnew, m_pY, sizeof(mfxF64) * m_nPoints);
			delete [] m_pY;
			m_pY = pnew;
		}
		m_pX[m_nPoints] = x;
		m_pY[m_nPoints] = y;

		m_nPoints ++;
	}

	mfxF64 at(mfxF64 x) {

		if (m_nPoints < 2)
		{
			return 0;
		}
		bool bwasmin = false;
		bool bwasmax = false;

		mfxU32 maxx = 0;
		mfxU32 minx = 0;
		mfxU32 i;

		for (i=0; i < m_nPoints; i++)
		{
			if (m_pX[i] <= x && (!bwasmin || m_pX[i] > m_pX[maxx]))
			{
				maxx = i;
				bwasmin = true;
			}
			if (m_pX[i] > x && (!bwasmax || m_pX[i] < m_pX[minx]))
			{
				minx = i;
				bwasmax = true;
			}
		}

		//point on the left
		if (!bwasmin)
		{
			for (i=0; i < m_nPoints; i++)
			{
				if (m_pX[i] > m_pX[minx] && (!bwasmin || m_pX[i] < m_pX[minx]))
				{
					maxx = i;
					bwasmin = true;
				}
			}
		}
		//point on the right
		if (!bwasmax)
		{
			for (i=0; i < m_nPoints; i++)
			{
				if (m_pX[i] < m_pX[maxx] && (!bwasmax || m_pX[i] > m_pX[minx]))
				{
					minx = i;
					bwasmax = true;
				}
			}
		}

		//linear interpolation
		return (x - m_pX[minx])*(m_pY[maxx] - m_pY[minx]) / (m_pX[maxx] - m_pX[minx]) + m_pY[minx];
	}

private:
	DISALLOW_COPY_AND_ASSIGN(PartiallyLinearFNC);
};

#endif //MEDIASDKSAMPLECLASSES