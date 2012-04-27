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

#define DEBUG

#ifdef DEBUG
#define LOG(x) fprintf(stderr, x)
#else
#define LOG(x)
#endif

CFStringRef to_cfstring(JNIEnv *env, jstring path);
void fs_callback(ConstFSEventStreamRef streamRef,
                 void *clientCallBackInfo,
                 size_t numEvents,
                 void *eventPaths,
                 const FSEventStreamEventFlags eventFlags[],
                 const FSEventStreamEventId eventIds[]);
void install_monitor();


static JavaVM *jvm;
static jclass gClass = NULL;


void fs_callback(ConstFSEventStreamRef streamRef,
                 void *clientCallBackInfo,
                 size_t numEvents,
                 void *eventPaths,
                 const FSEventStreamEventFlags eventFlags[],
                 const FSEventStreamEventId eventIds[])
{
    int i;
    char **paths = eventPaths;
    
    JNIEnv *env = NULL;
    
    LOG("fs_callback()\n");

    (*jvm)->AttachCurrentThread(jvm, (void **)&env, NULL);
    jmethodID java_callback = (*env)->GetStaticMethodID(env, gClass, "eventCallback", "(Ljava/lang/String;)V");
    for (i=0; i<numEvents; i++) {
        jstring path = (*env)->NewStringUTF(env, paths[i]);
        (*env)->CallStaticVoidMethod(env, gClass, java_callback, path);
    }
    (*jvm)->DetachCurrentThread(jvm);
}

static CFMutableArrayRef monitored_paths;
static volatile CFRunLoopRef monitor_runloop;

CFStringRef to_cfstring(JNIEnv *env, jstring path) {
    const char *cpath = (*env)->GetStringUTFChars(env, path, NULL);
    return CFStringCreateWithCString(kCFAllocatorDefault, cpath, kCFStringEncodingUTF8);
}

void install_monitor()
{
    LOG("install_monitor()\n");
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        FSEventStreamRef streamRef = monitor_paths(monitored_paths, &fs_callback);
        monitor_runloop = CFRunLoopGetCurrent();
        CFRunLoopRun();
        unmonitor(streamRef);
    });
    while (monitor_runloop == NULL) {
        sleep(1);
    }
}

JNIEXPORT void JNICALL Java_org_vaadin_jonatan_nativefsevents_NativeFSEvents_monitor(JNIEnv *env, jclass class, jstring path)
{
    LOG("monitor()\n");
    CFArrayAppendValue(monitored_paths, to_cfstring(env, path));
    install_monitor();
}
                       
JNIEXPORT void JNICALL Java_org_vaadin_jonatan_nativefsevents_NativeFSEvents_unmonitor(JNIEnv *env, jclass class, jstring path)
{
    LOG("unmonitor()\n");
    if (monitor_runloop != NULL) {
        CFRunLoopStop(monitor_runloop);
        monitor_runloop = NULL;
        CFStringRef cfpath = to_cfstring(env, path);
        for (int i=0; i<CFArrayGetCount(monitored_paths); i++) {
            CFStringRef str = CFArrayGetValueAtIndex(monitored_paths, i);
            if (CFStringCompare(str, cfpath, 0) == kCFCompareEqualTo) {
                CFArrayRemoveValueAtIndex(monitored_paths, i);
                break;
            }
        }
    }
    if (CFArrayGetCount(monitored_paths) > 0) {
        // Reinstall the monitor without the unmonitored path.
        install_monitor();
    }
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
    LOG("JNI_OnLoad()\n");
    monitored_paths = CFArrayCreateMutable(NULL, 0, NULL);
    
    jvm = vm;
    JNIEnv *env;
    (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_1_6);
    jclass class = (*env)->FindClass(env, "org/vaadin/jonatan/nativefsevents/NativeFSEvents");
    gClass = (*env)->NewGlobalRef(env, class);
    (*env)->DeleteLocalRef(env, class);
    return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *vm, void *reserved) {
    LOG("JNI_OnUnload()\n");
    CFRelease(monitored_paths);
}