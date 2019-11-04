#include "H264EncoderTang.h"

//参考链接：https://blog.csdn.net/zzqgtt/article/details/81390327  对pPic_in的赋值是对的
//参考链接：https://blog.csdn.net/leixiaohua1020/article/details/42078645  对pPic_in的赋值有误，导致转码后的视频有重影

//在类外启动线程执行mainLoop
//thread th(&H264EncoderTang::mainLoop, std::ref(coder264)); //coder264为类的实例
//th.detach();

H264EncoderTang::H264EncoderTang()
{
}

H264EncoderTang::~H264EncoderTang()
{
}

H264EncoderTang::H264EncoderTang(int width, int height, int fps)
{
	this->width = width;
	this->height = height;
	this->fps = fps;

	this->bStop = false;
	this->i_pts = 0;

	this->yuvFormat = X264_CSP_I420;
	this->pParam = (x264_param_t*)malloc(sizeof(x264_param_t));
	x264_param_default(this->pParam);
	this->pParam->i_height = this->height;
	this->pParam->i_width = this->width;
	this->pParam->i_fps_num = this->fps;
	this->pParam->i_csp = this->yuvFormat;
	x264_param_apply_profile(this->pParam, x264_profile_names[5]);

	this->pHandle = x264_encoder_open(this->pParam);
	if (this->pHandle == NULL) {
		printf("x264_encoder_open err\n");
		throw;
	}
	int y_size = this->pParam->i_width * this->pParam->i_height;


	this->pPic_in = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	this->pPic_out = (x264_picture_t*)malloc(sizeof(x264_picture_t));
	x264_picture_init(this->pPic_out);
	x264_picture_alloc(this->pPic_in, this->yuvFormat, this->pParam->i_width, this->pParam->i_height);

	this->yuvLen = this->width * this->height * 3 / 2;
	this->yuvBuffer = shared_ptr<unsigned char>(new unsigned char[this->yuvLen], [](unsigned char* p) {delete[] p; });
}

void H264EncoderTang::encodeNewFrame(Mat frame)
{
	cvtColor(frame, this->yuvImg, CV_BGR2YUV_I420);
	memcpy(this->yuvBuffer.get(), this->yuvImg.data, this->yuvLen * sizeof(unsigned char));
	lock_guard<mutex> lock(this->g_i_mutex);
	this->frameQue.push(this->yuvBuffer);
}


void H264EncoderTang::mainLoop() 
{
	shared_ptr<unsigned char> yuvBufTransfer;
	x264_nal_t* pNals = NULL;
	int iNal = 0;
	int ret;

	FILE* fp_dst = NULL;
	fp_dst = fopen("test.h264", "wb");

	while (!this->bStop)
	{
		yuvBufTransfer = getYuv();

		if (yuvBufTransfer)
		{
			//yuv420
			this->pPic_in->img.plane[0] = yuvBufTransfer.get();
			this->pPic_in->img.plane[1] = yuvBufTransfer.get() + this->height * this->width;
			this->pPic_in->img.plane[2] = yuvBufTransfer.get() + this->height * this->width * 5 / 4;	//这个存储是对的

			//网络版本多数是这个，这个显示异常
			//this->pPic_in->img.plane[1] = yuvBufTransfer.get() + this->height * this->width / 4;
			//this->pPic_in->img.plane[2] = yuvBufTransfer.get() + this->height * this->width / 4;

			this->pPic_in->img.i_stride[0] = this->pParam->i_width;
			this->pPic_in->img.i_stride[1] = this->pParam->i_width >> 1;
			this->pPic_in->img.i_stride[2] = this->pParam->i_width >> 1;

			this->pPic_in->i_pts++;
			ret = x264_encoder_encode(this->pHandle, &pNals, &iNal, this->pPic_in, this->pPic_out);

			for (int j = 0; j < iNal; ++j) {
				fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
			}
		}
		else
		{
			this_thread::sleep_for(chrono::milliseconds(5));
		}
	}

	//flush encoder处理残余的码流
	while (1) {
		ret = x264_encoder_encode(pHandle, &pNals, &iNal, NULL, pPic_out);
		if (ret == 0) {
			break;
		}
		for (int j = 0; j < iNal; ++j) {
			fwrite(pNals[j].p_payload, 1, pNals[j].i_payload, fp_dst);
		}
	}
	fclose(fp_dst);

	//释放资源
	//x264_picture_clean(this->pPic_in);
	x264_encoder_close(this->pHandle);
	this->pHandle = NULL;

	free(this->pPic_in);
	free(this->pPic_out);
	free(this->pParam);
	this->stopQue.push(true);
}

void H264EncoderTang::release()
{
	this->bStop = true;

	//等待线程结束
	while (this->stopQue.empty())
	{
		this_thread::sleep_for(chrono::milliseconds(100));
	}
}

shared_ptr<unsigned char> H264EncoderTang::getYuv()
	{
		shared_ptr<unsigned char> yuvBuf(nullptr);
		{
			lock_guard<mutex> lock(this->g_i_mutex);

			while (!this->frameQue.empty())	//取最新帧，且清空。避免帧速过高时处理不过来
			{
				yuvBuf = this->frameQue.back();
				this->frameQue.pop();
			}

		}
		return yuvBuf;
	}
