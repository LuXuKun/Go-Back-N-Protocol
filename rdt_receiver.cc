/*
 * FILE: rdt_receiver.cc
 * DESCRIPTION: Reliable data transfer receiver.
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
#include "rdt_receiver.h"

#define Receiver_Window_Size 60
static int packet_expected=0;
static struct packet *ackpacket=(struct packet*)malloc(sizeof(struct packet));


int create_checksum1(struct packet* pkt)
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

bool check_checksum1(struct packet* pkt)
{
	unsigned short* buf=(unsigned short*)(pkt->data);
	buf+=1;
	int* buf1=(int*)buf;
	int sum1=*buf1;
	int sum2=create_checksum1(pkt);
	if(sum1==sum2)return true;
	else return false;
}


/* receiver initialization, called once at the very beginning */
void Receiver_Init()
{
    fprintf(stdout, "At %.2fs: receiver initializing ...\n", GetSimulationTime());
}

/* receiver finalization, called once at the very end.
   you may find that you don't need it, in which case you can leave it blank.
   in certain cases, you might want to use this opportunity to release some 
   memory you allocated in Receiver_init(). */
void Receiver_Final()
{
    fprintf(stdout, "At %.2fs: receiver finalizing ...\n", GetSimulationTime());
}

/* event handler, called when a packet is passed from the lower layer at the 
   receiver */
void Receiver_FromLowerLayer(struct packet *pkt)
{
    /* 6-byte header indicating the size of the payload */
	int header_size = 6;

    /* construct a message and deliver to the upper layer */
    struct message *msg = (struct message*) malloc(sizeof(struct message));
    ASSERT(msg!=NULL);

    msg->size = pkt->data[0];
    /* sanity check in case the packet is corrupted */
    if (msg->size<0) msg->size=0;
    if (msg->size>RDT_PKTSIZE-header_size) msg->size=RDT_PKTSIZE-header_size;

	int type=(pkt->data[1]>>6)&3;
	int seq=(pkt->data[1])&63;
	bool packetsafe=check_checksum1(pkt);
	if(packetsafe)
	{
		if(type==0&&seq==packet_expected)
		{
			msg->data = (char*) malloc(msg->size);
			ASSERT(msg->data!=NULL);
			memcpy(msg->data, pkt->data+header_size, msg->size);
			Receiver_ToUpperLayer(msg);
			
			//send ack to sender
			ackpacket->data[1]=(1<<6)|packet_expected;
			Receiver_ToLowerLayer(ackpacket);
			packet_expected=(packet_expected+1)%(Receiver_Window_Size+1);
	
			/* don't forget to free the space */
			if (msg->data!=NULL) free(msg->data);
			if (msg!=NULL) free(msg);
		}
		else
		{
			//send ack to sender
			int last_ack=packet_expected;
			if(last_ack>0)
			{
				last_ack=last_ack-1;
			}
			else
			{
				last_ack=Receiver_Window_Size;
			}
			ackpacket->data[0]=0;
			ackpacket->data[1]=(1<<6)|last_ack;
			create_checksum1(ackpacket);

			Receiver_ToLowerLayer(ackpacket);
		}
	}
}
