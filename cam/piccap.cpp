#include "piccap.h"
#include "trans/trans.h"
#include <time.h>
#include <unistd.h>

#define FPS 1 //首先实现2帧
//FIXME 改成需要的长宽
#define PIC_HIEGHT 1080
#define PIC_WIDTH  1920

using namespace cv;
using namespace std;
/**
 * @brief PicCap::PicCap 在这里生成文件文件名
 * 想法是 构造时时间戳_第几张
 */
PicCap::PicCap(const string& path)
{
	time_t t = time(NULL);
	char t_str[64];
	strftime(t_str, sizeof(t_str), "%Y-%m-%d_%H:%M:%S_", localtime(&t));
	prefix = path + "pic/" + string(t_str);
	proc_cnt = 0;
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
 * @param	img			_in_	图片指针,可能为空,代表图片获取失败
 * -----------2018-05-30--------------
 * @param file_name 输出参数，决定是否返回
 * @return 存储是否成功,如果存储成功,输入参数指向的对象会被删除
 */
bool PicCap::storePic(Mat *img, string& file_name)
{
	if(!cap)
		return false; //cap没打开,不用玩了
	if(current_stored)
		return false; //图片已经存过了,不用玩了

	bool res = false;
	file_name = prefix + to_string(proc_cnt++) + ".jpg";
	try {
		res = imwrite(file_name, *img);
		/*
		 * 2018年05月15日修改确定的功能
		 * 如果存储没出问题,那就直接把文件加入传输列表
		 */
		//name_list.put(file_name.c_str()); 放弃这种方式（2018-05-30）
	} catch (const cv::Exception& ex) {
		fprintf(stderr, "Exception converting image to PNG format: %s\n", ex.what());
	}

	delete img; //释放图片占用的空间
	current_stored = true;
	//修改后的返回方式（2018-05-30）
	return res;
}

/**
 * @brief CamCap::CamCap 这个子类的构造方法就需要参数
 * @param dev 选中的相机索引
 */
CamCap::CamCap(const string &path, int dev) : PicCap(path)
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

	Mat* ret = new Mat;

	int fps = cap->get(CAP_PROP_FPS);
	int padding = fps/FPS - 1;

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

bool CamCap::hasNextPic() {return true;}

/**
 * @brief VideoCap::VideoCap
 * @param fileName
 */
VideoCap::VideoCap(const string& path, const string& fileName) : PicCap(path)
{
	inputFile = fileName;
	cap = new VideoCapture(inputFile);
	cap->set(CAP_PROP_FRAME_HEIGHT, PIC_HIEGHT);
	cap->set(CAP_PROP_FRAME_WIDTH, PIC_WIDTH);
	total_frames = cap->get(CAP_PROP_FRAME_COUNT);
	passed_frames = 0;
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

	Mat* ret = new Mat();

	int fps = cap->get(CAP_PROP_FPS);
	int padding = fps/FPS - 1;

	for(int i = 0; i < padding && hasNextPic(); i++)
	{
		cap->grab();
		passed_frames++;
	}
	cap->retrieve(*ret);

	current_stored = false;
	return ret;
}

bool VideoCap::hasNextPic()
{
	return passed_frames < total_frames;
}
