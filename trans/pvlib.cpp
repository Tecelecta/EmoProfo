#include "PVlib.h"
#include "cam/piccap.h"
#include "mic/voicap.h"
#include <iostream>

JNIEXPORT jint JNICALL Java_srv_1work_PVlib_lib_1getPic
  (JNIEnv *env, jclass cls, jstring jsrc, jstring jdest)
{
	const char* src = env->GetStringUTFChars(jsrc, 0);
	const char* dest = env->GetStringUTFChars(jdest, 0);
	jint res_cnt = 0;

	std::cout << "voi cap started...\n";
	std::string file_nm;
	PicCap *cap = new VideoCap(dest, src);
	while(cap->hasNextPic())
	{
		Mat* pic = cap->getPic();
		cap->storePic(pic,file_nm);
		res_cnt++;
	}
	delete cap;
	env->ReleaseStringUTFChars(jsrc, src);
	env->ReleaseStringUTFChars(jdest, dest);

	std::cout << "voi cap fin...\n";
	return res_cnt;
}

JNIEXPORT jint JNICALL Java_srv_1work_PVlib_lib_1getVoi
  (JNIEnv *env, jclass cls, jstring jsrc, jstring jdest)
{
	const char* src = env->GetStringUTFChars(jsrc, 0);
	const char* dest = env->GetStringUTFChars(jdest, 0);
	jint res_cnt = 0;

	std::cout << "voi cap started...\n";
	std::string file_nm;
	AudioCap *cap = new AudioCap(dest, src);
	cap->start();
	while(cap->hasNextTick())
	{
		cap->run_tick(file_nm);
	}
	cap->finish(file_nm);
	delete cap;
	env->ReleaseStringUTFChars(jsrc, src);
	env->ReleaseStringUTFChars(jdest, dest);

	std::cout << "voi cap fin...\n";
	return res_cnt;
}
