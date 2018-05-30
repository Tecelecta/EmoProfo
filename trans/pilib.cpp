#include "trans.h"
#include "Pilib.h"
#include <iostream>
#include <assert.h>

#define SERVER_PORT 16000
#define FB_TIMEO 10
#define FAIL_TRY 3

static int sockfd = -1;

/**
  * 初始化套接字描述符,将就是简单的函数转发
  */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1init
  (JNIEnv * env, jclass cls)
{
	sockfd = init_udp_socket(SERVER_PORT);
	std::cout << "sockfd is " << sockfd << std::endl;
}

/**
  * 启动树莓派,就是发送一个启动控制信号,然后等待确认
  */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1startPi
  (JNIEnv * env , jclass cls, jstring jip, jint jport)
{
	const char* ip = env->GetStringUTFChars(jip, 0);
	const int port = jport;

	assert(sockfd != -1);

	CntlMsg msg_to_send(CTL_START);

	//发送消息
	std::cout << "sending start msg to "<< ip << " : " << port << std::endl;
	int res = send_msg(sockfd, ip, port, &msg_to_send);

	//释放资源
	env->ReleaseStringUTFChars(jip, ip);
	return res;
}

/**
  * 这个和上面差不多,就是发送内容有区别
  */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1stopPi
  (JNIEnv *env , jclass cls, jstring jip, jint jport)
{
	//格式转换
	const char* ip = env->GetStringUTFChars(jip, 0);
	const int port = jport;

	assert(sockfd != -1);

	CntlMsg msg_to_send(CTL_STOP);

	//发送消息
	std::cout << "sending stop msg to "<< ip << " : " << port << std::endl;
	int res = send_msg(sockfd, ip, port, &msg_to_send);

	//释放资源
	env->ReleaseStringUTFChars(jip, ip);
	return res;
}

JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1queryPi
  (JNIEnv * env, jclass cls, jstring jip, jint jport)
{
	const char* ip = env->GetStringUTFChars(jip, 0);
	const int port = jport;

	char* feedback_ip = new char[16];
	int feedback_port = 0;

	assert(sockfd != -1);

	CntlMsg msg_to_send(CTL_STATUS);
	StatusMsg q_res(STAT_OFFLINE);
	FeedBackMsg conf(FB_CONFIRM);
	FeedBackMsg rej(FB_REJECT);

	//尝试发送几次,不成功就返回错误
	int try_count = FAIL_TRY;
	bool tsudzuke = true; //是否继续循环

	while(tsudzuke)
	{
		std::cout << "sending status msg to "<< ip << " : " << port << std::endl;
		send_msg(sockfd, ip, port, &msg_to_send);
		recv_msg(sockfd, feedback_ip, &feedback_port, &q_res, FB_TIMEO);

		if(q_res.status != STAT_OFFLINE)
		{//如果收到消息
			if(0 == strcmp(feedback_ip, ip) && feedback_port == port)
			{
				send_msg(sockfd, ip, port, &conf);
				tsudzuke = false;	//还是需要的机器发送的,确认结束
			}
			else
				send_msg(sockfd, feedback_ip, feedback_port, &rej);//不是需要机器发送的,拒绝信息
		} else if(try_count--) {
			tsudzuke = false;	//计数结束,认为没收到
		}
	}


	env->ReleaseStringUTFChars(jip, ip);
	delete feedback_ip;
	return q_res.status;
}
