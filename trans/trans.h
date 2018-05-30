#ifndef __TRANS__
#define __TRANS__

#include <cstring>
#include <pthread.h>

//网络
#include <arpa/inet.h>
#include <curl/curl.h>
/**
	用于客户端和服务器端文件传输的一些列定义
	为了方便操作多种语言实现的程序之间的通信，我们决定采用ftp服务器方式传送文件
	这样，服务端使用java实现的程序可以方便的与进程通信
*/

//消息接收状态
#define RET_FAILED		-1
#define RET_SUCCESS		0

//消息类型
enum MsgType{
	MT_EMPTY,	//空消息
	MT_OP,		//控制信息
	MT_STATUS,	//状态信息
	MT_FDBK		//反馈信息
};

/**
 * @brief The FeedBack enum
 * 反馈信息，主要作用是保证数据报的可靠性
 * 用于服务端确认收到客户端状态
 */
enum FeedBack{
	FB_CONFIRM,		//确认收到消息
	FB_REJECT		//拒绝消息
};

/**
 * @brief The CltOp enum
 * 控制协议，包含了服务器对客户端的所有控制
 */
enum CltOp{
	CTL_START,		//开始
	CTL_STOP,		//停止
	CTL_STATUS,		//状态查询
};
/**
 * @brief The PiStat enum
 * 终端状态，既是终端传回数据报的内容，又是服务端查询的返回值
 */
enum PiStat{
	STAT_STARTED,
	STAT_STOPPED,
	STAT_OFFLINE
};

/**
 * @brief The Msg struct
 * 这些结构体是信息的数据报载体
 */
struct Msg{
	MsgType msgId;
	Msg(MsgType id = MT_EMPTY):msgId(id){}
};

struct StatusMsg: public Msg{
	PiStat status;
	StatusMsg(PiStat s):Msg(MT_STATUS),status(s){}
};

struct CntlMsg: public Msg{
	CltOp op;
	CntlMsg(CltOp cntl):Msg(MT_OP),op(cntl){}
};

struct FeedBackMsg: public Msg{
	FeedBack fb;
	FeedBackMsg(FeedBack f):Msg(MT_FDBK),fb(f){}
};

/**
 * @brief The FileList class
 * 一个全局的链表，存储文件名，方便一边创建文件一边发送文件
 * 当然，这个类是线程安全的
 */
class FileList
{
public:
	FileList():head(0),last(0)
	{
		pthread_mutex_init(&mutex,NULL);
	}
	~FileList()
	{
		pthread_mutex_lock(&mutex);

		if(head == 0 && last == 0) return;
		while(last != head)
		{
			FileNode* p = last;
			last = last->next;
			delete p;
		}
		delete last;

		pthread_mutex_unlock(&mutex);
		pthread_mutex_destroy(&mutex);
	}

	void put(const char* name)
	{
		FileNode* next = new FileNode(name);
		pthread_mutex_lock(&mutex);

		if(head == 0)
		{
			last = next;
			head = next;
		}
		else
		{
			head->next= next;
			head = next;
		}


		pthread_mutex_unlock(&mutex);
	}

	bool pull(char* buffer, int buf_size)
	{
		if(head == 0 && last == 0) return false;
		pthread_mutex_lock(&mutex);

		FileNode * p = last;
		strncpy(buffer, last->fileName, buf_size);
		last = last->next;
		if(last == 0)
		{
			head = 0;
		}

		pthread_mutex_unlock(&mutex);
		delete p;
		return true;
	}

private:
	class FileNode
	{
	public:
		FileNode(const char* name): next(0)
		{
			fileName = new char[strlen(name)+1];
			strcpy(fileName, name);
		}
		~FileNode()
		{
			delete fileName;
		}
		FileNode* next;
		char* fileName;
	};
	FileNode* head;
	FileNode* last;
	pthread_mutex_t mutex;
};

extern FileList name_list;

//建立端口
int init_udp_socket(const int port);

int send_file(const char* src, const char* dest_url, CURL* curl);
int send_msg(int sockfd, const char* dest_ip, const int dest_port, const struct Msg* msg_p);
int recv_msg(int sockfd, char* src_ip, int* src_port, struct Msg* msg_buffer, int timeout);

#endif
