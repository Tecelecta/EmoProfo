/**
 * Technical testing code
 * @author Tecelecta
 * @date 2018.4.14
 */
#define __OS_LINUX__

#include <iostream>
#include <unistd.h>
#include <errno.h>
#include <stk/FileWvIn.h>
#include <stk/FileWvOut.h>
#include <stk/RtWvIn.h>
#include <stk/RtWvOut.h>

//#define RT_OUT
//#define RT_IN

using namespace std;
using namespace stk;

/**
 * @brief usage
 * 输出使用方法
 */
void usage()
{
	cout << "Usage:\n";
	cout << "\t./cam <input file> <output file>\n";
}

int main(int argc, char* argv[])
{
	if(argc != 3)
	{
		usage();
		exit(-1);
	}

	if(access(argv[1],F_OK) == -1)
	{
		cout << "Error: Input file doesn't exists!\n";
		exit(-1);
	}



	uint32_t in_size;
#ifdef RT_IN
	RtWvIn input(1,Stk::sampleRate(), 6, 20);
	in_size = 100000;
#else
	FileWvIn input;
	input.openFile(argv[1]);
	in_size = input.getSize();
	Stk::setSampleRate(input.getFileRate());
#endif

#ifdef RT_OUT
	RtWvOut *output;
	output = new RtWvOut(1, Stk::sampleRate(), 0, RT_BUFFER_SIZE, 20);
#else
	FileWvOut *output;
	output = new FileWvOut(argv[2]);
#endif

	try {
		for(uint32_t i = 0; i < in_size; i++)
		{
			StkFloat frame = input.tick();
			//cout << "frame " << i << " : "<<frame << endl;
			output->tick(frame);
		}
	} catch(StkError &){
		cerr << "Error occurred while processing file!\n";
		exit(-1);
	}
#ifdef RT_OUT
	delete output;
#else
	output->closeFile();
	delete output;
#endif
	return 0;
}

