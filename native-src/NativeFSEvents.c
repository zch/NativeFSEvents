//
//  NativeFSEvents.c
//  
//
//  Created by Jonatan Kronqvist on 16.12.2011.
//  Copyright 2011 n/a. All rights reserved.
//

#include <stdio.h>
#include "FSEventHandler.h"
#include "org_vaadin_jonatan_nativefsevents_NativeFSEvents.h"

static JavaVM *jvm;
static jclass gClass = NULL;


void fs_callback(
                 ConstFSEventStreamRef streamRef,
                 void *clientCallBackInfo,
                 size_t numEvents,
                 void *eventPaths,
                 const FSEventStreamEventFlags eventFlags[],
                 const FSEventStreamEventId eventIds[])
{
    int i;
    char **paths = eventPaths;

    JNIEnv *env = NULL;

    (*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL);
    jmethodID java_callback = (*env)->GetStaticMethodID(env, gClass, "eventCallback", "(Ljava/lang/String;)V");
    for (i=0; i<numEvents; i++) {
        jstring path = (*env)->NewStringUTF(env, paths[i]);
        (*env)->CallStaticVoidMethod(env, gClass, java_callback, path);
    }
    (*jvm)->DetachCurrentThread(jvm);
}

static int num_monitor_runloops = 0;
static CFRunLoopRef *monitor_runloops;

JNIEXPORT jlong JNICALL Java_org_vaadin_jonatan_nativefsevents_NativeFSEvents_monitor(JNIEnv *env, jclass class, jstring path)
{
    if (num_monitor_runloops >= 1024) {
        return -1;
    }
    
    const char *cpaths = (*env)->GetStringUTFChars(env, path, NULL);
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        FSEventStreamRef streamRef = monitor_path(cpaths, &fs_callback);
        monitor_runloops[num_monitor_runloops] = CFRunLoopGetCurrent();
        CFRunLoopRun();
        unmonitor(streamRef);
    });
    while (monitor_runloops[num_monitor_runloops] == NULL) {
        sleep(1);
    }
    jlong ix = num_monitor_runloops;
    num_monitor_runloops++;
    return ix;
}

JNIEXPORT void JNICALL Java_org_vaadin_jonatan_nativefsevents_NativeFSEvents_unmonitor(JNIEnv *env, jclass class, jlong monitorId)
{
    if (monitor_runloops != NULL && monitor_runloops[monitorId] != NULL) {
        CFRunLoopStop(monitor_runloops[monitorId]);
        monitor_runloops[monitorId] = NULL;
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
//    fprintf(stderr, "JNI_OnLoad\n");

    monitor_runloops = malloc(sizeof(CFRunLoopRef) * 1024);
    memset(&monitor_runloops[0], 0, sizeof(monitor_runloops));

    jvm = vm;
    JNIEnv *env;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
    jclass class = (*env)->FindClass(env, "org/vaadin/jonatan/nativefsevents/NativeFSEvents");
    gClass = (*env)->NewGlobalRef(env, class);
    (*env)->DeleteLocalRef(env, class);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    free(monitor_runloops);
}