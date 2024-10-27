#include <fcntl.h>
#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "android/log.h"
#include <signal.h>
#include <string.h>

#define LOG_TAG "MyTag"
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)

int dv, open_value;
#define MAX_BUTTON 9
unsigned char quit = 0;
char str_tmp[33];

//FPGA_DOT
unsigned char fpga_number[11][10] = {
	{0x3e,0x7f,0x63,0x73,0x73,0x6f,0x67,0x63,0x7f,0x3e}, // 0
	{0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e}, // 1
	{0x7e,0x7f,0x03,0x03,0x3f,0x7e,0x60,0x60,0x7f,0x7f}, // 2
	{0xfe,0x7f,0x03,0x03,0x7f,0x7f,0x03,0x03,0x7f,0x7e}, // 3
	{0x66,0x66,0x66,0x66,0x66,0x66,0x7f,0x7f,0x06,0x06}, // 4
	{0x7f,0x7f,0x60,0x60,0x7e,0x7f,0x03,0x03,0x7f,0x7e}, // 5
	{0x60,0x60,0x60,0x60,0x7e,0x7f,0x63,0x63,0x7f,0x3e}, // 6
	{0x7f,0x7f,0x63,0x63,0x03,0x03,0x03,0x03,0x03,0x03}, // 7
	{0x3e,0x7f,0x63,0x63,0x7f,0x7f,0x63,0x63,0x7f,0x3e}, // 8
	{0x3e,0x7f,0x63,0x63,0x7f,0x3f,0x03,0x03,0x03,0x03}, // 9
	{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}  //blank
};

unsigned char fpga_set_full[10] = {
	// memset(array,0x7e,sizeof(array));
	0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f
};

unsigned char fpga_set_blank[10] = {
	// memset(array,0x00,sizeof(array));
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    openSwitch
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_openSwitch
  (JNIEnv *env, jobject this){
	if(dv)
		return -1;
	int dev = open("/dev/drivers", O_RDWR);
	if (dev<0){
			LOGV("Device Open Error\n");
			close(dev);
			return -1;
	}
	LOGV("openSwitch returned: %d" , dev);
	dv = dev;
	return dev;
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    readSwitch
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_readSwitch
  (JNIEnv * env, jobject this, jint value){
	jclass thisClass = (*env)->GetObjectClass(env, this);
	jfieldID _field_id_switch_value = (*env)->GetFieldID(env,
			thisClass, "switch_value", "I");

	unsigned char push_sw_buff[9];
	int i = 0;
	int res = read(value, &push_sw_buff, sizeof(push_sw_buff));

	for(i=0;i<9;i++) {
		if(push_sw_buff[i] != 48 && push_sw_buff[i])
			break;
		LOGV("%d: %d ", i, push_sw_buff[i]);
	}
	LOGV("\n");
	(*env)->SetIntField(env, thisClass,
			_field_id_switch_value, i);

	return 0;
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    closeSwitch
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_closeSwitch
  (JNIEnv *env, jobject this){

	return 0;
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    openDot
 * Signature: (I)I
 */

JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_openDot
  (JNIEnv *env, jobject this, jint value){
	int fd =  open("/dev/fpga_dot", O_WRONLY);
	LOGV("openDot value : %d\n", value);

	if(value == -1)
		write(fd, fpga_set_blank, strlen(fpga_set_blank));
	else{
		int str_size = sizeof(fpga_number[value]);
		write(fd,fpga_number[value],str_size);
	}
	close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    closeDot
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_closeDot
  (JNIEnv *env, jobject this, jint fd){

}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    openFND
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_openFND
  (JNIEnv *env, jobject this, jstring str){
	int fd =  open("/dev/fpga_fnd", O_RDWR);
	const char *nativeString = (*env)->GetStringUTFChars(env, str, 0);
	int str_size = strlen(nativeString);
	char fnd_str[5];

	int i = 0;

	 if (strcmp(nativeString, "0000") == 0) {
	        // If the input is "0000", set all to '\0'
	        memset(fnd_str, '\0', 4);
	    } else {
	        for (i = 0; i < 4; i++) {
	        	fnd_str[i] = nativeString[i];
	        }
	    }
	 for(i=0;i<str_size;i++)
	     {
	         if((nativeString[i]<0x30)||(nativeString[i])>0x39) {
	             printf("Error! Invalid Value!\n");
	             return -1;
	         }
	         fnd_str[i]=nativeString[i]-0x30;
	     }

	write(fd,fnd_str,4);
	close(fd);

	(*env)->ReleaseStringUTFChars(env, str, nativeString);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    openLCD
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_org_example_ndk_CSE4116_1Final_openLCD
  (JNIEnv *env, jobject this, jstring str){
	int fd =  open("/dev/fpga_text_lcd", O_WRONLY);
	const char *nativeString = (*env)->GetStringUTFChars(env, str, 0);
	int str_size = strlen(nativeString);
	int i = 0;

//	for(i = 0; i < str_size; i++)
//		str_tmp[i] = nativeString[i];
//	memset(str_tmp+str_size, ' ', 32-str_size);
	LOGV("str_tmp in openLCD : %s\n", str_tmp);
	write(fd,nativeString,str_size);
	close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    writeLCDFirstLine
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_writeLCDFirstLine
  (JNIEnv *env, jobject this, jstring str){
	int fd =  open("/dev/fpga_text_lcd", O_WRONLY);
	const char *nativeString = (*env)->GetStringUTFChars(env, str, 0);
	int str_size = strlen(nativeString);


	int i = 0;

	for(i = 0; i < 32; i++)
		str_tmp[i] = ' ';
	for(i = 0; i < str_size; i++)
		str_tmp[i] = nativeString[i];
	write(fd,str_tmp, 32);

	close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    writeLCDSecondLine
 * Signature: (FFFF)V
 */
JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_writeLCDSecondLine
  (JNIEnv *env, jobject this, jint mode, jint x1, jint y1, jint x2, jint y2){
		int fd =  open("/dev/fpga_text_lcd", O_WRONLY);
		char lcd_tmp[16];

		LOGV("str_tmp before : %s\n", str_tmp);
		int i = 0;
		sprintf(lcd_tmp, "%03d %03d %03d %03d ", x1, y1, x2, y2);
		LOGV("str_tmp after : %s\n", lcd_tmp);

		if(mode == 0){
			char lcd_first[16] = "Rectangle       ";
			memcpy(&str_tmp[0], lcd_first, 16);
		}
		else if(mode == 1){
					char lcd_first[16] = "Pen             ";
					memcpy(&str_tmp[0], lcd_first, 16);
				}
		if(mode == 2){
					char lcd_first[16] = "Oval            ";
					memcpy(&str_tmp[0], lcd_first, 16);
				}
		if(mode == 3){
					char lcd_first[16] = "Eraser          ";
					memcpy(&str_tmp[0], lcd_first, 16);
				}
		memcpy(&str_tmp[16], lcd_tmp, 16);
		str_tmp[33] = '\0';
		write(fd,str_tmp, 32);
		LOGV("%s\n", str_tmp);
		close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    startPushSwitch
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_startPushSwitch
  (JNIEnv *env, jobject this){
	int i;
	int dev;
	int buff_size;
	jclass switch_class;
	jmethodID methodID;

	unsigned char push_sw_buff[MAX_BUTTON];

	dev = open("/dev/fpga_push_switch", O_RDWR);

	if (dev < 0) {
			printf("Device Open Error\n");
			close(dev);
			return;
	}
	buff_size = sizeof(push_sw_buff);
	switch_class = (*env)->GetObjectClass(env, this);
	methodID = (*env)->GetMethodID(env, switch_class, "getSwitchValues", "([I)V");

	while(!quit){
		usleep(400000);
				read(dev, &push_sw_buff, buff_size);

				jintArray values = (*env)->NewIntArray(env, MAX_BUTTON);
				jint *valuesBody = (*env)->GetIntArrayElements(env, values, 0);

				for (i = 0; i < MAX_BUTTON; i++) {
					valuesBody[i] = push_sw_buff[i];
				}

				(*env)->ReleaseIntArrayElements(env, values, valuesBody, 0);

				// Call the Java callback method
				(*env)->CallVoidMethod(env, this, methodID, values);

				(*env)->DeleteLocalRef(env, values);
	}
}

JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_closeLCD
  (JNIEnv *env, jobject this){
	int fd = open("/dev/fpga_text_lcd", O_WRONLY);
	char lcd_tmp[33];
	memset(lcd_tmp, ' ', sizeof(lcd_tmp));
	write(fd, lcd_tmp, sizeof(lcd_tmp));
	close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    closeFND
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_closeFND
(JNIEnv *env, jobject this){
	int fd =  open("/dev/fpga_fnd", O_RDWR);
	char fnd_tmp[5];
	memset(fnd_tmp, '0', sizeof(fnd_tmp));
	int i = 0;
	for(i = 0; i < 4; i++)
		fnd_tmp[i] -= 0x30;
	write(fd, fnd_tmp, sizeof(fnd_tmp));
	close(fd);
}

/*
 * Class:     org_example_ndk_CSE4116_Final
 * Method:    closeDOT
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_example_ndk_CSE4116_1Final_closeDOT
(JNIEnv *env, jobject this){
	int fd =  open("/dev/fpga_dot", O_WRONLY);
	char blank_str[10];
	memset(blank_str, 0x00, sizeof(blank_str));
	write(fd, blank_str, sizeof(blank_str));
	close(fd);
	LOGV("closeDOT called\n");
}
