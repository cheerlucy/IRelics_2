#include "cameraipc.h"

//#define POLLING
#define MAXLEN 1024
#define SAFE_DELETE(x) { if (x) delete x; x = NULL; }				
#define DEFER_FPS 5
// Global Vars:
HINSTANCE hInst;
HWND ghWnd = NULL;
const TCHAR szWindowClass[] = TEXT("StartIPCWin32");
extern TCHAR labelConnected[MAXLEN];
TCHAR labelFrameCounter[MAXLEN];
TCHAR labelPIF[MAXLEN];
TCHAR labelFlag[MAXLEN];
TCHAR labelTarget[MAXLEN];
queue<ofxCvGrayscaleImage> IRqueue;
ofxCvGrayscaleImage IRimage;
ofxCvGrayscaleImage IRimagePrev;

bool ipcInitialized = false;
bool frameInitialized = false;
bool Connected = false;
bool Colors = false;
bool Painted = false;
short FrameWidth = 160, FrameHeight = 120, FrameDepth = 2;
int FrameSize = 19200;
bool newIR;
mutex IR_mtx;
unsigned char* pixels = new unsigned char[FrameSize];
unsigned char* pixelBuffer = new unsigned char[FrameSize]; //use for blending of the motion detect area
														   //ofPixels pixelBuffer;

/*********************************** FOR   IPC  PROCESSING  ********************************/
void InitIPC(void)
{
	HRESULT hr;
	while (!ipcInitialized) //   IS THERE ANY BETTER SOLUTIONS HERE  ?     easy to be endless loop
	{
		hr = InitImagerIPC(0);

		if (FAILED(hr))
		{
			ipcInitialized = frameInitialized = false;
			printf("failed in initImagerIPC\n");
		}
		else
		{
#ifndef POLLING 
			SetCallback_OnServerStopped(0, OnServerStopped);
			SetCallback_OnFrameInit(0, OnFrameInit);
			SetCallback_OnNewFrameEx(0, OnNewFrame);
			SetCallback_OnInitCompleted(0, OnInitCompleted);
#endif
			hr = RunImagerIPC(0);
			ipcInitialized = SUCCEEDED(hr);
		}
		if (ipcInitialized) { printf("IPC Connect!\n"); }
		else { printf("if initImagerIPC didn't fail,RunImagerIPC failed\n"); }
	}
}

void ReleaseIPC(void)
{
	Connected = false;
	if (ipcInitialized)
	{
		ReleaseImagerIPC(0);
		delete[] pixels;
		ipcInitialized = false;
	}
}

void Idle(void)
{
#ifdef POLLING 
	if (Connected && frameInitialized && (FrameBuffer != NULL))
	{
		FrameMetadata Metadata;
		if (GetFrameQueue(0))
			if (SUCCEEDED(GetFrame(0, 0, FrameBuffer, FrameSize * FrameDepth, &Metadata)))
				OnNewFrame(FrameBuffer, &Metadata);
	}
#endif
}

void HandleEvents(void)
{
#ifdef POLLING 
	if (ipcInitialized)
	{
		WORD State = GetIPCState(0, true);
		if (State & IPC_EVENT_SERVER_STOPPED)
			OnServerStopped(0);
		if (!Connected && (State & IPC_EVENT_INIT_COMPLETED))
			OnInitCompleted();
		if (State & IPC_EVENT_FRAME_INIT)
		{
			int frameWidth, frameHeight, frameDepth;
			if (SUCCEEDED(GetFrameConfig(0, &frameWidth, &frameHeight, &frameDepth)))
				Init(frameWidth, frameHeight, frameDepth);
		}
	}
#endif
}



void Init(int frameWidth, int frameHeight, int frameDepth)
{
	FrameWidth = frameWidth;
	FrameHeight = frameHeight;
	FrameSize = FrameWidth * FrameHeight;
	FrameDepth = frameDepth;

	frameInitialized = true;
#ifdef POLLING 
	SAFE_DELETE(FrameBuffer);
	int Size = FrameWidth * FrameHeight * FrameDepth;
	FrameBuffer = new char[FrameSize * FrameDepth];
#endif
}

HRESULT WINAPI OnServerStopped(int reason)
{
	ReleaseIPC();
	return 0;
}

HRESULT WINAPI OnInitCompleted(void)
{
	_stprintf_s(labelConnected, MAXLEN, TEXT("Connected with #%d"), GetSerialNumber(0));
	InvalidateRect(ghWnd, NULL, FALSE);
	//	Colors = (TIPCMode(GetIPCMode(1)) == ipcColors);
	Connected = true;
	return S_OK;
}

HRESULT WINAPI OnFrameInit(int frameWidth, int frameHeight, int frameDepth)
{
	Init(frameWidth, frameHeight, frameDepth);
	return 0;
}

HRESULT WINAPI OnNewFrame(void * pBuffer, FrameMetadata *pMetadata)//pBuffer is the buffer for all the temperature pixels  ; and pMetadate is  the information of the IPC process
{
	_stprintf_s(labelFrameCounter, MAXLEN, TEXT("Frame counter HW/SW: %d/%d"), pMetadata->CounterHW, pMetadata->Counter);
	_stprintf_s(labelPIF, MAXLEN, TEXT("PIF   DI:%d     AI1:%d     AI2:%d"), (pMetadata->PIFin[0] >> 15) == 0, pMetadata->PIFin[0] & 0x3FF, pMetadata->PIFin[1] & 0x3FF);

	_tcscpy_s(labelFlag, MAXLEN, TEXT("Flag: "));
	switch (pMetadata->FlagState)
	{
	case fsFlagOpen: _tcscat_s(labelFlag, MAXLEN, TEXT("open")); break;
	case fsFlagClose: _tcscat_s(labelFlag, MAXLEN, TEXT("closed")); break;
	case fsFlagOpening: _tcscat_s(labelFlag, MAXLEN, TEXT("opening")); break;
	case fsFlagClosing: _tcscat_s(labelFlag, MAXLEN, TEXT("closing")); break;
	}
	//	printf("b\n");



	//unsigned char* pixels = new unsigned char[FrameSize];

	short* buf = (short*)pBuffer;
	short bmax = 0, bmin = 0;
	float bavg = 0;
	float sigma = 0;
	short buff;

	for (int i = 0; i < FrameSize; i++) {


		//bavg += (buf[i]-1000);
		bavg += buf[i];
		bmax = bmax > buf[i] ? bmax : buf[i];
		bmin = bmin < buf[i] ? bmin : buf[i];
	}
	bavg = bavg / FrameSize;

	//for (int i = 0; i < FrameSize; i++) {
	//	sigma += (buf[i] - bavg)*(buf[i] - bavg);
	//}
	//sigma = sqrt(sigma / FrameSize);

	IR_mtx.lock();
	for (int i = 0; i < FrameSize; i++) {
		//     //***temp=(buf[i]-1000)/10    we want tmin~tmax   now tmin=20 tmax=45.5 so tmin mapped to unsigned char 0 tmax map to unsigned char 255
		//		pixels[i] = (unsigned char)clip((int)buf[i] - 1200);
		buff = (short)round(ofMap(buf[i], bavg - 30, bavg + 30, 0, 255, true));
		pixels[i] = (unsigned char)buff;
	}
	//printf("source %d\n", pixels[17680]);
	//IRimagePrev = IRimage;

	//	IRimage.setFromPixels(pixels, FrameWidth, FrameHeight, OF_IMAGE_GRAYSCALE); // ofImage IRimage
	IRimage.setFromPixels(pixels, FrameWidth, FrameHeight);                                             // ofxCvGrayscaleImage IRimage

	if (IRqueue.size() < 10) {
		IRqueue.push(IRimage);

	}
	else
	{
		IRqueue.push(IRimage);
		IRimagePrev = IRqueue.front();
		IRqueue.pop();
		//printf("IRqueue size:%d \n", IRqueue.size());
	}
	newIR = true;
	IR_mtx.unlock();
	//printf("IRqueue size:%d \n", IRqueue.size());

	
	return 0;


}

void GetBitmap_Limits(short* buf, int FrameSize, short *min, short *max, bool Sigma)
{
	int y;
	double Sum, Mean, Variance;
	if (!buf) return;

	if (!Sigma)
	{
		*min = SHRT_MAX;
		*max = SHRT_MIN;
		for (y = 0; y < FrameSize; y++)
		{
			*min = MIN(buf[y], *min);
			*max = MAX(buf[y], *max);
		}
		return;
	}

	Sum = 0;
	for (y = 0; y < FrameSize; y++)
		Sum += buf[y];
	Mean = (double)Sum / FrameSize;
	Sum = 0;
	for (y = 0; y < FrameSize; y++)
		Sum += (Mean - buf[y]) * (Mean - buf[y]);
	Variance = Sum / FrameSize;
	Variance = sqrt(Variance);
	Variance *= 3;  // 3 Sigma
	*min = short(Mean - Variance);
	*max = short(Mean + Variance);
	unsigned short d = *max - *min;
	if (d < 40)
	{
		d >>= 1;
		*min = *min - d;
		*max = *max + d;
	}
}

BYTE clip(int val)
{
	return (val <= 255) ? ((val > 0) ? val : 0) : 255;
};
