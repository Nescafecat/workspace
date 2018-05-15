//File:     fingerprint.cpp
//Author:   陈强
//Date:     2018-05-02
//Desc:     三一渣土车指纹仪


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

//rs232句柄
int rs232_fingerprint_fd = -1;

TQueue<FINGER_CMD> finger_cmd_que_; //指纹命令队列

// template<class T>
// int length(T& arr)
// {
//     return sizeof(arr) / sizeof(arr[0]);

// }

#define DPERROR(X) {printf("ERROR(%s %d): ",__FILE__, __LINE__); printf X;}
#define DPINFO(X)  {printf("(%s %d): ", __FILE__, __LINE__); printf X;}
#define COM_PORT_FINGERPRINT "/dev/ttyUSB5";
// 功能:读串口数据
// 参数:
// fd,串口句柄，输入
// buf,接收到的数据，输出
// to_read,待读取的字节数
// 返回
// 接收到的数据字节数
// 说明
// 如果收到的数据小于to_read，说明读错
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

//功能:写串口数据
//参数：
//  fd,串口句柄，输入
//  buf,待写入的数据，输入
//  to_wirte,待写入的数据字节数
//返回：
//  实际写入的字节数
//  如实际写入的数少于待写入的字节数，则说明写错
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


//功能：读命令队列
bool read_finger_cmd(struct FINGER_CMD &rec)
{
    return finger_cmd_que_.read(rec);
}

//功能：写命令队列
bool write_finger_cmd(struct FINGER_CMD rec)
{
    return finger_cmd_que_.write(rec);
}
//初始化指纹模块
bool init_fingerprint_module()
{
    fingerprint_flowno = 0;
    finger_cmd_que_.set_buf_num(10);
    finger_cmd_que_.create();
}


//处理指纹命令
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
        if (rec.cmd == 1) //新增指纹
        {
            //执行add_fingerup_to_device();
            

        }
        else if (rec.cmd == 2) //删除指纹
        {
            //无对应命令
        }
        else if (rec.cmd == 3) //清空指纹库
        {
            //执行ClearMB();
            ClearMB();
        }
    }
}


//定时添加搜索指纹命令
void *thread_add_search_fingerup_cmd(void *arg)
{
    struct FINGER_CMD rec;

    while(true)
    {
        rec.cmd = 0x17; //搜索指纹
        rec.datalen = 0;
        //执行Search();
        Search();
        //写入命令队列的应该是加密后的命令
        sleep(1);
    }
    return 0;
}


//功能：启动指纹任务
//返回：
//     0,成功
//     -1，失败
int start_fingerprint()
{
    pthread_t pth_send, pth_recv, pth_send_cmd, pth_heartbeat;
    int count;
    count = 0;
    //外部串口
    while (count < 5)
    {
        rs232_fingerprint_fd = rs232_open(COM_PORT_FINGERPRINT);
        if (rs232_fingerprint_fd == -1)
        {
            printf("打开串口%s失败\n", COM_PORT_FINGERPRINT);
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
        printf("打开串口%s失败\n", COM_PORT_FINGERPRINT);
        return -1;
    }
    printf("打开串口%s成功\n", COM_PORT_FINGERPRINT);

    if (rs232_opt(rs232_fingerprint_fd, 115200, 8,'N', 1) != 0)
    {
        printf("设置串口%s参数失败\n", COM_PORT_FINGERPRINT);
        close(rs232_fingerprint_fd);
        return -1;
    }
    else
    {
        printf("设置串口%s波特率成功", COM_PORT_FINGERPRINT);
    }

    //处理指纹命令
    if(pthread_create(&pth_recv, NULL, thread_dispose_finger_cmd,
                       (void *)NULL) != 0)
    {
        close(rs232_fingerprint_fd);
        return -1;
    }
    //定时添加搜索指纹命令
    if(pthread_create(&pth_send_cmd, NULL, thread_add_search_fingerup_cmd,
                       (void *)NULL) != 0)
    {
        close(rs232_fingerprint_fd);
        return -1;
    }
    return 0;
}


//功能：加密数据
//参数说明
//  in, 加密前的数据
//  out, 加密后的输出数据
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
//功能：解密数据
//参数
//  in, 已加密数据
//  intlen, 加密数据长度,必须是偶数
//  out,解密后的数据，输出
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
            i += 2；
            if (i >= inlen)
            {
                break;
            }
        }
    }
    else 
        printf("inlen长度不是偶数\n");
}

//新增指纹
void add_fingerup_to_device(struct FINGER_CMD rec)
{
    unsigned char cmd[512], in[512];
    unsigned char userid[32];
    int inlen, outlen, cmdlen;
    inlen = 0;

    if (fg_rs232_write(rs232_fingerprint_fd, cmd, cmdlen) != cmdlen)
    {
        //失败
        
        return;
    }
}

//通用收取指纹设备响应的数据
/****************************************************
未拆分：02 00 04 17 00 00 00 13 03
拆分：02 30 30 30 34 31 37 30 30 30 30 30 30 31 33 03 
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
            printf("收取串口数据失败\n");
            continue;
        }
        data[count] = c;
        count++;
        //取长度
        len = read(rs232_fingerprint_fd, buf, 4);
        if (len != 4)
        {
            printf("取长度标识失败\n");
            continue;
        }
        Joint(buf, len, data, 1);
        Count += 4;
        //将长度换算成十进制
        data_len = ((data[1]&0xF0) * 16 * 16 * 16) + ((data[1]&0x0F) * 16 * 16) + ((data[2]&0xF0) * 16) + data[2]&0x0F;
        //取相应长度的数据
        len = read(rs232_checkter_fd, buf , data_len * 2 - 1);
        if (len != data_len * 2 - 1)
        {
            printf("数据长度不够\n");
            continue;
        }
        Joint(buf, len, data, 3);
        count += len;
        if (data[date_len + 3] ！= FingerPrintChk(data,data_len + 3))
        {
            printf("校验和不对\n");
            continue;
        }
        if (data[3] == 0x00)
        {
            printf("操作成功\n");
            return 0；
        }   
        else if (date[3] == 0x01)
        {
            printf("操作失败\n");
            return 1；
        }
        else if (data[3] == 0x04)
            printf("指纹数据库已满\n");
        else if (data[3] == 0x05)
            printf("无此用户\n");
        else if (data[3] == 0x07)
            printf("用户已存在\n");
        else if (data[3] == 0x08)
            printf("采集超时\n");
        else if (data[3] == 0x09)
            printf("空闲\n");
        else if (data[3] == 0x0A)
            printf("命令执行中\n");
        else if (data[3] == 0x0B)
            printf("有指纹按上\n");
        else if (data[3] == 0x0C)
            printf("无指纹按上\n");
        else if (data[3] == 0x0D)
            printf("指纹认证通过\n");
        else if (data[3] == 0x0E)
            printf("指纹认证失败\n");
        close(rs232_checkter_fd);
        //return NULL;
    }
}



//生成校验码
void FingerPrintChk(unsigned char *date, int len)
{
    unsigned char checknum = 0;
    int i;
    for (i = 1; i < len -2; i++)
        checknum ^= date[i];
    return checknum;
}

//与IC卡内指纹比较





//搜索指纹命令，采集一次指纹特征，在模板库中进行搜索，返回搜索结果。
void Search();
{
    unsigned char search_cmd[] = {0x02, 0x00, 0x04, 0x17, 0x00, 0x00, 0x00, 0x13, 0x03};
    unsigned char split_search_cmd[16];
    split_search_cmd[0] = 0x02;
    split_search_cmd[15] = 0x03;
    Split(search_cmd, 7 , split_search_cmd, 1);
    if (fg_rs232_write(rs232_fingerprint_fd, split_search_cmd, 16) != 16)
        printf("写串口命令失败\n");
    else    
        receive_fg_device_responce();//读应答信息
    // unsigned char search_fail[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char search_success[]

}

//下载指纹
void DownMBLib()
{
    unsigned char DownMBLib_cmd[] = {};
    // unsigned char DownMBLib_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char DownMBLib_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}
//写指纹到flash
void Write_to_flash()
{
    unsigned char write_cmd[] = {0x02, 0x00, 0x04, 0x15, 0x00, 0x00, 0x00, 0x11, 0x03};
    unsigned char split_write_cmd[16];
    split_write_cmd[0] = 0x02;
    split_write_cmd[15] = 0x03;
    Split(write_cmd, 7, split_write_cmd, 1);
    //写入串口命令
    if (fg_rs232_write(rs232_fingerprint_fd, split_write_cmd, 16) != 16)
        printf("写串口命令失败\n");
    else    
        receive_fg_device_responce();//读应答信息
    // unsigned char write_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char write_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}

//清空指纹库
void ClearMB()
{
    //加密前的命令
    unsigned char clear_cmd[] = {0x02, 0x00, 0x04, 0x0A, 0x00, 0x00, 0x00, 0x0E, 0x03};
    //加密后的命令
    unsigned char split_clear_cmd[16];
    split_clear_cmd[0] = 0x02;
    split_clear_cmd[15] = 0x03;
    Split(clear_cmd, 7, split_clear_cmd, 1);//加密命令函数
    //写入串口命令
    if (fg_rs232_write(rs232_fingerprint_fd, split_clear_cmd, 16) != 16)
        printf("写串口命令失败\n");
    else    
        receive_fg_device_responce();//读应答信息
    
    // unsigned char clear_success[] = {0x02, 0x00, 0x02, 0x00, 0x00, 0x02, 0x03};
    // unsigned char clear_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
}



//上传序列号
// void Upload_serialid()
// {
//     unsigned char upload_cmd[] = {0x02, 0x00, 0x04, 0x0E, 0x00, 0x00, 0x00, 0x0A, 0x03};
//     unsigned char upload_success[] = {};
//     unsigned char upload_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
// }


// void Connectfp()//建立连接的命令
// {
//     unsigned char connectfp_cmd[] = {0x02, 0x1B, 0x72, 0x73, 0x00, 0x02, 0x61, 0x63, 0x03};
//     unsigned char connectfp_success[] = {0x02, 0x1B, 0x72, 0x73, 0x00, 0x06, 0x61, 0x00, 0x00, 0x04, 0xFF, 0x6A, 0x03};
//     unsigned char connectfp_fail[] = {0x02, 0x00, 0x02, 0x01, 0x00, 0x03, 0x03};
// }

