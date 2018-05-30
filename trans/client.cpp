//系统调用
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <cstring>

#include <pthread.h>
#include <semaphore.h>

#include <stk/Stk.h>

//自定义
#include "cam/piccap.h"
#include "mic/voicap.h"
#include "trans.h"

//socket基本参数
#define CNTL_PORT	8000
#define MAX_PATH		250
#define FB_TIMEO	10

//这两个东西是服务端和终端相互认证用的，具体生成方法待定

//char* svr_port;
//char* key;

//表示当前状态
PiStat cur_status;
char cur_master[16];

struct func_args{
	sem_t start_sem;
	pthread_t tcb;
} worker[3];

typedef void* (*thread_func_t)(void*);

using namespace std;

/**
 * @brief function_thread
 * 完成主要录制功能的函数，同时负责向服务器发送数据
 * @param raw_args
 * @return
 */
static void* sender_thread(void* raw_args)
{

	func_args* args = (func_args*) raw_args;
	char ftp_url[MAX_PATH];

	//工作循环
	while(1)
	{
		char file_name_buffer[MAX_PATH];
		sem_wait(&args->start_sem);
		CURL* curl = curl_easy_init();
		while(cur_status == STAT_STARTED)
		{
			sleep(1);
			while(name_list.pull(&file_name_buffer[0], MAX_PATH))
			{
				printf("try to send file %s to %s\n",&file_name_buffer[0],&cur_master[0]);
				usleep(100000);
				sprintf(&ftp_url[0], "ftp://%s/%s", &cur_master[0], &file_name_buffer[0]);
				send_file(&file_name_buffer[0], &ftp_url[0], curl);
				memset(&file_name_buffer[0], 0, sizeof(file_name_buffer));
			}
		}
		curl_easy_cleanup(curl);
	}
}

/**
 * @brief voi_thread
 * 声音录制线程
 * @param raw_args
 * @return
 */
static void* voi_thread(void* raw_args)
{
	func_args* args = (func_args*) raw_args;
	MicCap* cap;
	string file_nm;
	while(1)
	{
		sem_wait(&args->start_sem);
		printf("voi cap started...\n");
		cap = new MicCap("",0);
		cap->start();
		while(cur_status == STAT_STARTED)
		{

			if(cap->run_tick(file_nm) == 1)
			{
				name_list.put(file_nm.c_str());
			}
		}
		cap->finish(file_nm);
		name_list.put(file_nm.c_str());
		delete cap;
		printf("voi cap fin...\n");
	}
}

/**
 * @brief pic_thread
 * 图片截取线程
 * @param raw_args
 * @return
 */
static void* pic_thread(void* raw_args)
{
	func_args* args = (func_args*) raw_args;
	CamCap* cap;
	string file_nm;
	while(1)
	{
		sem_wait(&args->start_sem);
		printf("pic cap started...\n");
		cap = new CamCap("",0);
		while(cur_status == STAT_STARTED)
		{
			Mat* pic = cap->getPic();
			if(cap->storePic(pic,file_nm))
			{
				name_list.put(file_nm.c_str());
			}
		}
		delete cap;
		printf("pic cap fin...\n");
	}
}

thread_func_t funcs[] = {sender_thread, voi_thread, pic_thread};
/**
 * @brief main
 * 设计上，我们不想用这个函数干太复杂的事情，首先，客户端一旦上线，那就估摸着不会再下线了，
 * 所以这里做一个不退出的函数，并且访问网络，不断等待,接受web端控制
 * 工作流程：
 * - 建立udp端口，等待服务端控制信号
 * - 接收到信号，启动线程开始录制工作，等待服务端终止信号
 * - 停止线程（不管通过什么方式反正就是让线程停下来）,等待下一个连接建立信号
 * 线程需要完成的工作定义在function_thread中
 * @param argc
 * @param argv
 * @return
 */
int main()
{
	int cntl_sock;
	int src_port;		//消息源port缓冲
	char src_ip[16];	//消息源ip缓冲

	stk::Stk::setSampleRate( 44100.0 );
	cur_status = STAT_STOPPED;

	cntl_sock = init_udp_socket(CNTL_PORT);

	//初始化工作线程
	for(int i = 0; i < 3; i++)
	{
		sem_init(&worker[i].start_sem, 0, 0);
		pthread_create(&worker[i].tcb, NULL, funcs[i], &worker[i]);
	}

	printf("initialization complete...\n");
	Msg *next_msg = new FeedBackMsg(FB_CONFIRM);	//这里是哪个都行，因为3个实际msg结构都是一样大的
	bool wait_feedback = false;
	next_msg->msgId = MT_EMPTY;
	while(1)
	{
		recv_msg(cntl_sock, &src_ip[0], &src_port, next_msg, FB_TIMEO);

		switch(next_msg->msgId)
		{
		case MT_OP:
		{
			//接收到操作符
			CntlMsg *real_msg = (CntlMsg*) next_msg;
			if(real_msg->op == CTL_START) {
				//接收到启动操作符
				printf("starting signal received...\n");
				if(cur_status == STAT_STARTED)
				{
					printf("started, reject request...\n");
					struct FeedBackMsg msg_to_send(FB_REJECT);
					send_msg(cntl_sock, &src_ip[0], src_port, &msg_to_send);
					break;
				}
				printf("start confirmed...\n");
				cur_status = STAT_STARTED;
				printf("Setting master as: %s...\n",src_ip);
				strcpy(cur_master, src_ip);
				printf("Now, cur_master = %s\n",cur_master);

				for(int i = 0; i < 3; i++)
					sem_post(&worker[i].start_sem);
			} else if(real_msg->op == CTL_STOP) {
				//接收到停止操作符
				printf("Stop signal reseived...\n");
				cur_status = STAT_STOPPED;
				//memset(&cur_master, 0, sizeof(cur_master));
			} else if(real_msg->op == CTL_STATUS){
				//接收到状态查询操作符
				printf("status query received...\n");
				wait_feedback = true;
			}
		}break;
		case MT_FDBK:
		{
			//接收到了反馈信息
			wait_feedback = false;
		}break;
		default:;//空消息,啥也不做
		}

		if(wait_feedback)
		{
			printf("sending signal...\n");
			struct StatusMsg msg_to_send(cur_status);
			send_msg(cntl_sock, src_ip, src_port, &msg_to_send);
		}
		next_msg->msgId = MT_EMPTY;
	}
}
