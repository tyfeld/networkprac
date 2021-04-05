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
// typedef struct frame_packet
// {
// 	frame contentl
// }
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static queue<frame> sendList;
	static int windowSize = 0;
	frame sendFrame;

	switch (messageType)
	{
	case MSG_TYPE_SEND:
		memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
		sendFrame.size = bufferSize;
		sendList.push(sendFrame);
		if (windowSize < WINDOW_SIZE_STOP_WAIT)
		{
			windowSize++;
			SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
		}
		else
			return 0;
		break;
	case MSG_TYPE_RECEIVE:
		unsigned int received_ack = ntohl((((frame *)pBuffer)->head).ack);
		if (!sendList.empty())
		{
			unsigned int front_ack = ntohl(sendList.front().head.seq);
			if (front_ack == received_ack)
			{
				sendList.pop();
				if (!sendList.empty())
				{
					sendFrame = sendList.front()
									SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
				}
				else
					windowSize--;
			}
		}
		break;
	case MSG_TYPE_TIMEOUT:
		sendFrame = sendList.front();
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
	static queue<frame> sendList;
	static deque<frame> sendWindow;
	//static int windowSize = 0;
	frame sendFrame;

	switch (messageType)
	{
	case MSG_TYPE_SEND:
		memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
		sendFrame.size = bufferSize;
		sendList.push(sendFrame);
		if (sendList.size() < WINDOW_SIZE_BACK_N_FRAME)
		{
			//sendList.push(sendFrame);
			//windowSize++;
			sendWindow.push_back(sendFrame);
			SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
			sendList.pop();
		}
		else
			return 0;
		break;
	case MSG_TYPE_RECEIVE:
		unsigned int received_ack = ntohl((((frame *)pBuffer)->head).ack);
		while (!sendWindow.empty() && ntohl(sendWindow.begin()->head.seq) != received_ack)
		{
			sendWindow.pop_front();
		}
		if (!sendWindow.empty())
		{
			sendWindow.pop_front();
		}
		while (sendWindow.size() < WINDOW_SIZE_BACK_N_FRAME && !sendList.empty())
		{
			sendFrame = sendList.front();
			sendWindow.push_back(sendFrame);
			SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
			sendList.pop();
		}
		break;
	case MSG_TYPE_TIMEOUT:
		for (deque<struct frame>::iterator it = sendWindow.begin(); it != sendWindow.end(); ++it)
		{
			SendFRAMEPacket((unsigned char *)(&it), (unsigned int)it->size);
		}
		break;
	}
	return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)

{
	static queue<frame> sendList;
	static deque<frame> sendWindow;
	//static int windowSize = 0;
	frame sendFrame;

	switch (messageType)
	{
	case MSG_TYPE_SEND:
		memcpy(&sendFrame, pBuffer, sizeof(sendFrame));
		sendFrame.size = bufferSize;
		sendList.push(sendFrame);
		if (sendList.size() < WINDOW_SIZE_BACK_N_FRAME)
		{
			//sendList.push(sendFrame);
			//windowSize++;
			sendWindow.push_back(sendFrame);
			SendFRAMEPacket((unsigned char *)pBuffer, (unsigned int)bufferSize);
			sendList.pop();
		}
		else
			return 0;
		break;
	case MSG_TYPE_RECEIVE:
		unsigned int received_ack = ntohl((((frame *)pBuffer)->head).ack);
		frame_kind received_kind = ntohl((((frame *)pBuffer)->head).kind);
		if (received_kind == ack)
		{
			while (!sendWindow.empty() && ntohl(sendWindow.begin()->head.seq) != received_ack)
			{
				sendWindow.pop_front();
			}
			if (!sendWindow.empty())
			{
				sendWindow.pop_front();
			}
		}
		else if (received_kind == nak)
		{
			for (deque<struct frame>::iterator it = sendWindow.begin(); it != sendWindow.end(); ++it)
				if (ntohl(it->head.seq) == received_ack)
				{
					SendFRAMEPacket((unsigned char *)(&it), (unsigned int)it->size);
				}
		}
		while (sendWindow.size() < WINDOW_SIZE_BACK_N_FRAME && !sendList.empty())
		{
			sendFrame = sendList.front();
			sendWindow.push_back(sendFrame);
			SendFRAMEPacket((unsigned char *)(&sendFrame), (unsigned int)sendFrame.size);
			sendList.pop();
		}
		break;
	// case MSG_TYPE_TIMEOUT:
	// 	for (deque<struct frame>::iterator it = sendWindow.begin(); it != sendWindow.end(); ++it)
	// 	{
	// 		SendFRAMEPacket((unsigned char *)(&it), (unsigned int)it->size);
	// 	}
	// 	break;
	}
	return 0;
}
