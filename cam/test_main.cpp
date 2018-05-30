#include "piccap.h"
#include "highgui.hpp"
#include <iostream>
#include <string>

using namespace std;
PicCap* cap;

inline void usage()
{

	cout << "Picture capture Experimental test" << endl;
	cout << "\t@Copyright Tecelecta" << endl;
	cout << "\tUsage: ./cam_test [ cam | avi ]" << endl;
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])
{
	if(argc != 2)
		usage();

	string cam = "cam";

	if(cam.compare(argv[1]) == 0)
		cap = new CamCap(0);
	else cap = new VideoCap("/home/tecelecta/Desktop/EmoProfo/cam/Megamind.avi");

	while(cap->hasNext())
	{
		Mat* ptr = cap->getPic();
		imshow(argv[1],*ptr);

		cap->storePic(ptr);
		if(waitKey(10) == 27)
			break;
	}

	return EXIT_SUCCESS;
}
