#include "H264EncoderTang.h"
#include <thread>
#include <time.h>

int main(int argc, char** argv)
{
	H264EncoderTang coder264(640, 480, 25);

	thread th(&H264EncoderTang::mainLoop, std::ref(coder264));
	th.detach();

	VideoCapture cap;
	Mat frame;
	int key;

	cap.open(0);
	if (!cap.isOpened())
		return 0;

	int btime = clock();
	int ltime;
	while (1)
	{
		cap >> frame;
		if (frame.empty())
			break;

		coder264.encodeNewFrame(frame);

		imshow("video", frame);
		key = waitKey(30);//每帧延时20毫秒
		if (key == 27)
		{
			break;
		}
		ltime = clock();
		if(ltime - btime >= 20000)
		{
			break;
		}
	}
	cap.release();
	coder264.release();
}
