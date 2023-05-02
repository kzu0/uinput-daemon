#ifndef UINPUT_H
#define UINPUT_H
//---------------------------------------------------------------------------------------------------------------------
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/if_link.h>
#include <linux/uinput.h>
#include <sys/stat.h>
#include <fcntl.h>
//---------------------------------------------------------------------------------------------------------------------
int UinputInit();
void UinputClose(int fd);
void UinputEvent(int fd, int type, int code, int val, __time_t sec = 0, __suseconds_t usec = 0) ;
//---------------------------------------------------------------------------------------------------------------------
#endif // UINPUT_H
