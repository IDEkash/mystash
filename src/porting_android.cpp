// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2014 celeron55, Perttu Ahola <celeron55@gmail.com>

#ifndef __ANDROID__
#error This file may only be compiled for android!
#endif

#include "util/numeric.h"
#include "porting.h"
#include "porting_android.h"
#include "threading/thread.h"
#include "config.h"
#include "filesys.h"
#include "log.h"
#include "settings.h"

#include <jni.h>
#define SDL_MAIN_HANDLED 1
#include <SDL.h>

#include <sstream>
#include <exception>
#include <cstdlib>

#ifdef GPROF
#include "prof.h"
#endif

extern int main(int argc, char *argv[]);

extern "C" JNIEXPORT void JNICALL
Java_net_minetest_minetest_GameActivity_saveSettings(JNIEnv* env, jobject /* this */) {
	if (!g_settings_path.empty())
		g_settings->updateConfigFile(g_settings_path.c_str());
}

namespace porting {
	// used here:
	void cleanupAndroid();
	std::string getLanguageAndroid();
	bool setSystemPaths(); // used in porting.cpp
}

extern "C" int SDL_Main(int _argc, char *_argv[])
{
	Thread::setName("Main");

	char *argv[] = {strdup(PROJECT_NAME), strdup("--verbose"), nullptr};
	int retval = main(ARRLEN(argv) - 1, argv);
	free(argv[0]);
	free(argv[1]);

	porting::cleanupAndroid();
	infostream << "Shutting down." << std::endl;
	exit(retval);
}

namespace porting {
jobject      activity;
jclass       activityClass;
JavaVM      *javaVM;

JNIEnv *getJNIEnv()
{
	JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	if (env)
		return env;

	if (javaVM) {
		int status = javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
		if (status == JNI_OK)
			return env;
		if (status == JNI_EDETACHED) {
#ifdef __ANDROID__
			if (javaVM->AttachCurrentThread(&env, nullptr) == JNI_OK)
#else
			if (javaVM->AttachCurrentThread((void**)&env, nullptr) == JNI_OK)
#endif
				return env;
		}
	}

	FATAL_ERROR("Could not get JNIEnv");
	return nullptr;
}

void osSpecificInit()
{
	JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
	FATAL_ERROR_IF(!env, "osSpecificInit: Could not get JNIEnv");

	env->GetJavaVM(&javaVM);

	jobject localActivity = (jobject)SDL_AndroidGetActivity();
	activity = env->NewGlobalRef(localActivity);
	env->DeleteLocalRef(localActivity);

	jclass localClass = env->GetObjectClass(activity);
	activityClass = (jclass)env->NewGlobalRef(localClass);
	env->DeleteLocalRef(localClass);

	// Set default language
	auto lang = getLanguageAndroid();
	unsetenv("LANGUAGE");
	setenv("LANG", lang.c_str(), 1);

#ifdef GPROF
	// in the start-up code
	warningstream << "Initializing GPROF profiler" << std::endl;
	monstartup("libluanti.so");
#endif
}

void cleanupAndroid()
{
#ifdef GPROF
	warningstream << "Shutting down GPROF profiler" << std::endl;
	setenv("CPUPROFILE", (path_user + DIR_DELIM + "gmon.out").c_str(), 1);
	moncleanup();
#endif
}

static std::string readJavaString(JNIEnv *env, jstring j_str)
{
	if (!j_str)
		return "";
	// Get string as a UTF-8 C string
	const char *c_str = env->GetStringUTFChars(j_str, nullptr);
	if (!c_str) {
		if (env->ExceptionCheck())
			env->ExceptionClear();
		return "";
	}
	// Save it
	std::string str(c_str);
	// And free the C string
	env->ReleaseStringUTFChars(j_str, c_str);
	return str;
}

bool setSystemPaths()
{
	JNIEnv *env = getJNIEnv();
	// Set user and share paths
	{
		jmethodID getUserDataPath = env->GetMethodID(activityClass,
				"getUserDataPath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getUserDataPath==nullptr,
				"porting::initializePathsAndroid unable to find Java getUserDataPath method");
		jstring result = (jstring)env->CallObjectMethod(activity, getUserDataPath);
		std::string str = readJavaString(env, result);
		if (result)
			env->DeleteLocalRef(result);
		path_user = str;
		path_share = str;
	}

	// Set cache path
	{
		jmethodID getCachePath = env->GetMethodID(activityClass,
				"getCachePath", "()Ljava/lang/String;");
		FATAL_ERROR_IF(getCachePath==nullptr,
				"porting::initializePathsAndroid unable to find Java getCachePath method");
		jstring result = (jstring)env->CallObjectMethod(activity, getCachePath);
		path_cache = readJavaString(env, result);
		if (result)
			env->DeleteLocalRef(result);
	}

	return true;
}

void showTextInputDialog(const std::string &hint, const std::string &current, int editType)
{
	JNIEnv *env = getJNIEnv();
	jmethodID showdialog = env->GetMethodID(activityClass, "showTextInputDialog",
			"(Ljava/lang/String;Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
			"porting::showTextInputDialog unable to find Java showTextInputDialog method");

	jstring jhint         = env->NewStringUTF(hint.c_str());
	jstring jcurrent      = env->NewStringUTF(current.c_str());
	jint    jeditType     = editType;

	env->CallVoidMethod(activity, showdialog,
			jhint, jcurrent, jeditType);

	if (jhint) env->DeleteLocalRef(jhint);
	if (jcurrent) env->DeleteLocalRef(jcurrent);
}

void showComboBoxDialog(const std::string *optionList, s32 listSize, s32 selectedIdx)
{
	JNIEnv *env = getJNIEnv();
	jmethodID showdialog = env->GetMethodID(activityClass, "showSelectionInputDialog",
			"([Ljava/lang/String;I)V");

	FATAL_ERROR_IF(showdialog == nullptr,
			"porting::showComboBoxDialog unable to find Java showSelectionInputDialog method");

	jclass       jStringClass = env->FindClass("java/lang/String");
	jobjectArray jOptionList  = env->NewObjectArray(listSize, jStringClass, NULL);
	jint         jselectedIdx = selectedIdx;

	for (s32 i = 0; i < listSize; i ++) {
		jstring s = env->NewStringUTF(optionList[i].c_str());
		env->SetObjectArrayElement(jOptionList, i, s);
		env->DeleteLocalRef(s);
	}

	env->CallVoidMethod(activity, showdialog, jOptionList,
			jselectedIdx);

	env->DeleteLocalRef(jOptionList);
	env->DeleteLocalRef(jStringClass);
}

void openURIAndroid(const char *url)
{
	JNIEnv *env = getJNIEnv();
	jmethodID url_open = env->GetMethodID(activityClass, "openURI",
		"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
		"porting::openURIAndroid unable to find Java openURI method");

	jstring jurl = env->NewStringUTF(url);
	env->CallVoidMethod(activity, url_open, jurl);
	if (jurl) env->DeleteLocalRef(jurl);
}

void showFilePickerAndroid()
{
	JNIEnv *env = getJNIEnv();

	static jmethodID picker = nullptr;
	if (!picker) {
		picker = env->GetMethodID(activityClass, "showFilePicker", "()V");
		sanity_check(picker != nullptr &&
			"porting::showFilePickerAndroid unable to find Java showFilePicker method");
	}

	env->CallVoidMethod(activity, picker);
}

void shareFileAndroid(const std::string &path)
{
	JNIEnv *env = getJNIEnv();
	jmethodID url_open = env->GetMethodID(activityClass, "shareFile",
			"(Ljava/lang/String;)V");

	FATAL_ERROR_IF(url_open == nullptr,
			"porting::shareFileAndroid unable to find Java shareFile method");

	jstring jurl = env->NewStringUTF(path.c_str());
	env->CallVoidMethod(activity, url_open, jurl);
	if (jurl) env->DeleteLocalRef(jurl);
}

void setPlayingNowNotification(bool show)
{
	JNIEnv *env = getJNIEnv();
	jmethodID play_notification = env->GetMethodID(activityClass,
			"setPlayingNowNotification", "(Z)V");

	FATAL_ERROR_IF(play_notification == nullptr,
			"porting::setPlayingNowNotification unable to find Java setPlayingNowNotification method");

	jboolean jshow = show;
	env->CallVoidMethod(activity, play_notification, jshow);
}

AndroidDialogType getLastInputDialogType()
{
	JNIEnv *env = getJNIEnv();
	jmethodID lastdialogtype = env->GetMethodID(activityClass,
			"getLastDialogType", "()I");

	FATAL_ERROR_IF(lastdialogtype == nullptr,
			"porting::getLastInputDialogType unable to find Java getLastDialogType method");

	int dialogType = env->CallIntMethod(activity, lastdialogtype);
	return static_cast<AndroidDialogType>(dialogType);
}

AndroidDialogState getInputDialogState()
{
	JNIEnv *env = getJNIEnv();
	jmethodID inputdialogstate = env->GetMethodID(activityClass,
			"getInputDialogState", "()I");

	FATAL_ERROR_IF(inputdialogstate == nullptr,
			"porting::getInputDialogState unable to find Java getInputDialogState method");

	int dialogState = env->CallIntMethod(activity, inputdialogstate);
	return static_cast<AndroidDialogState>(dialogState);
}

std::string getInputDialogMessage()
{
	JNIEnv *env = getJNIEnv();
	jmethodID dialogvalue = env->GetMethodID(activityClass,
			"getDialogMessage", "()Ljava/lang/String;");

	FATAL_ERROR_IF(dialogvalue == nullptr,
			"porting::getInputDialogMessage unable to find Java getDialogMessage method");

	jstring result = (jstring)env->CallObjectMethod(activity,
			dialogvalue);
	std::string str = readJavaString(env, result);
	if (result)
		env->DeleteLocalRef(result);
	return str;
}

int getInputDialogSelection()
{
	JNIEnv *env = getJNIEnv();
	jmethodID dialogvalue = env->GetMethodID(activityClass, "getDialogSelection", "()I");

	FATAL_ERROR_IF(dialogvalue == nullptr,
			"porting::getInputDialogSelection unable to find Java getDialogSelection method");

	return env->CallIntMethod(activity, dialogvalue);
}

float getDisplayDensity()
{
	static bool firstrun = true;
	static float value = 0;

	if (firstrun) {
		JNIEnv *env = getJNIEnv();
		jmethodID getDensity = env->GetMethodID(activityClass,
				"getDensity", "()F");

		FATAL_ERROR_IF(getDensity == nullptr,
			"porting::getDisplayDensity unable to find Java getDensity method");

		value = env->CallFloatMethod(activity, getDensity);
		firstrun = false;
	}

	return value;
}

v2u32 getDisplaySize()
{
	static bool firstrun = true;
	static v2u32 retval;

	if (firstrun) {
		JNIEnv *env = getJNIEnv();
		jmethodID getDisplayWidth = env->GetMethodID(activityClass,
				"getDisplayWidth", "()I");

		FATAL_ERROR_IF(getDisplayWidth == nullptr,
			"porting::getDisplayWidth unable to find Java getDisplayWidth method");

		retval.X = env->CallIntMethod(activity,
				getDisplayWidth);

		jmethodID getDisplayHeight = env->GetMethodID(activityClass,
				"getDisplayHeight", "()I");

		FATAL_ERROR_IF(getDisplayHeight == nullptr,
			"porting::getDisplayHeight unable to find Java getDisplayHeight method");

		retval.Y = env->CallIntMethod(activity,
				getDisplayHeight);

		firstrun = false;
	}

	return retval;
}

std::string getLanguageAndroid()
{
	JNIEnv *env = getJNIEnv();
	jmethodID getLanguage = env->GetMethodID(activityClass,
			"getLanguage", "()Ljava/lang/String;");

	FATAL_ERROR_IF(getLanguage == nullptr,
		"porting::getLanguageAndroid unable to find Java getLanguage method");

	jstring result = (jstring)env->CallObjectMethod(activity,
			getLanguage);
	std::string str = readJavaString(env, result);
	if (result)
		env->DeleteLocalRef(result);
	return str;
}

bool hasPhysicalKeyboardAndroid()
{
	JNIEnv *env = getJNIEnv();
	jmethodID hasPhysicalKeyboard = env->GetMethodID(activityClass,
			"hasPhysicalKeyboard", "()Z");

	FATAL_ERROR_IF(hasPhysicalKeyboard == nullptr,
		"porting::hasPhysicalKeyboardAndroid unable to find Java hasPhysicalKeyboard method");

	jboolean result = env->CallBooleanMethod(activity,
			hasPhysicalKeyboard);
	return result;
}

}
