#include "voicap.h"
#include <trans/trans.h>
#include <time.h>
#include <unistd.h>

using namespace stk;
using namespace std;

/***********************************************************************
 *
 * VoiCap类方法实现
 *
 ***********************************************************************/
/**
 * @brief VoiCap::VoiCap
 * 父类的成员的初始化：
 *  - 获取了文件前缀
 *  - 输出对象的建立和输出文件
 * @param nBuffer		内部WaveBuffer的大小
 * @param env_factor	环噪因数
 * @param quiet_thresh	安静阈
 * @param recalc_ratio	重算阈
 * @param raise_ratio	提升阈
 */
VoiCap::VoiCap( const std::string& path,
				const uint32_t nBuffer ,
				const StkFloat env_factor,
				const uint32_t quiet_thresh,
				const uint32_t recalc_ratio,
				const uint32_t raise_ratio) :
	frame_cnt(0), file_cnt(0),
	 _env_factor(env_factor), _quiet_thresh(quiet_thresh), _recalc_thresh(recalc_ratio), _raise_thresh(raise_ratio),
	need_redirect(false),_buffer(nBuffer),
	noise_level(0), sum_noise(0), sum_sig(0), cont_noise_cnt(0), noise_cnt(0), sig_cnt(0)
{
	time_t t = time(NULL);
	char t_str[64];
	strftime(t_str, sizeof(t_str), "%Y-%m-%d_%H:%M:%S_", localtime(&t));
	prefix = path + "voi/"+std::string(t_str);
	output = new FileWvOut();
}

/**
 * @brief VoiCap::~VoiCap
 * 释放输出类对象的空间
 */
VoiCap::~VoiCap()
{
	delete output;
}

/**
 * @brief VoiCap::redirect
 * 当输出变更条件触发时，变更输出对象
 * -------------2018-05-30------------
 * @param buf 添加了输出参数，用来将新生成的文件名传出去
 * @return 输出变更是否成功
 */
int VoiCap::redirect(string& buf)
{
	try
	{
	output->closeFile();
	/*2018-05-15修改 在文件切换的时候完成链表更新*/
	buf = prefix + to_string(file_cnt) + ".wav";
	//name_list.put(file_name.c_str()); (2018-05-30：不要了)

	output->openFile(prefix + to_string(++file_cnt), 1, FileWrite::FILE_WAV, Stk::STK_SINT16);
	frame_cnt = 0;
	} catch (StkError &) {
		return -1;
	}
	return 0;
}

/**
 * @brief VoiCap::start
 * 启动输入，这里父类做的仅仅是完成输出文件的打开，子类中可能还需要完成输入源的启动操作
 * @return 操作是否成功
 */
bool VoiCap::start()
{
	try
	{
		output->openFile(prefix + std::to_string(file_cnt), 1, FileWrite::FILE_WAV, Stk::STK_SINT16);
	} catch ( StkError & ) {
		return false;
	}

	return true;
}

/**
 * @brief VoiCap::finish
 * 结束输入，这里父类做的仅仅是关闭输出，子类中完成其他操作
 * @return 是否正确结束,对于本类来说
 */
bool VoiCap::finish(string& last_file_name)
{
	try
	{
		output->closeFile();
		/*2018-05-15修改 现在文件在停止工作的时候也会被加入链表*/
		//string last_file_name = prefix + std::to_string(file_cnt) + ".wav";
		//name_list.put(last_file_name.c_str());
		last_file_name = prefix + std::to_string(file_cnt) + ".wav";
	} catch ( StkError & ) {
		return false;
	}
	return true;
}


/**
 * @brief VoiCap::run_tick
 * 文件输入的倒采样点方法,从一个缓冲区移动到另一个缓冲区,不存在计算量和实时性的问题
 *
 * 我们使用变量noise_level来表示,如果_quiet_thresh个波内没有明显超过噪声等级( > noise_level*_env_factor )的波形,
 * 我们认为接下来进入安静时间并停止采样,停止采样后,第一个超过该值的波形将使采样继续
 *
 * noise_level动态确定:当样本点被视为环境噪声时,我们记录下这个样本点的值,当环境噪音数量累计达到_recalc_ratio的时候,
 * 我们用累计值的平均值作为修正后的环噪等级
 *---------------------------------------2018-05-30-------------------------------------------------
 * @param file_name_maybe 可能产生的新文件名，是否真的赋过值由返回值决定
 * @return 0 没有文件 1有文件 -1发生错误
 */
int VoiCap::run_tick(string& file_name_maybe)
{
	StkFrames frames;
	StkFloat level;
	int fcnt = _buffer.nextWindow(frames, *input);

	level = _buffer.getLevel();
	total_cnt += fcnt;

	if(level < noise_level*_env_factor || fcnt < FREQUANCY_UNL || fcnt > FREQUANCY_UPL)//多进行一次频率的判断
	{
		//环噪相关计数增加
		sum_noise += level;
		noise_cnt++;
		cont_noise_cnt++;

		//信号相关计数清0
		sum_sig = 0.0;
		sig_cnt = 0;

		//如果达到重算阈
		if(noise_cnt == _recalc_thresh)
		{
			//重新计算环噪
			noise_level = sum_noise / _recalc_thresh;
			/*Debug*/
			//printf("new noise level: %f\n",noise_level);
			//计数清0
			noise_cnt = 0;
			cont_noise_cnt = 0;
			sum_noise = .0;
		} else if(noise_cnt >= _quiet_thresh) {
			//停止采样,如果时机合适,进行重定向
			if(need_redirect)
			{
				cout << "redircecting output file " << file_cnt << endl;
				need_redirect = false;
				return redirect(file_name_maybe);
			}
		}
	} else {
		//信号相关计数增加,环噪相关数据就不用清0了??
		//2018年05月12日 不用清零才怪!!这里属于功能复用不合理:重新计算noise_level要求不清0,判定quiet_thresh要求清零,办法就是再加一个成员
		sum_sig += level;
		sig_cnt++;
		cont_noise_cnt = 0;
		if(sig_cnt == _raise_thresh)
		{
			//达到提升阈,重新计算
			noise_level = sum_sig / _raise_thresh;
			printf("new noise level: %f\n",noise_level);
			//计数清0
			sig_cnt = 0;
			sum_sig = .0;
		}
	}

	frame_cnt += fcnt;
	output->tick(frames);
	/*debug*/
	//cout << "time: " << total_cnt/Stk::sampleRate() << " level: " << level << " noise_level: " << noise_level <<endl;
	if( frame_cnt >= 50 * (uint32_t) Stk::sampleRate() )
	{
		need_redirect = true;
	}
	return 0;
}


	/*******************************************************************
	 *
	 * VoiCap::WaveBuffer 子类方法实现
	 *
	 *******************************************************************/

#define SAME_SIDE(a,b) ( (a) < 0 && (b) > 0 ) || ( (a) > 0 && (b) < 0 )
#define ABS(a) ( (a)>0 ? (a) : -(a) )

/**
 * @brief VoiCap::WaveBuffer::nextWave
 * 从输出中取出一个波形,并返回波形的振幅
 * @param out 输出数组
 * @param input 输入对象
 * @return 取出波形的长度,-1表明没有按顺序操作所以没取出来
 */
int VoiCap::WaveBuffer::nextWindow(StkFrames &out, WvIn &input)
{
	int cnt = 0;	//用来记录这个周期的帧数
	if(stat == BufferState::INIT)	//如果是初始化阶段,那么就取一个样本点进来,完成初始化
	{
		lastTick = input.tick();
		stat = WaveBuffer::ABOVE_HALF;
		profiled = true;
		while(lastTick == 0)
		{
			lastTick = input.tick();
		}
	}
	//还没有对上一个周期进行过分析,直接返回
	if(!profiled)
	{
		out.resize(0,0,0);
		return -1;
	}
	//将上一个周期完成时取出的的样本点(符号变化的点)存入缓冲区,作为本周期第一个样本点
	_put_val(lastTick);
	cnt++;

	//循环获取样本点
	StkFloat next_tick;
	while(1)
	{
		next_tick = input.tick();

		//更新振幅
		if(level < ABS(next_tick))
			level = ABS(next_tick);

		//判断是否符号发生变化
		if(SAME_SIDE(lastTick, next_tick))
		{
			//发生变化,完成状态转换
			if(stat == WaveBuffer::ABOVE_HALF)
			{
				stat = WaveBuffer::BELLOW_HALF;
			} else {
				stat = WaveBuffer::ABOVE_HALF;
				profiled = false;
				break;
			}
		}

		//将采集的帧放入缓冲区,并报告缓冲区已满的情况
		if(_put_val(next_tick))
		{
			/* Debug */
			//std::cout << "Warning: buffer overrun at "+  +"!\n";
//			static int cnt = 0;
//			std::string temp_name = "overrun";
//			temp_name += cnt+48;
//			temp_name += ".wav";
//			FileWvOut tout(temp_name);
//			for (uint32_t i = tick_ind; i != tick_ind -1 ; i++)
//			{
//				i %= buf_sz;
//				tout.tick(buf[i]);
//			}
//			cout << endl;
			cnt++;
			break;
		}
		cnt++;
		lastTick = next_tick;
	}
	lastTick = next_tick;

	//向缓冲区中写入
	_dump(out, cnt);

	return cnt;
}

/**
 * @brief VoiCap::WaveBuffer::getLevel
 * 返回上一个波形的振幅,并清空振幅,方便下一个波形的统计
 * @return 上一个波形的振幅
 */
StkFloat VoiCap::WaveBuffer::getLevel()
{
	StkFloat ret = level;
	level = 0;
	profiled = true;
	return ret;
}

/***********************************************************************
 *
 * AudioCap 类方法实现
 *
 ***********************************************************************/
/**
 * @brief AudioCap::AudioCap
 * 初始化这个类中的成员变量
 * @param fileName 输入文件的文件名
 * 以下参数见父类构造函数说明
 * @param nBuffer
 * @param env_factor
 * @param quiet_thresh
 * @param recalc_ratio
 * @param raise_ratio
 */
AudioCap::AudioCap(const std::string& path,
				   const std::string& fileName,
				   const uint32_t nBuffer,
				   const StkFloat env_factor,
				   const uint32_t quiet_thresh,
				   const uint32_t recalc_ratio,
				   const uint32_t raise_ratio) :
	VoiCap(path, nBuffer, env_factor, quiet_thresh, recalc_ratio, raise_ratio), inputName(fileName)
{
	input = my_input = new FileWvIn();
}

/**
 * @brief AudioCap::~AudioCap
 * 析构函数,删除掉在子类方法中申请的空间
 */
AudioCap::~AudioCap()
{
	delete my_input;
}

/**
 * @brief AudioCap::start
 * 文件输入类的start方法,打开输入文件即可
 * @return start流程是否成功结束
 */
bool AudioCap::start()
{
	try
	{
		my_input->openFile(inputName);
	} catch ( StkError & ) {
		return false;
	}
	return VoiCap::start();
}

/**
 * @brief AudioCap::finish
 * 文件输入类的finish方法,关闭输入文件
 * @return finish流程是否成功结束
 */
bool AudioCap::finish(string& last_file_name)
{
	try
	{
		my_input->closeFile();
	} catch ( StkError & ) {
		return false;
	}
	return VoiCap::finish(last_file_name);
}

/**
 * @brief AudioCap::hasNextTick
 * 对文件输入类来说,想要知道下一帧是否存在,只需要检查一下文件大小和当前已经获得的帧数就可以了
 * @return 是否还有下一帧
 */
bool AudioCap::hasNextTick()
{
	uint32_t tf = my_input->getSize();
	return total_cnt < tf;
}

/***********************************************************************
 *
 * MicCap 类方法实现
 *
 ***********************************************************************/

/**
 * @brief MicCap::MicCap
 * @param dev
 * @param nBuffer
 * @param env_factor
 * @param quiet_thresh
 * @param recalc_ratio
 * @param raise_ratio
 */
MicCap::MicCap(const std::string& path,
			   const int dev,
			   const uint32_t nBuffer,
			   const StkFloat env_factor,
			   const uint32_t quiet_thresh,
			   const uint32_t recalc_ratio,
			   const uint32_t raise_ratio) :
	VoiCap(path, nBuffer, env_factor, quiet_thresh, recalc_ratio, raise_ratio), dev_id(dev)
{
	input = my_input = new RtWvIn(1, Stk::sampleRate(), dev);
}

/**
 * @brief MicCap::~MicCap
 * 析构函数,删除my_input对象
 */
MicCap::~MicCap()
{
	delete my_input;
}

/**
 * @brief MicCap::start
 * 启动实时输入流,设备开始向缓冲区中传送数据
 * @return 启动是否成功
 */
bool MicCap::start()
{
	my_input->start();
	started = true;
	return VoiCap::start();
}

/**
 * @brief MicCap::finish
 * 停止设备输入,设备会停止向缓冲区输送数据
 * @return 停止是否成功
 */
bool MicCap::finish(string& last_file_name)
{
	my_input->stop();
	started = false;
	return VoiCap::finish(last_file_name);
}

/**
 * @brief MicCap::hasNextTick
 * 对于实时输入来说任何时候都有下一帧,想要停止,必须通过外界干涉
 * @return
 */
bool MicCap::hasNextTick()
{
	return started;
}




