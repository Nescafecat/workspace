//File:       fingerprint.h
//Author:     陈强
//Date:       2018-05-02
//Desc:       三一渣土车指纹仪


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#define FINGERPRINT_MAX_DATA_LEN    512


//指纹操作命令
struct FINGER_CMD
{
	unsigned char cmd;  //0x1=新增指纹,0x2=删除指定指纹,0x3=清空指纹库
					    //0x17=搜索指纹库
	int datalen;		//数据长度
	unsigned char data[FINGERPRINT_MAX_DATA_LEN]; //命令对应的数据
}


//指纹命令响应
struct FINGER_REPLY
{

}


//功能:读串口数据
//参数:
//  fd,串口句柄，输入
//  buf,接收到的数据，输出
//  to_read,待读取的字节数
//返回
//  接收到的数据字节数
//说明
//  如果收到的数据小于to_read，说明读错
int fg_rs232_read(int fd, unsigned char *buf, int to_read);

//功能:写串口数据
//参数：
//  fd,串口句柄，输入
//  buf,待写入的数据，输入
//  to_wirte,待写入的数据字节数
//返回：
//  实际写入的字节数
//  如实际写入的数少于待写入的字节数，则说明写错
int fg_rs232_write(int fd, unsigned char *buf, int to_write);

//功能：读命令队列
bool read_finger_cmd(struct FINGER_CMD &rec);

//功能：写命令队列
bool write_finger_cmd(struct FINGER_CMD rec);

//初始化指纹模块
bool init_fingerprint_module();

//处理指纹命令
void *thread_dispose_finger_cmd(void *arg);

//定时添加搜索指纹命令
void *thread_add_search_fingerup_cmd(void *arg);

#endif
