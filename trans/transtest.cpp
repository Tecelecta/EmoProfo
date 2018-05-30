#include "trans.h"

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		printf("Usage: ./a.out <file-to-send>\n");
		return -1;
	}

	CURL *curl = curl_easy_init();
	char file_url[250];
	printf("file to send : %s\n",argv[1]);
	sprintf(&file_url[0], "ftp://127.0.0.1//%s",argv[1]);
	if(send_file(argv[1],&file_url[0],curl) == RET_FAILED)
	{
		perror("transmission failed ");
	}
	curl_easy_cleanup(curl);
}
