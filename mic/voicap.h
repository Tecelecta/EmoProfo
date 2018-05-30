#ifndef VOICAP_H
#define VOICAP_H

#define __OS_LINUX__

#include <stk/FileWvOut.h>
#include <stk/FileWvIn.h>
#include <stk/RtWvIn.h>
#include <stk/Stk.h>


/* 与噪音识别有关的数据 */
#define DEFAULT_ENV_FACTOR		1.2
#define DEFAULT_QUIET_THRESH		500
#define DEFAULT_RECALC_RATIO		900
#define DEFAULT_RAISE_RATIO		1500

/*2018年05月12日 添加两个和频率相关的常数*/
#define FREQUANCY_UNL	12 //3.4khz
#define FREQUANCY_UPL	147//300hz

/**
 * @brief The VoiCap class
 * 定义了视频捕获的公共接口，对操作进行简单封装，目的是将输入和输出封装到一个对象里
 */
using namespace std;
using namespace stk;

class VoiCap
{

public:
	VoiCap( const string& path,
			const uint32_t nBuffer		= 200,
			const StkFloat env_factor	= DEFAULT_ENV_FACTOR,
			const uint32_t quiet_thresh = DEFAULT_QUIET_THRESH,
			const uint32_t recalc_ratio	= DEFAULT_RECALC_RATIO,
			const uint32_t raise_ratio	= DEFAULT_RAISE_RATIO);

	virtual ~VoiCap();												//析构，对于父类来说并没有什么内容
	int run_tick(string&);											//采样接口，每次运行，都会从输入源采样一个波形
	virtual bool hasNextTick() = 0;									//子类中必然会定义这个方法，将自己的终止条件暴露给用户
	virtual bool start();											//准备开始录制时的触发条件，初始化输入输出
	virtual bool finish(string&);									//录制结束或者条件触发时，调用这个方法完成输出文件的关闭

protected:
	/* 基本成员变量 */
	string prefix;			//当前会话使用的文件前缀
	uint32_t frame_cnt;		//当前文件中输入的帧数，到一分钟附近文件将被断开
	uint32_t total_cnt;		//当前会话中转移的总帧数
	uint32_t file_cnt;		//本次会话使用的文件个数，形成文件名前缀
	FileWvOut *output;		//输出类对象
	WvIn *input;			//输入类的公共父类指针

	const StkFloat _env_factor;		//环境噪音识别率,振幅在noise_level*env_factor范围内的波形会被识别为环境噪音
	const uint32_t _quiet_thresh;	/* 安静阈值,如果连续的"安静"波数达到这个阈值,那么将暂停输出到文件*/
	const uint32_t _recalc_thresh;	/* 重新计算频率,如果被识别为环境噪音的波形数量达到这个值,
										那么用这些值的平均值作为新的环境噪音等级 */
	const uint32_t _raise_thresh;	/* 提升频率,如果距离上一次被识别为噪音的点的波形数达到这个值,
									那么我们将提升环境噪音等级到这些被认为是有效值的样本点的平均值 */

	/* 输出重定向相关方法 */
	bool need_redirect;				//记录当前是否需要重定向的标识
	int redirect(string&);			//两个子类对输出文件重定向的操作有共同部分，这里就是简单的对重定向方法进行一个集中

	/**
	 * @brief The WaveBuffer class
	 * 在噪音处理上,我们需要一定的措施,这就要求按照周期处理输入样本点,这个类就是辅助我们处理的
	 */
	class WaveBuffer
	{
	public:
		/* 可以内联的构造析构 */
		/**
		 * @brief WaveBuffer
		 * 构造函数,完成所有成员的初始化
		 * @param size
		 */
		inline WaveBuffer(uint32_t size) :
			stat(BufferState::INIT), lastTick(0),
			buf_sz(size), tick_ind(0), dump_ind(0),
			profiled(false), level(0)
		{
			buf = new StkFloat[size];
		}

		/**
		 * @brief ~WaveBuffer
		 * 删除buf即可完成析构
		 */
		inline ~WaveBuffer()
		{
			delete buf;
		}

		int nextWindow(StkFrames &out, WvIn &input);	//从输入文件取出一个周期,放入数组(管他到底是不是一个周期有正有负有极大极小值就行)
		StkFloat getLevel();							//返回上一个周期的振幅(绝对值最大的样本点的绝对值)

	private:
		/*表示当前缓冲区的状态*/
		enum BufferState {
			INIT,			//缓冲区刚刚建立时的状态,会以第一个样本点的符号作为自己上半段的符号
			ABOVE_HALF,		//上半段状态,即波形处于上半周期,输入帧符号变化后转换到下半段状态
			BELLOW_HALF		//下半段状态,波形处于后半周期,输入帧符号变化后,讲缓冲内容导出,进入上半段状态
		} stat;
		StkFloat lastTick;	//上一个样本点的值,用来判断符号变化

		/*缓冲区管理成员*/
		StkFloat* buf;		//缓冲区指针
		uint32_t buf_sz;	//缓冲区大小
		uint32_t tick_ind;	//缓冲区输入指针
		uint32_t dump_ind;	//缓冲区输出指针

		/*用于分析的相关成员*/
		bool profiled;		//上一个周期是否被分析过,如果没被分析过,就拒绝获取下一个周期
		StkFloat level;		//上一个周期的振幅

		/*辅助方法*/

		/**
		 * @brief _put_val
		 * 向环状缓冲区中填充样本,返回缓冲区是否已满
		 * @param val 放入缓冲区中的值
		 * @return 缓冲区是否已满
		 */
		inline bool _put_val(StkFloat val)
		{
			buf[tick_ind++] = val;
			tick_ind %= buf_sz;
			return tick_ind == dump_ind;
		}

		/**
		 * @brief _dump
		 * 重新设置输出数组大小并将缓冲区中的数据按顺序输入
		 * @param out 输出数组
		 */
		inline void _dump(StkFrames &out, int cnt)
		{
			out.resize(cnt,1,0);

			int i = 0;
			while(dump_ind != tick_ind)
			{
				out[i++] = buf[dump_ind++];
				dump_ind %= buf_sz;
			}
		}


	} _buffer;

private:
	/* 环境噪音相关变量 */
	StkFloat noise_level;			//噪音等级,将作为识别有效声音的标准

	StkFloat sum_noise;				//被识别为环噪的波形的加和
	StkFloat sum_sig;				//被连续识别为信号的波形的加和
	uint32_t cont_noise_cnt;		//连续的噪音窗口数,用来判断安静时间
	uint32_t noise_cnt;				//环噪波形计数
	uint32_t sig_cnt;				//连续的信号计数

};

class AudioCap : public VoiCap
{

public:
	AudioCap(	const std::string& path,
				const std::string& fileName,
				const uint32_t nBuffer		= 200,
				const StkFloat env_factor	= DEFAULT_ENV_FACTOR,
				const uint32_t quiet_thresh = DEFAULT_QUIET_THRESH,
				const uint32_t recalc_ratio	= DEFAULT_RECALC_RATIO,
				const uint32_t raise_ratio	= DEFAULT_RAISE_RATIO);
	~AudioCap();
	bool hasNextTick();
	bool start();
	bool finish(string&);


protected:
	int redirect();				//这里需要多做一些工作,统计总共输出的帧数

private:

	FileWvIn *my_input;			//文件输入类
	std::string inputName;		//输入文件名称
};

class MicCap : public VoiCap
{
public:
	MicCap(	const std::string&	path,
			const int		dev,
			const uint32_t	nBuffer				= 200,
			const StkFloat	env_factor	= DEFAULT_ENV_FACTOR,
			const uint32_t	quiet_thresh = DEFAULT_QUIET_THRESH,
			const uint32_t	recalc_ratio	= DEFAULT_RECALC_RATIO,
			const uint32_t	raise_ratio	= DEFAULT_RAISE_RATIO);
	~MicCap();
	bool hasNextTick();
	bool start();
	bool finish(string&);

private:

	RtWvIn *my_input;		//音频设备输入类
	int dev_id;				//音频设备号
	bool started;			//输入是否启动
};

#endif // VOICAP_H
