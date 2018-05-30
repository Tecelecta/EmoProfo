#include "trans.h"
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define BUFF_SZ		1024

//初始化file_list
FileList name_list;

/**
 * @brief init_udp_socket
 * 创建一个udp socket并绑定到系统的一个端口
 * @param port	_in_	端口
 * @return
 */
int init_udp_socket(const int port)
{
	//const char* local_ip = "127.0.0.1";
	int sock;
	struct sockaddr_in addr_param;
	//创建描述符
	while ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
		perror("Cannot open socket ");
		sleep(3);
	}

	memset(&addr_param, 0, sizeof(addr_param));
	addr_param.sin_family = AF_INET;
	addr_param.sin_port = htons(port);
	//inet_pton(AF_INET, local_ip, &addr_param.sin_addr);
	addr_param.sin_addr.s_addr = htonl(INADDR_ANY);

	while( bind(sock, (struct sockaddr* )&addr_param, sizeof(struct sockaddr)) < 0)
	{
		perror("Binding control port failed ");
		sleep(3);
	}

	return sock;
}

/**
 * @brief send_file
 * 向ftp服务器上传文件，使用libcurl
 * @param src		_in_	文件路径
 * @param dest_url	_in_	目的ftp的URL
 * @param curl		_in_	在外层创建好的结构体
 * @return
 */
int send_file(const char* src, const char *dest_url, CURL* curl)
{
	struct stat file_info;
	curl_off_t file_size;
	FILE* fp;
	CURLcode res;

	printf("sending file %s to %s\n",src ,dest_url);
	//检查文件大小
	if(stat(src, &file_info))
	{
		fprintf(stderr, "Unable to retreive size of %s :%s\n",src,strerror(errno));
		return RET_FAILED;
	}
	file_size = (curl_off_t)file_info.st_size;

	//打开文件
	fp = fopen(src, "rb");
	if(fp == NULL)
	{
		perror("Unable to open file for transmission ");
		return RET_FAILED;
	}

	//设置操作
	curl_easy_setopt(curl, CURLOPT_USERPWD, "ftp:ftp");
	curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
	curl_easy_setopt(curl, CURLOPT_URL, dest_url);
	curl_easy_setopt(curl, CURLOPT_UPLOAD, 1);
	curl_easy_setopt(curl, CURLOPT_READDATA, fp);
	curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, file_size);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);

	res = curl_easy_perform(curl);
	if(res != CURLE_OK)
	{
		fprintf(stderr, "perform failed: %s\n",curl_easy_strerror(res));
	}

	fclose(fp);
	return res == CURLE_OK ? RET_SUCCESS : RET_FAILED;
}

/**
 * @brief send_msg
 * 向指定的ip和端口发送一个数据报
 * @param dset_ip		_in_	目的ip
 * @param dest_port		_in_	目的端口
 * @param msg_p			_in_	发送内容
 * @return
 */
int send_msg(int sockfd, const char *dest_ip, const int dest_port, const struct Msg* msg_p)
{
	sockaddr_in dest_addr;
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(dest_port);
	inet_pton(AF_INET, dest_ip, &dest_addr.sin_addr);
	socklen_t dest_len = sizeof(dest_addr);

	size_t data_sz;
	switch(msg_p->msgId)
	{
	case MsgType::MT_OP:
		data_sz = sizeof(CntlMsg);
		break;
	case MsgType::MT_STATUS:
		data_sz = sizeof(StatusMsg);
		break;
	case MsgType::MT_FDBK:
		data_sz = sizeof(FeedBackMsg);
	}
	if(sendto(sockfd, msg_p, data_sz, 0,(struct sockaddr* ) &dest_addr, dest_len) == -1)
	{
		perror("Failed sending data ");
		return RET_FAILED;
	}
	return RET_SUCCESS;
}

/**
 * @brief recv_msg
 * 接收下一个消息
 * @param sockfd		_in_	终端的udp描述符
 * @param src_ip			_out_	源ip
 * @param src_port		_out_	源端口
 * @param msg_buffer	_out_	信息内容缓存
 * @param timesout		_in_	接收超时,以秒为单位
 * @return
 */
int recv_msg(const int sockfd, char* src_ip, int* src_port, struct Msg* msg_buffer, int timeout)
{
	char loc_buffer[BUFF_SZ];
	struct sockaddr_in server_info;
	timeval orig_timeout, next_timeout;

	next_timeout.tv_sec = timeout;
	next_timeout.tv_usec = 0;

	memset(&loc_buffer, 0, sizeof(loc_buffer));
	memset(&orig_timeout, 0, sizeof(orig_timeout));
	socklen_t time_size = sizeof(orig_timeout);
	socklen_t info_len = sizeof(server_info);

	//获取原超时时间
	if(getsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO ,&orig_timeout, &time_size))
	{
		perror("Getting socket option failed ");
		if(errno == EINVAL)fprintf(stderr, "EINVAL: \n");
		return RET_FAILED;
	}
	//设置新超时时间
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &next_timeout, sizeof(next_timeout)))
	{
		perror("Setting time out failed ");
		return RET_FAILED;
	}
	//接受信号
	if(recvfrom(sockfd, &loc_buffer[0], BUFF_SZ -1, 0, (struct sockaddr*) &server_info, &info_len) == -1)
	{
		//没收到信号,返回
		if(errno != EAGAIN && errno != EWOULDBLOCK)
			perror("failed to recv a transmission ");
		return RET_FAILED;
	}
	//设回原超时时间
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &orig_timeout, time_size))
	{
		perror("Setting back timeout failed ");
		return RET_FAILED;
	}

	//装填源信息
	*src_port = ntohs(server_info.sin_port);
	inet_ntop(AF_INET, &server_info.sin_addr, src_ip, info_len);

	//解析并装填Msg
	Msg* msg_p = (Msg*) &loc_buffer;
	switch(msg_p->msgId)
	{
	case MsgType::MT_OP:
		memcpy(msg_buffer, msg_p, sizeof(CntlMsg));
		break;
	case MsgType::MT_STATUS:
		memcpy(msg_buffer, msg_p, sizeof(StatusMsg));
		break;
	case MsgType::MT_FDBK:
		memcpy(msg_buffer, msg_p, sizeof(FeedBackMsg));
	}
	return RET_SUCCESS;
}
