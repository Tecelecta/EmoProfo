#音视频捕获输入(一)

[TOC]

这一部分的代码主要通过外设完成音视频的捕获和向系统的输入.

- 作者:李鸣一

##基本功能

### 视频的捕获和输入

设计上,视频源有两个:摄像头和拍摄好的视频.鉴于Face++api以及网络性能,我们决定以2~10fps的速率处理连续图片.

经过多方了解和讨论,我决定采用opencv完成处理

- opencv包含统一的接口,可以读取现有视频文件,同时可以从摄像头输入
- opencv支持多种处理方式,可以将输入数据做成视频文件,同时也可以做成连续图片

###音频的捕获和输入

与视频源相同,音频源同样设计了2个:文件和麦克风录制,为了方便处理,我们决定将输入数据处理成多个1min左右的音频文件.

经过意见征集,我们确定了如下几个音频处理工具包:

- STK(Systhesis ToolKit):一个历史悠久的音频信号处理工具包
- CLAM:一个功能强大,广泛运用到各种领域的音频信号处理

经过进一步讨论我们决定首先试验STK,因为器相关特点:

- 源码式的发布模式:使用只需要将该工程代码和自己工程代码一同编译即可,跨平台性强
- 定义好的输入输出类:无论是文件还是设备,该工具包都定义了相关的类
- 简单的音频处理方式:通过tick()函数来对采样点进行提取,处理,输出一系列操作容易理解
- 在包含我们需要功能的基础上,又足够简单

## 试验以及实现

###视频捕获部分

通过观察相关实例,识别出下列需要参与实现的类型:

- VedioCapture类:获取视频源的类,可用的源包括视频文件,图像序列,相机
- Mat类:基本的图像存储类,有如下特点:
  - 实现了自动化的内存管理
  - 重载了赋值操作符和复制构造函数
  - 所有隐式操作都采用了引用传递的方式来实现性能的提高

原则上,我们需要两种视频输入:视频文件和相机.按照设计,我们对图像处理的速度要求不高,而对于两种不同的输入,我们需要采取的策略也是不同的:

- 相机:这个比较容易,因为采样频率通过sleep这种简单的方式都能进行控制
- 视频文件:这个方面就需要从原文件中定长截取相应的帧

初步的试验,首先截取相应的图片,以文件的形式存储,作出如下的类设计:

```c++
//piccap.h
#ifndef PICCAP_H
#define PICCAP_H
/**
 * 视频捕获的相关类型,采用多态设计,方便后期变动
 * @author LI Mingyi
 * piccap.h
 */
#include "opencv.hpp"
#include <string>

using namespace cv;
using namespace std;

/**
 * @brief 公共父类定义操作,包括构造,图片显示,图片保存
 */
class PicCap
{
public:
	virtual ~PicCap();
	virtual Mat* getPic() = 0;
	bool storePic(Mat* img);
protected:
	PicCap();
	//string nextName(); //简单地返回下一个输出文件的文件名应该是什么
	VideoCapture *cap; //cv对象指针
	bool current_stored; //记录当前取出的文件是否存储过,使流操作变得安全
private:
	int proc_cnt; //进度计数,记录现在取到第几张图片,为下一个文件名服务
	string prefix; //输出文件名的前缀,在构造时生成
};

/**
 * @brief The CamCap class, 通过相机截取图片
 */
class CamCap : public PicCap
{
public:
	CamCap(int dev);
	Mat* getPic();
	bool storePic(Mat* img);
private:
	int dev_index; //对应相机的索引号
};

/**
 * @brief The VideoCap class, 通过视频截取图片
 */
class VideoCap : public PicCap
{
public:
	VideoCap(const string& fileName);
	Mat* getPic();
	bool storePic(Mat* img);
private:
	string inputFile; //对应视频文件名
};

#endif // PICCAP_H

```

具体方法实现如下

```c++
//piccap.cpp
#include "piccap.h"
#include <time.h>
#include <unistd.h>

#define FPS 2 //首先实现2帧

using namespace cv;
/**
 * @brief PicCap::PicCap 在这里生成文件文件名
 * 想法是 构造时时间戳_第几张
 */
PicCap::PicCap()
{
	time_t t = time(NULL);
	char t_str[64];
	strftime(t_str, sizeof(t_str), "%Y-%m-%d_%H:%M:%S_", localtime(&t));
	prefix = std::string(t_str);
	current_stored = true;
}

/**
 * @brief PicCap::~PicCap
 * 简单的删除cap
 */
PicCap::~PicCap()
{
	delete cap;
}

/**
 * @brief PicCap::storePic
 * 所有子类都通过这个方法将图片存入文件
 * @param img 图片指针,可能为空,代表图片获取失败
 * @return 存储是否成功,如果存储成功,输入参数指向的对象会被删除
 */
bool PicCap::storePic(Mat *img)
{
	if(!cap)
		return false; //cap没打开,不用玩了
	if(current_stored)
		return false; //图片已经存过了,不用玩了

	imwrite(prefix + to_string(proc_cnt++) + ".jpg", *img);

	delete img; //释放图片占用的空间
	current_stored = true;
	return true;
}

/**
 * @brief CamCap::CamCap 这个子类的构造方法就需要参数
 * @param dev 选中的相机索引
 */
CamCap::CamCap(int dev) : PicCap()
{
	dev_index = dev;
	cap = new VideoCapture(dev);
}

/**
 * @brief CamCap::getPic 取出一张图片
 * 实现帧数控制的方法十分简单粗暴,通过sleep来实现
 * **经过试验证明,上述措施不可行,因为相机会缓存自己捕获的帧
 * @return 图片指针
 */
Mat* CamCap::getPic()
{
	if(!cap)
		return NULL;
	if(!current_stored)
		return NULL;

	//这是修改后的内容 (4.9.2018)
	int fps = cap->get(CAP_PROP_FPS);
	int padding = fps/FPS;
	//到此为止 (4.9.2018)

	Mat* ret = new Mat;
	for(int i = 0; i < padding; i++)
	{
		cap->grab();
	}
	cap->retrieve(*ret);
	current_stored = false;
	/*这个想法是行不通的,需要在这一段时间内连续取帧,即像视频文件一样处理
	 * usleep(1000/FPS * 1000); //就用sleep控制帧数
	 */
	return ret;
}

/**
 * @brief VideoCap::VideoCap
 * @param fileName
 */
VideoCap::VideoCap(const string& fileName) : PicCap()
{
	inputFile = fileName;
	cap = new VideoCapture(inputFile);
}

Mat* VideoCap::getPic()
{
	if(!cap)
		return NULL;
	if(!current_stored)
		return NULL;
	/*
	 * TODO
	 * 通过cap的get方法可以get到视频的FPS:CAP_PROP_FPS
	 * 同时也可以得到视频总共帧的数量:CAP_PROP_FRAME_COUNT
	 * 按帧截取思路如下:
	 *  - 根据视频帧数计算出视频的帧数比需要的帧数快几倍
	 *  - 连续读出倍数张图片,保存其中一张
	 */
	int fps = cap->get(CAP_PROP_FPS);
	int padding = fps/FPS;

	Mat* ret = new Mat();
	for(int i = 0; i < padding; i++)
	{
		cap->grab();
	}
	cap->retrieve(*ret);
	current_stored = false;
	return ret;
}
```

为了测试这个类的可用性,编写了如下测试代码

```c++
//test_main.cpp
#include "piccap.h"
#include "highgui.hpp"
#include <iostream>
#include <string>

using namespace std;
PicCap* cap;

inline void usage()
{

	cout << "Picture capture Experimental test" << endl;
	cout << "\t@Copyright Tecelecta" << endl;
	cout << "\tUsage: ./cam_test [ cam | avi ]" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	if(argc != 2)
		usage();

	string cam = "cam";

	if(cam.compare(argv[1]) == 0)
		cap = new CamCap(0);
	else cap = new VideoCap("/home/tecelecta/Desktop/EmoProfo/cam/Megamind.avi");

	while(1)
	{
		Mat* ptr = cap->getPic();
		imshow(argv[1],*ptr);

		cap->storePic(ptr);
		if(waitKey(10) == 27)
			break;
	}

	return EXIT_SUCCESS;
}
```

运行时发现了问题(见piccap.cpp中的TODO),这可能与opencv的实现有关系,原来想要通过sleep控制fps的方式完全不可行,最终,将视频方法和相机方法进行了统一,都采用了跳帧方式.

## 小结

本部分主要完成了功能的定义,视频捕获部分的实现,音频部分将作为下一个模块来操作.

开发环境:

- Ubuntu 16.04 LTS
- qt creator 3.5.1 & qt 5.5.1
- gcc 5.4.0 (这个版本有点高,后期向arm平台移植的过程中可能需要修改代码)
- opencv 3.4.1

## 改进思路

经过和老师的讨论，我在图像的输入方面有了一些新的想法：

- 使用云相机思想，将图片首先发送至一个专用服务器，这个服务器对图片进行简单处理，之后再由后端完成api的调用
- 在拥有服务器的基础上，添加对多摄像头的支持
  - 将收到的图片先存储到映射到内存的目录中
  - 打开图片，完成处理、存储到永久存储
- 当后端调用API时，只需要提供图像云上的URL，即可完成图像的传递，进一步降低后端的压力

# 音视频捕获输入（二）

[TOC]

## 试验以及实现

### 音频捕获部分

首先来明确我们需要做的工作：

- 打开一个音频文件（从视频文件中分离的），或是麦克风
- 获得文件的比特率，将音频文件截成1min左右的一段，发送至服务器
  - 时间确定的标准是，选择一个比较安静的时刻，防止将说着的话截断

为了这些基本要求，我从stk的API中获得了如下必要对象和函数

#### FileLoop类

# 服务端处理思路实现

鉴于前面提到的改进思路，我们在中间服务器（图片云）上运行一个神经网络识别器，仅仅将人从输入的图片中截下，这样可以大幅度提高表情识别的效率。

## 框架的选择

框架是直接与神经网络系统性能直接相关，这里我们选择了当下性能最高的一款框架：darknet。该框架由纯C实现，第三方依赖少，具有很不错的性能，在其上设计的YOLO系列网络模型更是具有十分优异的性能。在真正投入使用之前，我们首先对这个框架进行一下研究，探索与我们项目的结合方法

