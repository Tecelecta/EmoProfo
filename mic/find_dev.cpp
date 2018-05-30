#include <stk/RtAudio.h>
#include <iostream>

using namespace std;

int main()
{
	RtAudio dev;
	cout << "default in :" << dev.getDeviceInfo(dev.getDefaultInputDevice()).name << endl;
	cout << "default out : " << dev.getDeviceInfo(dev.getDefaultOutputDevice()).name << endl;
	for(int i = 0; i < dev.getDeviceCount(); i++)
		cout << "dev " << i << " : " << dev.getDeviceInfo(i).name << endl;
}
