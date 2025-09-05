#ifndef _HAWKDMSCALL_H
#define _HAWKDMSCALL_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>

#define PIPELINE_PATH "/tmp/.hawkcall"
#define BUFFER_SIZE 512

extern int broadcastMessage(const char *message);

#endif