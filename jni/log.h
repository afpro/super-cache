#ifndef SC_LOG_H
#define SC_LOG_H

#ifdef SC_NO_LOG
#define LOGV(...)
#define LOGD(...)
#define LOGI(...)
#define LOGW(...)
#define LOGE(...)
#else
#include <android/log.h>
#ifndef SC_TAG
#define SC_TAG "sc"
#endif
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, SC_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, SC_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, SC_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, SC_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, SC_TAG, __VA_ARGS__)
#endif

#endif