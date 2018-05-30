//--------------------------------------【程序说明】-------------------------------------------
//		程序说明：《OpenCV3编程入门》OpenCV2版书本配套示例程序07
//		程序描述：使用VideoCapture类调用摄像头读入视频并显示
//		开发测试所用操作系统： Windows 7 64bit
//		开发测试所用IDE版本：Visual Studio 2010
//		开发测试所用OpenCV版本：	2.4.9
//		2014年03月 Created by @浅墨_毛星云
//		2014年11月 Revised by @浅墨_毛星云
//------------------------------------------------------------------------------------------------
//---------------------------------【头文件、命名空间包含部分】----------------------------
//		描述：包含程序所使用的头文件和命名空间
//-------------------------------------------------------------------------------------------------
#include <opencv.hpp>
#include <highgui.hpp>
#include <iostream>

using namespace cv;  
using namespace std;
Mat frame;  //定义一个Mat变量，用于存储每一帧的图像
double time0;
int exposure = -1;
//-----------------------------------【main( )函数】--------------------------------------------
//		描述：控制台应用程序的入口函数，我们的程序从这里开始
//-------------------------------------------------------------------------------------------------

void videoCap();

int main()
{
	videoCap();
}

int _main( )
{  
	//【1】从摄像头读入视频
	VideoCapture capture(0);
	if(false == capture.isOpened())
	{
		return -1;
	}

	exposure = capture.get(CV_CAP_PROP_EXPOSURE);
	//显示曝光值
	cout << ">设置前: 曝光值= " << exposure << endl;
	//设置曝光值
	capture.set(CV_CAP_PROP_EXPOSURE, exposure);
	exposure = capture.get(CV_CAP_PROP_EXPOSURE);
	//显示曝光值
	cout << ">设置后: 曝光值= " << exposure << endl;
	double width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
	//显示尺寸
	cout << ">宽：= " << width << ";  高: =" << height << endl;
	//【2】循环显示每一帧
	while(1)  
	{  
		
		time0 = static_cast<double>(getTickCount( ));//记录起始时间
		capture >> frame;  //读取当前帧
		//若视频播放完成，退出循环
		if (frame.empty())
		{
			break;
		}
		imshow("UVC",frame);  //显示当前帧
		//显示帧率
		cout << ">帧率= " << getTickFrequency() / (getTickCount() - time0) << endl;
	}  
	return 0;     
}
