
#pragma once
#include "ofMain.h"
#include "Lib\ImagerIPC2.h"
#include "ofxOpenCv.h"
#include<mutex>
#include <queue>
extern ofxCvGrayscaleImage IRimage;
extern ofxCvGrayscaleImage IRimagePrev;
extern mutex IR_mtx;
extern short FrameWidth, FrameHeight, FrameDepth;
extern int FrameSize;
extern unsigned char* pixelBuffer; //use for blending of the motion detect area



void InitIPC(void);
void ReleaseIPC(void);
void Idle(void);
void HandleEvents(void);
void Init(int frameWidth, int frameHeight, int frameDepth);
BYTE clip(int val);
void GetBitmap_Limits(short* buf, int FrameSize, short *min, short *max, bool Sigma);

HRESULT WINAPI OnServerStopped(int reason);
HRESULT WINAPI OnInitCompleted(void);
HRESULT WINAPI OnFrameInit(int frameWidth, int frameHeight, int frameDepth);
HRESULT WINAPI OnNewFrame(void * pBuffer, FrameMetadata *pMetadata);
