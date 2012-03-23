//
//  FSEventHandler.c
//  jnifsevents
//
//  Created by Jonatan Kronqvist on 15.12.2011.
//  Copyright 2011 n/a. All rights reserved.
//

#include <stdio.h>
#include "FSEventHandler.h"


FSEventStreamRef monitor_path(const char *path, FSEventStreamCallback callback)
{
    CFStringRef mypath = CFStringCreateWithCString(kCFAllocatorDefault, path, kCFStringEncodingUTF8);
    CFArrayRef pathsToWatch = CFArrayCreate(NULL, (const void **)&mypath, 1, NULL);
    void *callbackInfo = NULL; // could put stream-specific data here.
    FSEventStreamRef stream;
    CFAbsoluteTime latency = 1.0; /* Latency in seconds */
    
    /* Create the stream, passing in a callback */
    stream = FSEventStreamCreate(NULL,
                                 callback,
                                 callbackInfo,
                                 pathsToWatch,
                                 kFSEventStreamEventIdSinceNow, /* Or a previous event ID */
                                 latency,
                                 kFSEventStreamCreateFlagNoDefer /* Flags explained in reference */
                                 );
    
    FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamStart(stream);
    return stream;
}

void unmonitor(FSEventStreamRef stream)
{
    FSEventStreamStop(stream);
    FSEventStreamUnscheduleFromRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    FSEventStreamInvalidate(stream);
    FSEventStreamRelease(stream);
}