#include "sysinclude.h"
#include "queue"
#include <deque>
extern void SendFRAMEPacket(unsigned char *pData, unsigned int len);
using namespace std;
#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4
/*
* 停等协议测试函数
*/
typedef enum
{
    data,
    ack,
    nak
} frame_kind;
typedef struct frame_head
{
    frame_kind kind;
    unsigned int seq;
    unsigned int ack;
    unsigned char data[100]; //数据
};
typedef struct frame
{
    frame_head head;   //帧头
    unsigned int size; //数据的大小
};
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
    static queue<frame> waitList;
    static int windowSize = 0;
    frame sendFrame;

    switch (messageType)
    {
    case MSG_TYPE_SEND:
    {
        memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
        sendFrame.size = bufferSize;
        waitList.push(sendFrame);
        if (windowSize < WINDOW_SIZE_STOP_WAIT)
        {
            windowSize++;
            SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
        }
        else
            return 0;
        break;
    }
    case MSG_TYPE_RECEIVE:
    {
        unsigned int received_ack = ntohl((((frame *)pBuffer)->head).ack);
        if (!waitList.empty())
        {
            unsigned int front_ack = ntohl(waitList.front().head.seq);
            if (front_ack == received_ack)
            {
                waitList.pop();
                if (!waitList.empty())
                {
                    sendFrame = waitList.front();
                    SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
                }
                else
                    windowSize--;
            }
        }
        break;
    }
    case MSG_TYPE_TIMEOUT:
        sendFrame = waitList.front();
        SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
        break;
    }
    return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
    static queue<frame> waitList;
    static deque<frame> slidingWindow;
    frame sendFrame;
    unsigned int received_ack;
    switch (messageType)
    {
    case MSG_TYPE_SEND:
        memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
        sendFrame.size = bufferSize;
        waitList.push(sendFrame);
        if (slidingWindow.size() < WINDOW_SIZE_BACK_N_FRAME)
        {
            slidingWindow.push_back(sendFrame);
            SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
            waitList.pop();
        }
        else
            return 0;
        break;
    case MSG_TYPE_RECEIVE:
        received_ack = ntohl((((frame *)pBuffer)->head).ack);
        while (!slidingWindow.empty() && ntohl(slidingWindow.begin()->head.seq) != received_ack)
            slidingWindow.pop_front();
        if (!slidingWindow.empty())
            slidingWindow.pop_front();
        while (slidingWindow.size() < WINDOW_SIZE_BACK_N_FRAME && !waitList.empty())
        {
            sendFrame = waitList.front();
            slidingWindow.push_back(sendFrame);
            SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
            waitList.pop();
        }
        break;

    case MSG_TYPE_TIMEOUT:
        for (deque<struct frame>::iterator it = slidingWindow.begin(); it != slidingWindow.end(); ++it)
            SendFRAMEPacket((unsigned char *)&(*it), it->size);
        break;
    }

    return 0;
}
/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)

{
    static queue<frame> waitList;
    static deque<frame> slidingWindow;
    frame sendFrame;
    unsigned int received_ack;
    frame_kind received_kind;

    switch (messageType)
    {
    case MSG_TYPE_SEND:
        memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
        sendFrame.size = bufferSize;
        waitList.push(sendFrame);
        if (waitList.size() < WINDOW_SIZE_BACK_N_FRAME)
        {
            slidingWindow.push_back(sendFrame);
            SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
            waitList.pop();
        }
        else
            return 0;
        break;
    case MSG_TYPE_RECEIVE:
        received_ack = ntohl((((frame *)pBuffer)->head).ack);
        received_kind = frame_kind(ntohl((((frame *)pBuffer)->head).kind));
        if (received_kind == ack)
        {
            while (!slidingWindow.empty() && ntohl(slidingWindow.begin()->head.seq) != received_ack)
                slidingWindow.pop_front();
            if (!slidingWindow.empty())
                slidingWindow.pop_front();
        }
        else if (received_kind == nak)
            for (deque<struct frame>::iterator it = slidingWindow.begin(); it != slidingWindow.end(); ++it)
                if (ntohl(it->head.seq) == received_ack)
                {
                    SendFRAMEPacket((unsigned char *)&(*it), it->size);
                    break;
                }
        while (slidingWindow.size() < WINDOW_SIZE_BACK_N_FRAME && !waitList.empty())
        {
            sendFrame = waitList.front();
            slidingWindow.push_back(sendFrame);
            SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
            waitList.pop();
        }
        break;
    }
    return 0;
}

