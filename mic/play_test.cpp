// rtsine.cpp STK tutorial program
#define __OS_LINUX__

#include "voicap.h"
#include <cstdlib>
#include <iostream>

using namespace stk;
int main(int argc, char* argv[])
{

	if(argc != 2)
	{
		cout<< "Usage: ./cam <input dir>\n";
		exit(0);
	}
	// Set the global sample rate before creating class instances.

	Stk::setSampleRate( 44100.0 );
	Stk::showWarnings( true );

	AudioCap cap(argv[1], 1000);
	//MicCap cap(6, 4000);

	int total=0, noise=0, sig=0;
	cap.start();

	while(cap.hasNextTick())
	{
		switch(cap.run_tick())
		{
		case 0: noise++; break;
		case 1: sig++; break;
		}
		total++;
	}

	cout<< "total: " << total <<" sig "<<sig<<" noise "<<noise;

  return 0;
}
