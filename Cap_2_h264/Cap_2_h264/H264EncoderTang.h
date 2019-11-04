#pragma once
#ifndef H264_ENCODER_H
#define H264_ENCODER_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <Windows.h>
#include <mutex>
#include <queue>
#include <memory>

#include<opencv2/opencv.hpp>

#if defined ( __cplusplus)
extern "C"
{
#include "x264.h"
};
#else
#include "x264.h"
#endif

using namespace cv;
using namespace std;

class H264EncoderTang
{
public:
	int width;
	int height;
	int fps;

private:
	int yuvFormat;
	int yuvLen;
	Mat yuvImg;
	bool bStop;  //子线程结束
	x264_param_t* pParam;
	x264_t* pHandle;
	x264_picture_t* pPic_in;
	x264_picture_t* pPic_out;
	shared_ptr<unsigned char> yuvBuffer;
	int i_pts;

	mutex g_i_mutex;
	queue <shared_ptr<unsigned char>> frameQue;	//线程通信的队列
	queue <bool> stopQue;


public:
	H264EncoderTang();
	~H264EncoderTang();
	H264EncoderTang(int width, int height, int fps);
	void mainLoop();
	void encodeNewFrame(Mat frame);
	void release();

private:
	shared_ptr<unsigned char> getYuv();
};

#endif

