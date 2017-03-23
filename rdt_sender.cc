/*
 * FILE: rdt_sender.cc
 * DESCRIPTION: Reliable data transfer sender.
 * NOTE: This implementation assumes there is no packet loss, corruption, or 
 *       reordering.  You will need to enhance it to deal with all these 
 *       situations.  In this implementation, the packet format is laid out as 
 *       the following:
 *       
 *       |<-  1 byte  ->|<-             the rest            ->|
 *       | payload size |<-             payload             ->|
 *
 *       The first byte of each packet indicates the size of the payload
 *       (excluding this single-byte header)
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rdt_struct.h"
#include "rdt_sender.h"

#define Window_Size 5
#define Real_Window_Size 60
#define Buffer_Size 1000000

static packet window[Real_Window_Size+1];
static packet buffer[Buffer_Size+1];
static double timeout=0.3;
static int ack_expected=0;
static int next_packet_to_send=0;
static int buf_begin=0;
static int buf_end=0;
static int winbuffered=0;
static int bufbuffered=0;

static bool between(int a, int b, int c)
{
	return (a<=b&&b<c)||(c<a&&a<=b)||(c<a&&b<c);
}

int create_checksum(struct packet* pkt)
{
	
	unsigned short* buf=(unsigned short*)(pkt->data);
	int sum=0;
	int size=(RDT_PKTSIZE/4)-2;

	sum += *buf;
	buf+=3;
	sum+=*buf;
	buf+=1;

	int* buf1=(int*)buf;
	
	while(size>0)
	{
		sum += *buf1;
		//sum = (sum>>16) + (sum&0xffff);
		buf1+=1;
		size-=1;
	}
	
	buf=(unsigned short*)(pkt->data);
	buf+=1;
	buf1=(int*)buf;
	*buf1=sum;
	
	return sum;
}

bool check_checksum(struct packet* pkt)
{
	unsigned short* buf=(unsigned short*)(pkt->data);
	buf+=1;
	int* buf1=(int*)buf;
	int sum1=*buf1;
	int sum2=create_checksum(pkt);
	if(sum1==sum2)return true;
	else return false;
}

/* sender initialization, called once at the very beginning */
void Sender_Init()
{
    fprintf(stdout, "At %.2fs: sender initializing ...\n", GetSimulationTime());
}

/* sender finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to take this opportunity to release some 
   memory you allocated in Sender_init(). */
void Sender_Final()
{
    fprintf(stdout, "At %.2fs: sender finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a message is passed from the upper layer at the 
   sender */
void Sender_FromUpperLayer(struct message *msg)
{

    /* 6-byte header indicating the size of the payload */
    int header_size = 6;

    /* maximum payload size */
    int maxpayload_size = RDT_PKTSIZE - header_size;

    /* split the message if it is too big */
    /* the cursor always points to the first unsent byte in the message */
    int cursor = 0;
    while (msg->size-cursor > maxpayload_size) {
	/*check the buffer first*/
	while(bufbuffered>0)
	{
	
		if(winbuffered<Window_Size)
		{
			//copy packet
			window[next_packet_to_send].data[0] = buffer[buf_begin].data[0];
			window[next_packet_to_send].data[1] = next_packet_to_send;
			memcpy(window[next_packet_to_send].data+header_size, buffer[buf_begin].data+header_size, buffer[buf_begin].data[0]);
			create_checksum(&window[next_packet_to_send]); 
			
			//send packet and set timer
			Sender_ToLowerLayer(&window[next_packet_to_send]);
			if(winbuffered==0)Sender_StartTimer(timeout);

			//change buffer and window cusor
			winbuffered+=1;
			next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
			bufbuffered-=1;
			buf_begin=(buf_begin+1)%(Buffer_Size+1);
		}
		else
		{
			break;
		}
	}

	/*check the window size*/
	if(winbuffered<Window_Size)
	{
		window[next_packet_to_send].data[0] = maxpayload_size;
		window[next_packet_to_send].data[1]=next_packet_to_send;
		memcpy(window[next_packet_to_send].data+header_size, msg->data+cursor, maxpayload_size);
		create_checksum(&window[next_packet_to_send]);

		//send packet and set timer
		Sender_ToLowerLayer(&window[next_packet_to_send]);
		if(winbuffered==0)Sender_StartTimer(timeout);

		winbuffered+=1;
		next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
	}
	else if(bufbuffered<Buffer_Size)
	{
		buffer[buf_end].data[0]=maxpayload_size;
		memcpy(buffer[buf_end].data+header_size, msg->data+cursor, maxpayload_size);
		
		bufbuffered+=1;
		buf_end=(buf_end+1)%(Buffer_Size+1);
	}
	else
	{
		fprintf(stdout, "Even buffer is full now1!!!\n");
	}

	/* move the cursor */
	cursor += maxpayload_size;
    }

    /* send out the last packet */
    if (msg->size > cursor) {

	/*check the buffer first*/
	while(bufbuffered>0)
	{
	
		if(winbuffered<Window_Size)
		{
			//copy packet
			window[next_packet_to_send].data[0] = buffer[buf_begin].data[0];
			window[next_packet_to_send].data[1] = next_packet_to_send;
			memcpy(window[next_packet_to_send].data+header_size, buffer[buf_begin].data+header_size, buffer[buf_begin].data[0]);
			create_checksum(&window[next_packet_to_send]);

			//send packet and set timer
			Sender_ToLowerLayer(&window[next_packet_to_send]);
			if(winbuffered==0)Sender_StartTimer(timeout);

			//change buffer and window cusor
			winbuffered+=1;
			next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
			bufbuffered-=1;
			buf_begin=(buf_begin+1)%(Buffer_Size+1);
		}
		else
		{
			break;
		}
	}
	
	if(winbuffered<Window_Size)
	{
		window[next_packet_to_send].data[0] = msg->size-cursor;
		window[next_packet_to_send].data[1]=next_packet_to_send;
		memcpy(window[next_packet_to_send].data+header_size, msg->data+cursor, window[next_packet_to_send].data[0]);
		create_checksum(&window[next_packet_to_send]);

		//send packet and set timer
		Sender_ToLowerLayer(&window[next_packet_to_send]);
		if(winbuffered==0)Sender_StartTimer(timeout);

		winbuffered+=1;
		next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
	}
	else if(bufbuffered<Buffer_Size)
	{
		buffer[buf_end].data[0]=msg->size-cursor;
		memcpy(buffer[buf_end].data+header_size, msg->data+cursor, buffer[buf_end].data[0]);
		
		bufbuffered+=1;
		buf_end=(buf_end+1)%(Buffer_Size+1);
	}
	else
	{
		fprintf(stdout, "Even buffer is full now2!!!\n");
	}
    }
}

/* event handler, called when a packet is passed from the lower layer at the 
   sender */
void Sender_FromLowerLayer(struct packet *pkt)
{
	int type=(pkt->data[1]>>6)&3;
	int seq=pkt->data[1]&63;
	bool packetsafe=check_checksum(pkt);
	if(type==1&&packetsafe)
	{
		Sender_StopTimer();
		while(between(ack_expected,seq,next_packet_to_send))
		{
			winbuffered-=1;
			ack_expected=(ack_expected+1)%(Real_Window_Size+1);
		}

		int header_size=6;
		while(bufbuffered>0)
		{
	
			if(winbuffered<Window_Size)
			{
				//copy packet
				window[next_packet_to_send].data[0] = buffer[buf_begin].data[0];
				window[next_packet_to_send].data[1] = next_packet_to_send;
				memcpy(window[next_packet_to_send].data+header_size, buffer[buf_begin].data+header_size, buffer[buf_begin].data[0]);
				create_checksum(&window[next_packet_to_send]);

				//send packet and set timer
				Sender_ToLowerLayer(&window[next_packet_to_send]);
				if(winbuffered==0)Sender_StartTimer(timeout);
	
				//change buffer and window cusor
				winbuffered+=1;
				next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
				bufbuffered-=1;
				buf_begin=(buf_begin+1)%(Buffer_Size+1);
			}
			else
			{
				break;
			}
		}

		if(winbuffered>0)Sender_StartTimer(timeout);
	}
}

/* event handler, called when the timer expires */
void Sender_Timeout()
{
	Sender_StopTimer();
	next_packet_to_send=ack_expected;
	for(int i=0;i<winbuffered;++i)
	{
		Sender_ToLowerLayer(&window[next_packet_to_send]);
		next_packet_to_send=(next_packet_to_send+1)%(Real_Window_Size+1);
	}
	if(winbuffered>0)Sender_StartTimer(timeout);
}
