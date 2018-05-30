#include "trans.h"
#include <stdio.h>
#include <unistd.h>
int main()
{
	int sock = init_udp_socket(10086);
	char term_ip[16];
	int term_port;

	printf("allocated socket: %d\n", sock);
	CntlMsg stat_msg(CTL_STATUS);
	FeedBackMsg fdbk_msg(FB_CONFIRM);
	StatusMsg state_rcv(STAT_OFFLINE);
	printf("Sending status query...\n");
	send_msg(sock, "127.0.0.1", 8000, &stat_msg);
	printf("waiting query res...\n");
	recv_msg(sock, &term_ip[0], &term_port, &state_rcv, 10);
	printf("Terminal status is :%d\n", int(state_rcv.status));
	send_msg(sock, "127.0.0.1", 8000, &fdbk_msg);
	sleep(3);

	CntlMsg start_msg(CTL_START);
	printf("sending start signal...\n");
	send_msg(sock, "127.0.0.1", 8000, &start_msg);

	sleep(30);

	CntlMsg stop_msg(CTL_STOP);
	printf("Sending stop signal...\n");
	send_msg(sock, "127.0.0.1", 8000, &stop_msg);

	close(sock);
}
