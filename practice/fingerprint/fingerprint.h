//File:       fingerprint.h
//Author:     ��ǿ
//Date:       2018-05-02
//Desc:       ��һ������ָ����


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


//ָ�Ʋ�������
struct FINGER_CMD
{
	unsigned char cmd;  //0x1=����ָ��,0x2=ɾ��ָ��ָ��,0x3=���ָ�ƿ�
					    //0x17=����ָ�ƿ�
	int datalen;		//���ݳ���
	unsigned char data[FINGERPRINT_MAX_DATA_LEN]; //�����Ӧ������
}


//ָ��������Ӧ
struct FINGER_REPLY
{

}


//����:����������
//����:
//  fd,���ھ��������
//  buf,���յ������ݣ����
//  to_read,����ȡ���ֽ���
//����
//  ���յ��������ֽ���
//˵��
//  ����յ�������С��to_read��˵������
int fg_rs232_read(int fd, unsigned char *buf, int to_read);

//����:д��������
//������
//  fd,���ھ��������
//  buf,��д������ݣ�����
//  to_wirte,��д��������ֽ���
//���أ�
//  ʵ��д����ֽ���
//  ��ʵ��д��������ڴ�д����ֽ�������˵��д��
int fg_rs232_write(int fd, unsigned char *buf, int to_write);

//���ܣ����������
bool read_finger_cmd(struct FINGER_CMD &rec);

//���ܣ�д�������
bool write_finger_cmd(struct FINGER_CMD rec);

//��ʼ��ָ��ģ��
bool init_fingerprint_module();

//����ָ������
void *thread_dispose_finger_cmd(void *arg);

//��ʱ�������ָ������
void *thread_add_search_fingerup_cmd(void *arg);

#endif
