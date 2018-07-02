//File:     qingyan.cpp
//Author:   ��ǿ
//Date:     2018-05-25
//Desc:     ����ƣ�ͼ�ʻ�����豸

#include <iostream>
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
#include <pthread.h>
#include "board_hi3521.h"
#include "rs232_sdk.h"
#include "qingyan.h"
#include "TQueue.h"
#include "packet.h"
#include "commondata.h"
#include "util.h"
#include "syProc.h"
#include "comm_stm32_proc2.h"
#include "display_module.h"
#include "rcvPacket.h"
#include "MediaEvent.h"
#include "MultiMedia.h"
#include "Camera.h"
#include "HD808.h"
#include <sys/time.h>
#include<fstream>
#include <bitset> 
#include "commondata.h"
#define TMP_PATH "/tmp"

//rs232���
int rs232_qingyan_fd = -1;

TQueue<struct qingyan_cmd> qingyan_cmd_que;
TQueue<qingyan_cmd_result> qingyan_cmd_result_que;

#define DPERROR(X) {printf("ERROR(%s %d): ",__FILE__, __LINE__); printf X;}
#define DPINFO(X)  {printf("(%s %d): ", __FILE__, __LINE__); printf X;}
#define COM_PORT_FATIHUEDRIVING "/dev/ttyUSB6"
#ifdef EM6000
//���ò�ͬ����
#define COM_PORT_FATIHUEDRIVING "/dev/ttyAMA2"
#endif

// ����:����������
// ����:
// fd,���ھ��������
// buf,���յ������ݣ����
// to_read,����ȡ���ֽ���
// ����
// ���յ��������ֽ���
// ˵��
// ����յ�������С��to_read��˵������
int fd_rs232_read(int fd, unsigned char *buf, int to_read)
{
    DPINFO(("������������ݺ�����\n"));
    int total = 0, count = 0, cmd_len, n;
    n = to_read;
    cmd_len = 0;
    while(total < to_read)
    {
        count = read(fd, &buf[cmd_len], n);
        if (count <= 0)
        {
            break;
        }
        total += count;
        cmd_len += count;
        n -= count;
    }
    //DPINFO(("���������ݳɹ���\n"));
    return total;
}

//����:д��������
//������
//  fd,���ھ��������
//  buf,��д������ݣ�����
//  to_wirte,��д��������ֽ���
//���أ�
//  ʵ��д����ֽ���
//  ��ʵ��д��������ڴ�д����ֽ�������˵��д��
int fd_rs232_write(int fd, unsigned char *buf, int to_write)
{
    DPINFO(("����д�������ݺ�����\n"));
    int total = 0, count = 0, cmd_len, n;
    n = to_write;
    cmd_len = 0;
    while(total < to_write)
    {
        count = write(fd, &buf[cmd_len], n);
        if (count <= 0)
        {
            break;
        }
        total += count;
        cmd_len += count;
        n -= count;
    }
    //DPINFO(("д�������ݳɹ���\n"));
    return total;
}

//���ܣ�����һ��
//return:
// > 0, ʵ�ʽ��յ��������ֽ���
// <= 0, ����ʧ��
int fd_rs232_read_timeout(int fd, unsigned char *buf,int maxlen,int timeout)
{
    DPINFO(("��������ڳ�ʱ��\n"));
    int count, rc;
	if( fd < 0 || NULL==buf )
	{
		return -1;
	}
	if(timeout <= 0)
	{
		timeout = 3;
	}

	timeval recv_tv;
	recv_tv.tv_sec = timeout;   //wait seconds
	recv_tv.tv_usec = 0;
	fd_set  	fdmask;

	FD_ZERO(&fdmask);		// clear
	FD_SET((unsigned int) fd, &fdmask);	// add
	count = 0;
    while(count <= 2)
    {
    	rc = select( fd + 1, &fdmask, NULL, NULL, &recv_tv);
    	if (rc <= 0)
    	{
	    	perror("@@@@@@select failed!");
		    LOGINFO("timeout socket[%d] ret[%d] errorno[%d] timeout[%d] tv[%u] time[%lu]@@@@@@ \n",fd,rc,errno,timeout,(unsigned int)recv_tv.tv_sec,time(NULL));
	    	if (rc ==0)
	    	{
		    	return rc;
		    }
		    return SOCKET_ERROR; //-1=error
	    }
    	if(FD_ISSET(fd, &fdmask))
        {
            break;
        }
        count++;
    }
    if (count > 2)
    {
        return -1;
    }
	rc = read(fd, buf, maxlen);
	return rc;
}

//д����ָ�����
bool write_qingyan_que(struct qingyan_cmd rec)
{
	return qingyan_cmd_que.write(&rec);
}

//������ָ�����
bool read_qingyan_que(struct qingyan_cmd &rec)
{
	return qingyan_cmd_que.read(&rec);
}

//���Ӧ�����
void clear_qingyan_result_que()
{
	qingyan_cmd_result_que.clear();
}

//д�������ؽ������
bool write_qingyan_result_que(qingyan_cmd_result result)
{
	return qingyan_cmd_result_que.write(&result);
}

//���������ؽ������
bool read_qingyan_result_que(qingyan_cmd_result &result)
{
	return qingyan_cmd_result_que.read(&result);
}

//���ص�ǰ�����м�¼����
long long int count_qingyan_result_que()
{
    return qingyan_cmd_result_que.count();
}

//�Ӷ����ж����ϵ�һ����¼���������Ӷ�����ɾ���ü�¼
bool read_no_remove_qingyan_result_que(qingyan_cmd_result &result)
{
	return qingyan_cmd_result_que.read_no_remove(&result);
}

//��ʼ��ģ��
bool init_qingyan_module()
{
    qingyan_cmd_que.set_buf_num(10);
	if (!qingyan_cmd_que.create())
	{
		return false;
	}
	qingyan_cmd_result_que.set_buf_num(10);
	qingyan_cmd_result_que.create();
    return true;
}

//��ʱ�������,ͨ�� 0xAC 0xC8 ���������ӻ������ٶ��źţ�ƣ��Ԥ����Ӧ�𱨾�״̬
void *thread_add_qingyan_cmd(void *arg)
{
    DPINFO(("������ʱ����̳߳ɹ���\n"));
    //sleep(40);
    //AA 55 CC 33 00 0D AC C8 00 00 41(�ٶ�) 3E 1F   ��������
    //AA 55 CC 33 00 0D AC C8 00 00 06(����) 0C 5F   Ӧ������
    unsigned char speed_cmd[13] = {0xAA,0x55,0xCC,0x33,0x00,0x0D,0xAC,0xC8,0x00,0x00};//,0x41,0x3E,0x1F};
    int speed = 0;
    unsigned short check;
    unsigned char buf[10];
    unsigned char data[20];
    unsigned char c;
    int len, count, times;
    int cmd_len = 13;
    while(true)
    {   
        times = 0;
        if ((g_BNTGlobal.PosInfo_.can_velocity/1000) > 0)
        {//can �ٶ�����
            speed = g_BNTGlobal.PosInfo_.can_velocity / 1000;
        }
        else if ((g_BNTGlobal.PosInfo_.velocity / 1000) > 0)
        {// GPS �ٶ�
            speed = g_BNTGlobal.PosInfo_.velocity / 1000;
        }
        else if ((g_BNTGlobal.PosInfo_.rec_velocity / 1000) > 0)
        {//�����ٶ�
            speed = g_BNTGlobal.PosInfo_.rec_velocity / 1000;
        }
        else
        {
            speed = 0;
        }
        speed_cmd[10] = speed % 256;
        check = getCRC16(speed_cmd, 11);
        speed_cmd[11] = check / 256;
        speed_cmd[12] = check % 256;
        if (fd_rs232_write(rs232_qingyan_fd, speed_cmd, cmd_len) != cmd_len)
        {
            printf("qingyan: д����ʧ��\n");
            continue;
        }
        while(true)
        {
            count = 0;
            if (times > 10)
                break;
            len = fd_rs232_read_timeout(rs232_qingyan_fd, &c, 1, 2);
            if((len != 1) || (c != 0xAA))
            {
                DPINFO(("������Ӧ��ʧ�ܣ�\n"));
                data[10] = 0x00;
                cout << "data[10] ==" << hex << (unsigned int)(unsigned char)data[10] << endl;
                times++;
                continue;
            }
            data[count] = c;
            count++;
            len = read(rs232_qingyan_fd, &c, 1);
            if((len != 1) || (c != 0x55))
            {
                DPINFO(("������Ӧ��ʧ�ܣ�\n"));
                continue;
            }
            data[count] = c;
            count++;
            len = read(rs232_qingyan_fd, &c, 1);
            if((len != 1) || (c != 0xCC))
            {
                DPINFO(("������Ӧ��ʧ�ܣ�\n"));
                continue;
            }
            data[count] = c;
            count++;
            len = read(rs232_qingyan_fd, &c, 1);
            if((len != 1) || (c != 0x33))
            {
                DPINFO(("������Ӧ��ʧ�ܣ�\n"));
                continue;
            }
            data[count] = c;
            count++;
            if (fd_rs232_read(rs232_qingyan_fd, buf, 9) != 9)
            {
                printf("qingyan:������ʧ��\n");
                continue;
            }
            memcpy(&data[count], buf, 9);
            for (int i = 0; i < 13; i++)
                cout << hex << (unsigned int)(unsigned char)data[i] << endl;
            cout << endl;
            break;
        }
        if (data[10] == 0x01)
        {
            take_alarm_photo(1);
            DPINFO(("����ƣ�ͱ�����\n"));
            sleep(1);
        }
        else if (data[10] == 0x03)
        {
            take_alarm_photo(3);
            DPINFO(("���ҿ�������\n"));
            sleep(1);
        }
        else if (data[10] == 0x06)
        {
            take_alarm_photo(6);
            DPINFO(("��ڱ�����\n"));
            sleep(1);
        }
        else if (data[10] == 0x02)
        {
            take_alarm_photo(2);
            DPINFO(("���Ƿƣ�ͱ�����\n"));
            sleep(1);
        }
        else if (data[10] == 0x04)
        {
            take_alarm_photo(4);
            DPINFO(("���ֻ�������\n"));
            sleep(1);
        }
        else if (data[10] == 0x05)
        {
            take_alarm_photo(5);
            DPINFO(("���̱�����\n"));
            sleep(1);
        }
        sleep(5);
    }
    return 0;
}

//���ܣ�����ƣ�ͼ�ʻ����
//���أ�
//     0,�ɹ�
//     -1,ʧ��
int start_qingyan()
{
    pthread_t pth_send_cmd;
    int count;
    count = 0;
    //�򿪴��ڵ���
    // #ifdef EM6000
    // system("himm 0x120F0104 1");
    // system("himm 0x120F0100 1");
    // #endif
    //�ⲿ����
    while (count < 5)
    {
        rs232_qingyan_fd = rs232_open(COM_PORT_FATIHUEDRIVING);
        if (rs232_qingyan_fd == -1)
        {
            printf("�򿪴���%sʧ��\n", COM_PORT_FATIHUEDRIVING);
            sleep(5);
        }
        else
        {
            break;
        }
        count++;
    }
    if (rs232_qingyan_fd == -1)
    {
        printf("�򿪴���%sʧ��\n", COM_PORT_FATIHUEDRIVING);
        return -1;
    }
    printf("�򿪴���%s�ɹ�\n", COM_PORT_FATIHUEDRIVING);
    //DPINFO(("�򿪴��ڳɹ���\n"));
    if (rs232_opt(rs232_qingyan_fd, 115200, 8,'N', 1) != 0)
    {
        printf("���ô���%s����ʧ��\n", COM_PORT_FATIHUEDRIVING);
        close(rs232_qingyan_fd);
        return -1;
    }
    else
    {
        printf("���ô���%s�����ʳɹ�\n", COM_PORT_FATIHUEDRIVING);
        //DPINFO(("���ô��ڲ����ʳɹ���\n"));
    }
    if(pthread_create(&pth_send_cmd, NULL, thread_add_qingyan_cmd,
                       (void *)NULL) != 0)
    {
        close(rs232_qingyan_fd);
        return -1;
    }
    return 0;
}

//��ȡ����ͼƬ
bool read_alarm_photo(char *filename)
{
    DPINFO(("�����ȡ����ͼƬ������\n"));
    bool succeed = true;
    FILE* fp;

    if ((fp = fopen(filename, "w+"))==NULL)
    {
        DPINFO(("���ļ�ʧ�ܣ�\n"));
        succeed = false;
        return false;
    }
    unsigned short check1,check2;
    unsigned char buf1[2];
    unsigned char buf2[2];
    unsigned char buf[512];
    unsigned char photo[522];
    unsigned char data[14];
    unsigned char c;
    int len, count, data_len;
    int cmd_len = 12, package_num;
    int i = 1;
    int timeout = 5;
    unsigned char alarm_photo[12] = {0xAA,0x55,0xCC,0x33,0x00,0x0C,0xAC,0xC9,0x00,0x00};
    check1 = getCRC16(alarm_photo, 10);
    alarm_photo[10] = check1 / 256;
    alarm_photo[11] = check1 % 256;
    if (fd_rs232_write(rs232_qingyan_fd, alarm_photo, cmd_len) != cmd_len)
    {
        DPINFO(("д��������ʧ�ܣ�\n"));
        succeed = false;
    }
    if (fd_rs232_read_timeout(rs232_qingyan_fd, data, 14, timeout) != 14)
    {
        DPINFO(("����������ʧ�ܣ�\n"));
        succeed = false;
    }
    //AA 55 CC 33 00 0C AC C9 00 00 00 08(ͼƬ����) 9E 40 //Ӧ������
    package_num = data[10]* 256 + data[11];

    while(i <= package_num)
    {
        count = 0;

        alarm_photo[9] = i;
        check1 = getCRC16(alarm_photo, 10);
        alarm_photo[10] = check1 / 256;
        alarm_photo[11] = check1 % 256;
        
        if (fd_rs232_write(rs232_qingyan_fd, alarm_photo, cmd_len) != cmd_len)
        {
            DPINFO(("д��������ʧ�ܣ�\n"));
			succeed = false;
            break;
        }
        DPINFO(("д�����ݳɹ���\n"));
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0xAA))
        {
            DPINFO(("����Ϣ��ʶʧ�ܣ�\n"));
			succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0x55))
        {
            DPINFO(("����Ϣ��ʶʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0xCC))
        {
            DPINFO(("����Ϣ��ʶʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0x33))
        {
            DPINFO(("����Ϣ��ʶʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = fd_rs232_read(rs232_qingyan_fd, buf1, 2);
        if(len != 2)
        {
            DPINFO(("ȡ����ʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        data_len = buf1[0] * 256 + buf1[1] - 12;
        photo[count] = buf1[0];
        count++;
        photo[count] = buf1[1];
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0xAC))
        {
            DPINFO(("�������ʶʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0xC9))
        {
            DPINFO(("�������ʶʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != 0x00))
        {
            DPINFO(("��ʣ�����ʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len = read(rs232_qingyan_fd, &c, 1);
        if((len != 1) || (c != i))
        {
            DPINFO(("��ʣ�����ʧ�ܣ�\n"));
            succeed = false;
            break;
        }
        photo[count] = c;
        count++;
        len =  fd_rs232_read(rs232_qingyan_fd, buf, data_len);
        if(len != data_len)
        {
            DPINFO(("ȡͼƬ��ʧ�ܣ�\n"));
            succeed = false;
            break;
        }
		memcpy(&photo[count], buf, data_len);
        count += len;
        //У��
        len = read(rs232_qingyan_fd, buf2, 2);
        if(len != 2)
        {
            DPINFO(("ȡУ���ʧ��\n"));
            succeed = false;
            break;
        }
        photo[count] = buf2[0];
        count++;
        photo[count] = buf2[1];
        count++;
		check1 = getCRC16(photo, count - 2);
		check2 = buf2[0]*256 + buf2[1];
		if (check1 == check2)
		{
        	if (i == 1)
            {
                fwrite(&buf[2], 1, data_len - 2, fp);
            }
            else
            {
                fwrite(buf, 1, data_len, fp);
            }
		}
		else
		{
			printf("У���, check1=%u, check2=%u\n",check1,check2);
			succeed = false;
            break;
		}
        i++;
    }
    fclose(fp);
    if (succeed == true)
    {
        return true;
        DPINFO(("��ȡ����ͼƬ�����ɹ���\n"));
    }
    else
    {
        DPINFO(("��ȡ����ͼƬ����ʧ�ܣ�\n"));
        return false;
    }
}

//�ϴ�����ͼƬ
void take_alarm_photo(unsigned short alarm_type)
{
    struct qingyan_cmd_result result_rec;
    result_rec.alarm_type.data = 0;
    
    struct  CommHead head;
	char file[100] = "", path[80] = "", cmd[300] = "", tmpfile[160] = "";
	//int  i;
	struct MULTIMEDIA_INDEX Index;
	struct PRE_SEND_MULTIMEDIA_INDEX pre_send_index;
	time_t tCur;
	struct  tm  *today;
	//struct MultiMediaEvent_Info mediaEvent;
	bool HD_exists = false, SD_exists = false;
    bool result;
	if (check_disk_mount(HDISK_DIR))
	{
		HD_exists = true;
	}
	else if (check_disk_mount(SD1_DIR))
	{
		SD_exists = true;
	}

	time(&tCur);
	if (HD_exists) //��HD��
	{
		sprintf(cmd, "mkdir -p %s", pict_hd_path);
		SystemEx(cmd, 10);
		sprintf(cmd, "mkdir -p %sgeneral", pict_hd_path);
		SystemEx(cmd, 10);
		today = localtime((const time_t *)&tCur);
		sprintf((char *)path, "%sgeneral/%04d-%02d-%02d",
			pict_hd_path,
			today->tm_year + 1900,
			today->tm_mon + 1,
			today->tm_mday);
		sprintf(cmd, "mkdir -p %s", path);
		SystemEx(cmd, 10);
	}
	else if (SD_exists)
	{
		sprintf(cmd, "mkdir -p %s", pict_sd_path);
		SystemEx(cmd, 10);
		sprintf(cmd, "mkdir -p %sgeneral", pict_sd_path);
		SystemEx(cmd, 10);
		today = localtime((const time_t *)&tCur);
		sprintf((char *)path, "%sgeneral/%04d-%02d-%02d",
			pict_sd_path,
			today->tm_year + 1900,
			today->tm_mon + 1,
			today->tm_mday);
		sprintf(cmd, "mkdir -p %s", path);
		SystemEx(cmd, 10);
	}

    //��һ����Ƭ
    time(&tCur);
    g_BNTGlobal.lock();
    Copy_Pos(g_BNTGlobal.PosInfo_, &Index);
    g_BNTGlobal.unlock();

    today = localtime((const time_t *)&tCur);
    sprintf((char *)file, "%s/photo-%04d-%02d-%02d-%02d-%02d-%02d.%u.jpg",
        path,
        today->tm_year + 1900,
        today->tm_mon + 1,
        today->tm_mday,
        today->tm_hour,
        today->tm_min,
        today->tm_sec, rand());

    sprintf((char *)tmpfile, "%s/tmp_%02d_%02d_%02d.%u.jpg",
        TMP_PATH, today->tm_hour, today->tm_min, today->tm_sec,
        rand());

    result = read_alarm_photo(tmpfile);
    if (!result)
    {
        debug("����ʧ��\n");
    }
    else
    {
        debug("���ճɹ�\n");
    
        Index.type = 0; //ͼ��
        Index.mediaID = get_mediaID(PHOTO );
        Index.channelID = 0;
        Index.eventcode = 4; //����ԭ�򴥷�����
        Index.formatcode = 0; //JPEG
        if (sizeof(tmpfile) != 0)
        {
            strncpy(Index.file, file, sizeof(Index.file) -1);
            sprintf((char *)Index.time, "%04d%02d%02d%02d%02d%02d",
                today->tm_year + 1900,
                today->tm_mon + 1,
                today->tm_mday,
                today->tm_hour,
                today->tm_min,
                today->tm_sec);

            if (HD_exists || SD_exists) //��Ӳ�̻�SD��,������Ƭ
            {
                sprintf(cmd, "cp %s %s -dpRf", tmpfile, file);
                if (SystemEx(cmd, 10) == 0 )
                {
                    strncpy(Index.file, file, sizeof(Index.file) -1);
                    if (Save_Picture_Index(&Index))
                    {
                        debug("�洢������Ƭ�����ɹ�\n");
                    }
                    else
                    {
                        debug("�洢������Ƭ����ʧ��\n");
                    }
                }
                else
                {
                    debug("TakePhoto: %s failed\n", cmd);
                }
            }
            #ifdef EM6000
            int i;
            memset(head.toservers, 0, sizeof(head.toservers));
            for (i = 0; i < SERVER_808_NUM; i++)
            {
                if (g_BNTGlobal.servers_808_[i].start)
                {
                    head.toservers[i].send = true;
                }
            }
            head.recv_server_index = 0;
            strncpy(Index.file, tmpfile, sizeof(Index.file) -1);
            //�������,ͳһ����
            memcpy(&pre_send_index.index, &Index, sizeof(Index));
            memcpy(&pre_send_index.head, &head, sizeof(head));
            g_BNTGlobal.mediaSendFileQue_.write(&pre_send_index);
            #endif
            Check_Photo_Schedule();
            switch (alarm_type)
            {
                case 1:
                    result_rec.alarm_type.qingyan_driving.fatigue = 1;
                    break;
                case 2:
                    result_rec.alarm_type.qingyan_driving.yawn = 1;
                    break;
                case 3:
                    result_rec.alarm_type.qingyan_driving.left_right_see = 1;
                    break;
                case 4:
                    result_rec.alarm_type.qingyan_driving.telephone = 1;
                    break;
                case 5:
                    result_rec.alarm_type.qingyan_driving.smoking = 1;
                    break;
                case 6:
                    result_rec.alarm_type.qingyan_driving.leave_workplace = 1;
                    break;
                default:
                    break;
            }
            result_rec.alarm = alarm_type;
            result_rec.level = 50;
            result_rec.mediaID = Index.mediaID;
            write_qingyan_result_que(result_rec);
        }
    }
}

//���������ؽ�����к���
int get_qingyan_result(struct qingyan_cmd_result &result)
{
    int alarm_status;
    if (read_qingyan_result_que(result))
        alarm_status = 1;
    else
        alarm_status = 0;
    
    return alarm_status;
}

//�Ӷ����ж����ϵ�һ����¼���������Ӷ�����ɾ���ü�¼
int read_no_remove_qingyan_result(struct qingyan_cmd_result &result)
{
    int alarm_status;
    if (read_no_remove_qingyan_result_que(result))
        alarm_status = 1;
    else
        alarm_status = 0;
    
    return alarm_status;
}

//CRC У�����ݣ�CRC-16/MODBUS�����ֽ���ǰ��
const unsigned short g_McRctable_16[256] =
{
0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

//CRC У�麯����CRC-16/MODBUS�����ֽ���ǰ��
unsigned short getCRC16(unsigned char *pdata, int len)
{
    unsigned short cRc_16 = 0xFFFF;
    unsigned char temp;
    while(len-- > 0)
    {
        temp = cRc_16 & 0xFF;
        cRc_16 = (cRc_16 >> 8) ^ g_McRctable_16[(temp ^ *pdata++) & 0xFF];
    }
    return cRc_16;
}

