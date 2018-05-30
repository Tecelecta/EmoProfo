﻿/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class srv_cntl_Pilib */

#ifndef _Included_srv_cntl_Pilib
#define _Included_srv_cntl_Pilib
#ifdef __cplusplus
extern "C" {
#endif
#undef srv_cntl_Pilib_STAT_STARTED
#define srv_cntl_Pilib_STAT_STARTED 0L
#undef srv_cntl_Pilib_STAT_STOPED
#define srv_cntl_Pilib_STAT_STOPED 1L
#undef srv_cntl_Pilib_STAT_OFFLIINE
#define srv_cntl_Pilib_STAT_OFFLIINE 2L
#undef srv_cntl_Pilib_RET_SUCCESS
#define srv_cntl_Pilib_RET_SUCCESS 0L
#undef srv_cntl_Pilib_RET_FAILED
#define srv_cntl_Pilib_RET_FAILED -1L
/*
 * Class:     srv_cntl_Pilib
 * Method:    lib_init
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1init
  (JNIEnv *, jclass);

/*
 * Class:     srv_cntl_Pilib
 * Method:    lib_startPi
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1startPi
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     srv_cntl_Pilib
 * Method:    lib_stopPi
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1stopPi
  (JNIEnv *, jclass, jstring, jint);

/*
 * Class:     srv_cntl_Pilib
 * Method:    lib_queryPi
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_srv_1cntl_Pilib_lib_1queryPi
  (JNIEnv *, jclass, jstring, jint);

#ifdef __cplusplus
}
#endif
#endif