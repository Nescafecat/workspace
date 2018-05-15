//File:     fingerprint.cpp
//Author:   ��ǿ
//Date:     2018-05-02
//Desc:     ��һ������ָ����


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
#include "fingerprint.h"
#include "TQueue.h"
#include "packet.h"
#include "commondata.h"
#include "util.h"
#include "syProc.h"

//rs232���
int rs232_fingerprint_fd = -1;

TQueue<FINGER_CMD> finger_cmd_que_; //ָ���������

// template<class T>
// int length(T& arr)
// {
//     return sizeof(arr) / sizeof(arr[0]);

// }

#define DPERROR(X) {printf("ERROR(%s %d): ",__FILE__, __LINE__); printf X;}
#define DPINFO(X)  {printf("(%s %d): ", __FILE__, __LINE__); printf X;}
#define COM_PORT_FINGERPRINT "/dev/ttyUSB5";
// ����:����������
// ����:
// fd,���ھ��������
// buf,���յ������ݣ����
// to_read,����ȡ���ֽ���
// ����
// ���յ��������ֽ���
// ˵��
// ����յ�������С��to_read��˵������
int fg_rs232_read(int fd, unsigned char *buf, int to_read)
{
    int total = 0, count = 0, pos, n;
    n = to_read;
    pos = 0;
    while(total < to_read)
    {
        count = read(fd, &buf[pos], n);
        if (count <= 0)
        {
            break;
        }
        total += count;
        pos += count;
        n -= count;
    }
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
int fg_rs232_write(int fd, unsigned char *buf, int to_write)
{
    int total = 0, count = 0, pos, n;
    n = to_write;
    pos = 0;
    while(total < to_write)
    {
        count = write(fd, &buf[pos], n);
        if (count <= 0)
        {
            break;
        }
        total += count;
        pos += count;
        n -= count;
    }
    return total;
}


//���ܣ����������
bool read_finger_cmd(struct FINGER_CMD &rec)
{
    return finger_cmd_que_.read(rec);
}

//���ܣ�д�������
bool write_finger_cmd(struct FINGER_CMD rec)
{
    return finger_cmd_que_.write(rec);
}
//��ʼ��ָ��ģ��
bool init_fingerprint_module()
{
    fingerprint_flowno = 0;
    finger_cmd_que_.set_buf_num(10);
    finger_cmd_que_.create();
}


//����ָ������
void *thread_dispose_finger_cmd(void *arg)
{
    struct FINGER_CMD rec;
    while(true)
    {
        if(!read_finger_cmd(rec))
        {
            sleep(1);
            continue;
        }
        if (rec.cmd == 1) //����ָ��
        {
            //ִ��add_fingerup_to_device();
            

        }
        else if (rec.cmd == 2) //ɾ��ָ��
        {
            //�޶�Ӧ����
        }
        else if (rec.cmd == 3) //���ָ�ƿ�
        {
            //ִ��ClearMB();
            ClearMB();
        }
    }
}


//��ʱ�������ָ������
void *thread_add_search_fingerup_cmd(void *arg)
{
    struct FINGER_CMD rec;

    while(true)
    {
        rec.cmd = 0x17; //����ָ��
        rec.datalen = 0;
        //ִ��Search();
        Search();
        //д��������е�Ӧ���Ǽ��ܺ������
        sleep(1);
    }
    return 0;
}


//���ܣ�����ָ������
//���أ�
//     0,�ɹ�
//     -1��ʧ��
int start_fingerprint()
{
    pthread_t pth_send, pth_recv, pth_send_cmd, pth_heartbeat;
    int count;
    count = 0;
    //�ⲿ����
    while (count < 5)
    {
        rs232_fingerprint_fd = rs232_open(COM_PORT_FINGERPRINT);
        if (rs232_fingerprint_fd == -1)
        {
            printf("�򿪴���%sʧ��\n", COM_PORT_FINGERPRINT);
            sleep(5);
        }
        else
        {
            break;
        }
        count++;
    }
    if (rs232_fingerprint_fd == -1)
    {
        printf("�򿪴���%sʧ��\n", COM_PORT_FINGERPRINT);
        return -1;
    }
    printf("�򿪴���%s�ɹ�\n", COM_PORT_FINGERPRINT);

    if (rs232_opt(rs232_fingerprint_fd, 115200, 8,'N', 1) != 0)
    {
        printf("���ô���%s����ʧ��\n", COM_PORT_FINGERPRINT);
        close(rs232_fingerprint_fd);
        return -1;
    }
    else
    {
        printf("���ô���%s�����ʳɹ�", COM_PORT_FINGERPRINT);
    }

    //����ָ������
    if(pthread_create(&pth_recv, NULL, thread_dispose_finger_cmd,
                       (void *)NULL) != 0)
    {
        close(rs232_fingerprint_fd);
        return -1;
    }
    //��ʱ�������ָ������
    if(pthread_create(&pth_send_cmd, NULL, thread_add_search_fingerup_cmd,
                       (void *)NULL) != 0)
    {
        close(rs232_fingerprint_fd);
        return -1;
    }
    return 0;
}


//���ܣ���������
//����˵��
//  in, ����ǰ������
//  out, ���ܺ���������
void Split(unsigned char *in, int inlen, unsigned char *out, int outlen)
{

    int i;
    //outlen = 1;
    for (i = 0; i < inlen; i++)
    {
        out[outlen] = (in[i] & 0xf0 ) >> 4+0x30;
        outlen += 1;
        out[outlen] = (in[i] & 0x0f) + 0x30;
        outlen++;
    }
}
//���ܣ���������
//����
//  in, �Ѽ�������
//  intlen, �������ݳ���,������ż��
//  out,���ܺ�����ݣ����
void Joint(unsigned char *in, int inlen, unsigned char *out, int outlen)
{
    int i = 0;
    //outlen = 1;
    if (inlen % 2 == 0)
    {
        while (true)
        {
            out[outlen] = ((in[i] &0x0f) << 4) | (in[i+1] & 0x0f);
            outlen++;
            i += 2��
            if (i >= inlen)
            {
                break;
            }
        }
    }
    else 
        printf("inlen���Ȳ���ż��\n");
}

//����ָ��
void add_fingerup_to_device(struct FINGER_CMD rec)
{
    unsigned char cmd[512], in[512];
    unsigned char userid[32];
    int inlen, outlen, cmdlen;
    inlen = 0;

    if (fg_rs232_write(rs232_fingerprint_fd, cmd, cmdlen) != cmdlen)
    {
        //ʧ��
        
        return;
    }
}

//ͨ����ȡָ���豸��Ӧ������
/****************************************************
δ��֣�02 00 04 17 00 00 00 13 03
��֣�02 30 30 30 34 31 37 30 30 30 30 30 30 31 33 03 
*****************************************************/
void receive_fg_device_responce(void *arg)
{
    unsigned char buf[256], date[512];
    unsigned char c;
    int len, count, data_len;
    while (true)
    {
        count = 0;
        len = read(rs232_fingerprint_fd, &c,1);
        if (len != 1 || c != 0x02)
        {
            printf("��ȡ��������ʧ��\n");
            continue;
        }
        data[count] = c;
        count++;
        //ȡ����
        len = read(rs232_fingerprint_fd, buf, 4);
        if (len != 4)
        {
            printf("ȡ���ȱ�ʶʧ��\n");
            continue;
        }
        Joint(buf, len, data, 1);
        Count += 4;
        //�����Ȼ����ʮ����
        data_len = ((data[1]&0xF0) * 16 * 16 * 16) + ((data[1]&0x0F) * 16 * 16) + ((data[2]&0xF0) * 16) + data[2]&0x0F;
        //ȡ��Ӧ���ȵ�����
        len = read(rs232_checkter_fd, buf , data_len * 2 - 1);
        if (len != data_len * 2 - 1)
        {
            printf("���ݳ��Ȳ���\n");
            continue;
        }
        Joint(buf, len, data, 3);
        count += len;
        if (data[date_len + 3] ��= FingerPrintChk(data,data_len + 3))
        {
            printf("У��Ͳ���\n");
            continue;
        }
        if (data[3] == 0x00)
        {
            printf("�����ɹ�\n");
            return 0��
        }   
        else if (date[3] == 0x01)
        {
            printf("����ʧ��\n");
            return 1��
        }
        else if (data[3] == 0x04)
            printf("ָ�����ݿ�����\n");
        else if (data[3] == 0x05)
            printf("�޴��û�\n");
        else if (data[3] == 0x07)
            printf("�û��Ѵ���\n");
        else if (data[3] == 0x08)
            printf("�ɼ���ʱ\n");
        else if (data[3] == 0x09)
            printf("����\n");
        else if (data[3] == 0x0A)
            printf("����ִ����\n");
        else if (data[3] == 0x0B)
            printf("��ָ�ư���\n");
        else if (data[3] == 0x0C)
            printf("��ָ�ư���\n");
        else if (data[3] == 0x0D)
            printf("ָ����֤ͨ��\n");
        else if (data[3] == 0x0E)
            printf("ָ����֤ʧ��\n");
        close(rs232_checkter_fd);
        //return NULL;
    }
}



//����У����
void FingerPrintChk(unsigned char *date, int len)
{
    unsigned char checknum = 0;
    int i;
    for (i = 1; i < len -2; i++)
        checknum ^= date[i];
    return checknum;
}

//��IC����ָ�ƱȽ�





//����ָ������ɼ�һ��ָ����������ģ����н����������������������
void Search();
{
    unsigned char search_cmd[] = {0x02, 0x00, 0x04, 0x17, 0x00, 0x00, 0x00, 0x13, 0x03};
    unsigned char split_search_cmd[16];
    split_search_cmd[0] = 0x02;
    split_search_cmd[15] = 0x03;
    Split(search_cmd, 7 , split_search_cmd, 1);
    if (fg_rs232_write(rs232_fingerprint_fd, split_search_cmd, 16) != 16)
        printf("д��������ʧ��\n");
    else    
        receive_fg_device_responce();//��Ӧ����Ϣ
    // unsigned char search_fail[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char search_success[]

}

//����ָ��
void DownMBLib()
{
    unsigned char DownMBLib_cmd[] = {};
    // unsigned char DownMBLib_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char DownMBLib_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}
//дָ�Ƶ�flash
void Write_to_flash()
{
    unsigned char write_cmd[] = {0x02, 0x00, 0x04, 0x15, 0x00, 0x00, 0x00, 0x11, 0x03};
    unsigned char split_write_cmd[16];
    split_write_cmd[0] = 0x02;
    split_write_cmd[15] = 0x03;
    Split(write_cmd, 7, split_write_cmd, 1);
    //д�봮������
    if (fg_rs232_write(rs232_fingerprint_fd, split_write_cmd, 16) != 16)
        printf("д��������ʧ��\n");
    else    
        receive_fg_device_responce();//��Ӧ����Ϣ
    // unsigned char write_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char write_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}

//���ָ�ƿ�
void ClearMB()
{
    //����ǰ������
    unsigned char clear_cmd[] = {0x02, 0x00, 0x04, 0x0A, 0x00, 0x00, 0x00, 0x0E, 0x03};
    //���ܺ������
    unsigned char split_clear_cmd[16];
    split_clear_cmd[0] = 0x02;
    split_clear_cmd[15] = 0x03;
    Split(clear_cmd, 7, split_clear_cmd, 1);//���������
    //д�봮������
    if (fg_rs232_write(rs232_fingerprint_fd, split_clear_cmd, 16) != 16)
        printf("д��������ʧ��\n");
    else    
        receive_fg_device_responce();//��Ӧ����Ϣ
    
    // unsigned char clear_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char clear_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}



//�ϴ����к�
// void Upload_serialid()
// {
//     unsigned char upload_cmd[] = {0x02, 0x00, 0x04, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x03};
//     unsigned char upload_success[] = {};
//     unsigned char upload_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
// }


// void Connectfp()//�������ӵ�����
// {
//     unsigned char connectfp_cmd[] = {0x02, 0x1B, 0x72, 0x73, 0x00, 0x02, 0x61, 0x63, 0x03};
//     unsigned char connectfp_success[] = {0x02, 0x1B, 0x72, 0x73, 0x00, 0x06, 0x61, 0x00, 0x00, 0x04, 0xFF, 0x6A, 0x03};
//     unsigned char connectfp_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
// }

