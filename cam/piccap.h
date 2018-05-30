#ifndef PICCAP_H
#define PICCAP_H
/**
 * 视频捕获的相关类型,采用多态设计,方便后期变动
 * @author LI Mingyi
 * piccap.h
 */
#include "opencv.hpp"
#include <string>

using namespace std;
using namespace cv;

/**
 * @brief 公共父类定义操作,包括构造,图片显示,图片保存
 */
class PicCap
{
public:
	virtual ~PicCap();
	virtual Mat* getPic() = 0;
	virtual bool hasNextPic() = 0;
	bool storePic(Mat* img, string& file_name);	//还是应该将保存的文件名返回（2018年05月30日）
protected:
	PicCap(const string& path); //这里的path参数,包括下面的所有path参数,务必要输入目录符"/"
	//string nextName(); //简单地返回下一个输出文件的文件名应该是什么
	cv::VideoCapture *cap; //cv对象指针
	bool current_stored; //记录当前取出的文件是否存储过,使流操作变得安全
	private:
	string prefix; //输出文件名的前缀,在构造时生成
	int proc_cnt; //进度计数,记录现在取到第几张图片,为下一个文件名服务
};

/**
 * @brief The CamCap class, 通过相机截取图片
 */
class CamCap : public PicCap
{
public:
	CamCap(const string& path ,int dev);
	bool hasNextPic();
	cv::Mat* getPic();
private:
	int dev_index; //对应相机的索引号
};

/**
 * @brief The VideoCap class, 通过视频截取图片
 */
class VideoCap : public PicCap
{
public:
	VideoCap(const string& path, const string& fileName);
	bool hasNextPic();
	cv::Mat* getPic();
private:
	string inputFile; //对应视频文件名
	/*-----------2018-05-30-----------*/
	uint32_t total_frames;	//视频总帧数
	uint32_t passed_frames;	//已经取出的帧数
};

#endif // PICCAP_H
