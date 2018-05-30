#include "opencv.hpp"
#include "highgui.hpp"
#include <iostream>
#include <unistd.h>

using namespace cv;
using namespace std;

void videoCap()
{
	VideoCapture usbCap(0);
	if (!usbCap.isOpened())
	{
		cout << "Cannot open the camera" << endl;
		exit(-1);
	}
	int i = 0;
	char imgName[100];
	namedWindow("usb video", WINDOW_AUTOSIZE);
	bool start = false;
	bool stop = false;
	usbCap.set(CAP_PROP_FRAME_HEIGHT, 1080);
	usbCap.set(CAP_PROP_FRAME_WIDTH, 1920);
	sleep(1);
	while (!stop)
	{
		Mat frame;
		usbCap >> frame;
		imshow("usb video", frame);
	}
}
