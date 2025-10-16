#ifndef _Included_com_dongge0210_renderbridge_NativeBridge
#define _Included_com_dongge0210_renderbridge_NativeBridge
#include <jni.h>
#ifdef __cplusplus
extern "C" {
#endif
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nInit(JNIEnv*, jobject, jint);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nRender(JNIEnv*, jobject, jfloat, jlong);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nResize(JNIEnv*, jobject, jint, jint);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nShutdown(JNIEnv*, jobject);
JNIEXPORT void JNICALL Java_com_dongge0210_renderbridge_NativeBridge_nSetWindowHandle(JNIEnv*, jobject, jlong, jint);
#ifdef __cplusplus
}
#endif
#endif