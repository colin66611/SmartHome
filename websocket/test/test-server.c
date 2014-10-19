/*
 * libwebsockets-test-server - libwebsockets test implementation
 *
 * Copyright (C) 2010-2011 Andy Green <andy@warmcat.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation:
 *  version 2.1 of the License.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA  02110-1301  USA
 */
#ifdef CMAKE_BUILD
#include "lws_config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#ifdef WIN32

#ifdef EXTERNAL_POLL
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <stddef.h>

	#include "websock-w32.h"
#endif

#else // NOT WIN32
#include <syslog.h>
#endif

#include <signal.h>

#include "libwebsockets.h"
//#include "private-libwebsockets.h"
#include "os_types.h"
#include <pthread.h>


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#include "hal_gpio_if.h"
#include "ipcl_gpio.h"



typedef struct {
    int argc;
    char** argv;
} proc_args_t;

static int close_testing;
int max_poll_elements;

struct pollfd *pollfds;
int *fd_lookup;
int count_pollfds;
int force_exit = 0;

/*
 * This demo server shows how to use libwebsockets for one or more
 * websocket protocols in the same server
 *
 * It defines the following websocket protocols:
 *
 *  dumb-increment-protocol:  once the socket is opened, an incrementing
 *				ascii string is sent down it every 50ms.
 *				If you send "reset\n" on the websocket, then
 *				the incrementing number is reset to 0.
 *
 *  lws-mirror-protocol: copies any received packet to every connection also
 *				using this protocol, including the sender
 */

enum demo_protocols {
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_DUMB_INCREMENT,
	PROTOCOL_LWS_MIRROR,

	/* always last */
	DEMO_PROTOCOL_COUNT
};

unsigned16 sec(void)
{
	struct timeval tv = {0};
	struct timezone tz = {0};
	gettimeofday(&tv, &tz);
	return (tv.tv_sec % 60);
}
unsigned16 msec(void)
{
	struct timeval tv = {0};
	struct timezone tz = {0};
	gettimeofday(&tv, &tz);
	return (tv.tv_usec/1000);
}
void close_operation(unsigned8 port, unsigned8 * operation, int index);
void open_operation(unsigned8 port, unsigned8 * operation, int index);

//#define LOCAL_RESOURCE_PATH "./libwebsockets-test-server"
#define LOCAL_RESOURCE_PATH "."
char *resource_path = LOCAL_RESOURCE_PATH;



#define VMF_SEND_DEBUG	1
#define PRESET_NUM		4
#define BAND_NUM		3

#define STATION_LEN		4
#define PRESET_INFO_LEN	16
#define VOL_LEN			4
#define MSG_INFO_LEN    1
#define MSG_STATION_OPERATION_LEN      4
#define MAX_VOL			31
#define MIN_VOL			0
#define CLOUD_RADIO_DEBUG(...)		{printf("[%2d.%03d]Cloud:", sec(), msec());printf(__VA_ARGS__);printf("\n");}
#define CLOUD_RADIO_VMF(...)		{printf(__VA_ARGS__);}
#define CLOUD_RADIO_INFO(...)		{printf("[%2d.%03d]Cloud:", sec(), msec());printf(__VA_ARGS__);printf("\n");}
#define CLOUD_RADIO_ERROR(...)		{printf("[%2d.%03d]Cloud:", sec(), msec());printf(__VA_ARGS__);printf("\n\n");}
#define MY_GROUP		220
#define ONE_MS			1000
#define SYNC_TIME_INTERVAL_S		(1000*ONE_MS)
#define MAX_FRAME		512
#define MAX_IPCL_FRAME	1024

/*GPIO pin num*/
#define GPJ0_0	(172)		//curtain1
#define GPJ0_1	(173)		//curtain2
#define GPJ0_2	(174)		//light1
#define GPJ0_3	(175)		//light2
#define GPJ0_4	(176)		//light3
#define GPJ0_5	(177)		//light4
#define GPJ0_6	(178)		//other appliances

enum
{
	CURTAIN1_BIT,
	CURTAIN2_BIT,
	LIGHT1_BIT,
	LIGHT2_BIT,
	LIGHT3_BIT,
	LIGHT4_BIT,
	NUM_OF_GPIO
};

#define SET_BIT(x, bit)					((x)|(1<<bit))
#define CLEAR_BIT(x, bit)				((x)&(~(1<<bit)))
#define REVERSE_BIT(x, bit)				((x)^(1<<bit))
#define CHECK_BIT(x, bit)				(((x)>>bit)&1)

/*GPIO STATE FLAGS*/
static bool curtain_one_opened = false;
static bool curtain_two_opened = false;
static bool light_one_opened = false;
static bool light_two_opened = false;
static bool light_three_opened = false;
static bool light_four_opened = false;

static unsigned16 gpio_state = 0;
static unsigned8	test = 0;
static unsigned char *cls_str[6] = 
{
	"ClsCur1",
	"ClsCur2",
	"ClsLight1",
	"ClsLight2",
	"ClsLight3",
	"ClsLight4"
};

static unsigned char *opn_str[6] = 
{
	"OpnCur1",
	"OpnCur2",
	"OpnLight1",
	"OpnLight2",
	"OpnLight3",
	"OpnLight4"
};

#define CAN_MSG_PRINT	0

unsigned8 gpio_handle1, gpio_handle2, gpio_handle3, gpio_handle4, gpio_handle5, gpio_handle6;

typedef enum
{
	AMFM_BANK_CURRENT = 0,										// Select current bank (Used with a specific g_u32PresetFreq)
	AMFM_BANK_FM_1,
	AMFM_BANK_FM_2,
	AMFM_BANK_AM,
	AMFM_BANK_UP,												// Select next bank
	AMFM_BANK_DOWN												// Select previous bank
} AMFM_BANK_TYPE;

/* Message queue structure format */
typedef struct
{
	long mtype;
	unsigned32 length;
	char msg[MAX_FRAME];
}msg_queue;

/*msg queue id*/
int tx_msg_id, rx_msg_id;

unsigned8 					g_u8BandBack = AMFM_BANK_CURRENT;



/*
 * We take a strict whitelist approach to stop ../ attacks
 */

struct serveable {
	const char *urlpath;
	const char *mimetype;
}; 

static const struct serveable whitelist[] = {
	{ "/icon/Next_Tuner-06.png", "image/png" },
	{ "/icon/Next_Tuner-Glow.png", "image/png" },
	{ "/icon/06_UL.png", "image/png" },
	{ "/icon/07_UL.png", "image/png" },
	{ "/icon/10_UL.png", "image/png" },
	{ "/icon/Tuner_Band-06.png", "image/png" },
	{ "/icon/Scrollbar_Bank_BG.png", "image/png" },
	{ "/icon/Background_Tuner.png", "image/png" },
	{ "/icon/Tuner_Band-Glow.png", "image/png" },
	{ "/icon/Previous_Tuner-Glow.png", "image/png" },
	{ "/icon/Previous_Tuner-06.png", "image/png" },
	{ "/icon/Backward-Glow.png", "image/png" },
	{ "/icon/Forward-Glow.png", "image/png" },

	/* last one is the default served if no match */
	{ "/qhetest.html", "text/html" },
};

struct per_session_data__http {
	int fd;
};
#if 0
static unsigned8 msg_ready_to_send()
{
	if(vmf_msg_ready == true)
	{
		return 1;
	}
	else
	{
		return 0;
	}
	
}
static void require_msg_send()
{
	vmf_msg_ready = true;
}
static void msg_finish_sending()
{
	vmf_msg_ready = false;
#endif
static int callback_http(struct libwebsocket_context *context,
		struct libwebsocket *wsi,
		enum libwebsocket_callback_reasons reason, void *user,
							   void *in, size_t len)
{
#if 0
	char client_name[128];
	char client_ip[128];
#endif
	char buf[256];
	char leaf_path[1024];
	int n, m;
	unsigned char *p;
	static unsigned char buffer[4096];
	struct stat stat_buf;
	struct per_session_data__http *pss =
			(struct per_session_data__http *)user;
#ifdef EXTERNAL_POLL
	int fd = (int)(long)in;
#endif

	switch (reason) {
	case LWS_CALLBACK_HTTP:
		CLOUD_RADIO_DEBUG("LWS_CALLBACK_HTTP.\n");
		/* check for the "send a big file by hand" example case */

		if (!strcmp((const char *)in, "/leaf.jpg")) {
			if (strlen(resource_path) > sizeof(leaf_path) - 10)
				return -1;
			sprintf(leaf_path, "%s/leaf.jpg", resource_path);

			/* well, let's demonstrate how to send the hard way */

			p = buffer;

#ifdef WIN32
			pss->fd = open(leaf_path, O_RDONLY | _O_BINARY);
#else
			pss->fd = open(leaf_path, O_RDONLY);
#endif

			if (pss->fd < 0)
				return -1;

			fstat(pss->fd, &stat_buf);

			/*
			 * we will send a big jpeg file, but it could be
			 * anything.  Set the Content-Type: appropriately
			 * so the browser knows what to do with it.
			 */

			p += sprintf((char *)p,
				"HTTP/1.0 200 OK\x0d\x0a"
				"Server: libwebsockets\x0d\x0a"
				"Content-Type: image/jpeg\x0d\x0a"
					"Content-Length: %u\x0d\x0a\x0d\x0a",
					(unsigned int)stat_buf.st_size);

			/*
			 * send the http headers...
			 * this won't block since it's the first payload sent
			 * on the connection since it was established
			 * (too small for partial)
			 */

			n = libwebsocket_write(wsi, buffer,
				   p - buffer, LWS_WRITE_HTTP);

			if (n < 0) {
				close(pss->fd);
				return -1;
			}
			/*
			 * book us a LWS_CALLBACK_HTTP_WRITEABLE callback
			 */
			libwebsocket_callback_on_writable(context, wsi);
			break;
		}

		/* if not, send a file the easy way */

		for (n = 0; n < (sizeof(whitelist) / sizeof(whitelist[0]) - 1); n++)
			if (in && strcmp((const char *)in, whitelist[n].urlpath) == 0)
				break;

		sprintf(buf, "%s%s", resource_path, whitelist[n].urlpath);
		CLOUD_RADIO_DEBUG("tranmit:%s to web.\n", buf);
		CLOUD_RADIO_DEBUG("web send in %s.\n", (const char *)in);

		if (libwebsockets_serve_http_file(context, wsi, buf, whitelist[n].mimetype))
			return -1; /* through completion or error, close the socket */

		/*
		 * notice that the sending of the file completes asynchronously,
		 * we'll get a LWS_CALLBACK_HTTP_FILE_COMPLETION callback when
		 * it's done
		 */

		break;

	case LWS_CALLBACK_HTTP_FILE_COMPLETION:
//		lwsl_info("LWS_CALLBACK_HTTP_FILE_COMPLETION seen\n");
		/* kill the connection after we sent one file */
		return -1;

	case LWS_CALLBACK_HTTP_WRITEABLE:
		/*
		 * we can send more of whatever it is we were sending
		 */

		do {
			n = read(pss->fd, buffer, sizeof buffer);
			/* problem reading, close conn */
			if (n < 0)
				goto bail;
			/* sent it all, close conn */
			if (n == 0)
				goto bail;
			/*
			 * because it's HTTP and not websocket, don't need to take
			 * care about pre and postamble
			 */
			m = libwebsocket_write(wsi, buffer, n, LWS_WRITE_HTTP);
			if (m < 0)
				/* write failed, close conn */
				goto bail;
			if (m != n)
				/* partial write, adjust */
				lseek(pss->fd, m - n, SEEK_CUR);

		} while (!lws_send_pipe_choked(wsi));
		libwebsocket_callback_on_writable(context, wsi);
		break;

bail:
		close(pss->fd);
		return -1;

	/*
	 * callback for confirming to continue with client IP appear in
	 * protocol 0 callback since no websocket protocol has been agreed
	 * yet.  You can just ignore this if you won't filter on client IP
	 * since the default uhandled callback return is 0 meaning let the
	 * connection continue.
	 */

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
#if 0
		libwebsockets_get_peer_addresses(context, wsi, (int)(long)in, client_name,
			     sizeof(client_name), client_ip, sizeof(client_ip));

		fprintf(stderr, "Received network connect from %s (%s)\n",
							client_name, client_ip);
#endif
		/* if we returned non-zero from here, we kill the connection */
		break;

#ifdef EXTERNAL_POLL
	/*
	 * callbacks for managing the external poll() array appear in
	 * protocol 0 callback
	 */

	case LWS_CALLBACK_ADD_POLL_FD:

		if (count_pollfds >= max_poll_elements) {
			lwsl_err("LWS_CALLBACK_ADD_POLL_FD: too many sockets to track\n");
			return 1;
		}

		fd_lookup[fd] = count_pollfds;
		pollfds[count_pollfds].fd = fd;
		pollfds[count_pollfds].events = (int)(long)len;
		pollfds[count_pollfds++].revents = 0;
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		if (!--count_pollfds)
			break;
		m = fd_lookup[fd];
		/* have the last guy take up the vacant slot */
		pollfds[m] = pollfds[count_pollfds];
		fd_lookup[pollfds[count_pollfds].fd] = m;
		break;

	case LWS_CALLBACK_SET_MODE_POLL_FD:
		pollfds[fd_lookup[fd]].events |= (int)(long)len;
		break;

	case LWS_CALLBACK_CLEAR_MODE_POLL_FD:
		pollfds[fd_lookup[fd]].events &= ~(int)(long)len;
		break;
#endif

	default:
		break;
	}

	return 0;
}

/*
 * this is just an example of parsing handshake headers, you don't need this
 * in your code unless you will filter allowing connections by the header
 * content
 */

static void
dump_handshake_info(struct libwebsocket *wsi)
{
	int n;
	static const char *token_names[WSI_TOKEN_COUNT] = {
		/*[WSI_TOKEN_GET_URI]		=*/ "GET URI",
		/*[WSI_TOKEN_HOST]		=*/ "Host",
		/*[WSI_TOKEN_CONNECTION]	=*/ "Connection",
		/*[WSI_TOKEN_KEY1]		=*/ "key 1",
		/*[WSI_TOKEN_KEY2]		=*/ "key 2",
		/*[WSI_TOKEN_PROTOCOL]		=*/ "Protocol",
		/*[WSI_TOKEN_UPGRADE]		=*/ "Upgrade",
		/*[WSI_TOKEN_ORIGIN]		=*/ "Origin",
		/*[WSI_TOKEN_DRAFT]		=*/ "Draft",
		/*[WSI_TOKEN_CHALLENGE]		=*/ "Challenge",

		/* new for 04 */
		/*[WSI_TOKEN_KEY]		=*/ "Key",
		/*[WSI_TOKEN_VERSION]		=*/ "Version",
		/*[WSI_TOKEN_SWORIGIN]		=*/ "Sworigin",

		/* new for 05 */
		/*[WSI_TOKEN_EXTENSIONS]	=*/ "Extensions",

		/* client receives these */
		/*[WSI_TOKEN_ACCEPT]		=*/ "Accept",
		/*[WSI_TOKEN_NONCE]		=*/ "Nonce",
		/*[WSI_TOKEN_HTTP]		=*/ "Http",
		/*[WSI_TOKEN_MUXURL]	=*/ "MuxURL",
	};
	char buf[256];

	for (n = 0; n < WSI_TOKEN_COUNT; n++) {
		if (!lws_hdr_total_length(wsi, n))
			continue;

		lws_hdr_copy(wsi, buf, sizeof buf, n);

		fprintf(stderr, "    %s = %s\n", token_names[n], buf);
	}
}

/* dumb_increment protocol */

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 *
 * for this example protocol we use it to individualize the count for each
 * connection.
 */

struct per_session_data__dumb_increment {

	struct libwebsocket *wsi;
	unsigned16 state;
	unsigned8 temp;
};

static int
callback_dumb_increment(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	int n, m;
	unsigned char buf[LWS_SEND_BUFFER_PRE_PADDING + 512 +
						  LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *p = &buf[LWS_SEND_BUFFER_PRE_PADDING];
	struct per_session_data__dumb_increment *pss = (struct per_session_data__dumb_increment *)user;

	unsigned8 vmf_msg_receive[20];
	unsigned8 payload[20];
    char vmf_group_id;
    char vmf_msg_id;
    unsigned8 vmf_msg_len;
	unsigned8 i;

	char *pt;
	
	
	struct msqid_ds attr;
	unsigned32 tx_msg_num = 0;
	unsigned32 tx_length = 0;
	msg_queue tx_buf;

	static int msg_rcv_debug = 0;

	/*message queue*/
	msg_queue tx;
	static char send_buf[20];
	memset(send_buf, 0, sizeof(send_buf));

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_dumb_increment: "
						 "LWS_CALLBACK_ESTABLISHED\n");
		CLOUD_RADIO_INFO("fds_count is %d. fd of the connection is %d.wsi is %x.\n", context->fds_count, context->lws_lookup[context->fds[i].fd], wsi);
		pss->state = 0;
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
#if 0
		//CLOUD_RADIO_DEBUG("Enter writable callback.");
		//printf("wsi is 0%x, wsi_addr is 0%x.\n", wsi, wsi_addr);
		/* Receive the messages from the queue */
		if (msgrcv (tx_msg_id, &tx_buf, sizeof(tx_buf), 1, IPC_NOWAIT) == -1)
		{/* Rx msg queue access error */
			if(errno != ENOMSG)
			{
				CLOUD_RADIO_ERROR("Msg queue data receive failed. Errno is %d(%s).", errno, strerror(errno));
			}
			break;
		}
		else
		{
			n = tx_buf.length;
			strcpy(p, tx_buf.msg);

			CLOUD_RADIO_DEBUG(">>>>>>>>>>send %s to first web.", p);
			for(i = 0; i < context->fds_count; i++)
			{
				m = libwebsocket_write(context->lws_lookup[context->fds[i].fd], p, n, LWS_WRITE_TEXT);
				CLOUD_RADIO_ERROR(">>>>>>>>>>send %s to first wsi: %x.", p, wsi);
			}
		}
		//libwebsocket_rx_flow_allow_all_protocol(libwebsockets_get_protocol(wsi));
#endif
		//printf("wsi is %x.temp is %d.\n", wsi, pss->temp);
#if 0
		if(pss->temp != test)
		{
			sprintf(send_buf, "test%d", test);
			pss->temp = test;
			printf("send %s to web %ld.\n", send_buf, wsi);
			m = libwebsocket_write(wsi, send_buf, strlen(send_buf), LWS_WRITE_TEXT);
			CLOUD_RADIO_ERROR(">>>>>>>>>>send %s to first wsi: %x.", send_buf, wsi);
		}
#endif
#if 1
		//TBD: compare pss and global bool variable and write socket
		for(i = 0; i < NUM_OF_GPIO; i++)
		{
			if(CHECK_BIT(pss->state, i) != CHECK_BIT(gpio_state, i))
			{
				CLOUD_RADIO_INFO("Bit %d is different.", i);
				printf("state now is %d.\n", pss->state);
				pss->state = REVERSE_BIT(pss->state, i);
				if(CHECK_BIT(pss->state, i))
				{
					sprintf(send_buf, opn_str[i]);
					CLOUD_RADIO_INFO("prepare to send %s to wsi %x.", send_buf, wsi);
					//m = libwebsocket_write(wsi, "ClsCur1", 7, LWS_WRITE_TEXT);
					m = libwebsocket_write(wsi, send_buf, strlen(send_buf), LWS_WRITE_TEXT);
					if(m<=0)
					{
						m = libwebsocket_write(wsi, send_buf, strlen(send_buf), LWS_WRITE_TEXT);
					}
					CLOUD_RADIO_INFO(">>>>>>>>>>send %s to wsi: %x, return %d.", opn_str[i], wsi, m);
				}
				else
				{
					sprintf(send_buf, cls_str[i]);
					CLOUD_RADIO_INFO("prepare to send %s to wsi %x.", send_buf, wsi);
					//m = libwebsocket_write(wsi, "OpnCur1", 7, LWS_WRITE_TEXT);
					m = libwebsocket_write(wsi, send_buf, strlen(send_buf), LWS_WRITE_TEXT);
					CLOUD_RADIO_INFO(">>>>>>>>>>send %s to wsi: %x, return %d.", cls_str[i], wsi, m);
					if(m<=0)
					{
						m = libwebsocket_write(wsi, send_buf, strlen(send_buf), LWS_WRITE_TEXT);
					}

				}

				
				
			}
		}
#endif		
		libwebsocket_rx_flow_allow_all_protocol(libwebsockets_get_protocol(wsi));
		
#if 1
		if (lws_send_pipe_choked(wsi)) {
			libwebsocket_callback_on_writable(context, wsi);
			CLOUD_RADIO_DEBUG("libwebsocket_callback_on_writable.");
			break;
		}
		
#endif

	
		break;
	

	case LWS_CALLBACK_RECEIVE:


		CLOUD_RADIO_DEBUG("receive msg is %s from wsi %x.", (const char *)in, wsi);

		if(strcmp((const char *)in, "ClsCur1") == 0)
		{
			test++;
			pss->state = CLEAR_BIT(pss->state, 0);
			printf("state now is %d.\n", pss->state);
			CLOUD_RADIO_DEBUG("Receive ClsCur1 command.");
			close_operation(GPJ0_0, "ClsCur1", CURTAIN1_BIT);
		}
		else if(strcmp((const char *)in, "OpnCur1") == 0)
		{
			pss->state = SET_BIT(pss->state, 0);
			printf("state now is %d.\n", pss->state);
			CLOUD_RADIO_DEBUG("Receive OpnCur1 command.");
			open_operation(GPJ0_0, "OpnCur1", CURTAIN1_BIT);
		}		

		else if(strcmp((const char *)in, "ClsCur2") == 0)
		{
			pss->state = CLEAR_BIT(pss->state, 1);
			CLOUD_RADIO_DEBUG("Receive ClsCur2 command.");
			close_operation(GPJ0_1, "ClsCur2", CURTAIN2_BIT);
		}		

		else if(strcmp((const char *)in, "OpnCur2") == 0)
		{
			pss->state = SET_BIT(pss->state, 1);
			CLOUD_RADIO_DEBUG("Receive ClsCur2 command.");
			open_operation(GPJ0_1, "OpnCur2", CURTAIN2_BIT);
		}		
		else if(strcmp((const char *)in, "ClsLight1") == 0)
		{
			pss->state = CLEAR_BIT(pss->state, 2);
			CLOUD_RADIO_DEBUG("Receive ClsLight1 command.");
			close_operation(GPJ0_2, "ClsLight1", LIGHT1_BIT);
		}
		else if(strcmp((const char *)in, "OpnLight1") == 0)
		{
			pss->state = SET_BIT(pss->state, 2);
			CLOUD_RADIO_DEBUG("Receive OpnLight1 command.");
			open_operation(GPJ0_2, "OpnLight1", LIGHT1_BIT);
		}
		else if(strcmp((const char *)in, "ClsLight2") == 0)
		{
			pss->state = CLEAR_BIT(pss->state, 3);
			CLOUD_RADIO_DEBUG("Receive ClsLight2 command.");
			close_operation(GPJ0_3, "ClsLight2", LIGHT2_BIT);
		}
		else if(strcmp((const char *)in, "OpnLight2") == 0)
		{
			pss->state = SET_BIT(pss->state, 3);
			CLOUD_RADIO_DEBUG("Receive OpnLight2 command.");
			open_operation(GPJ0_3, "OpnLight2", LIGHT2_BIT);
		}
		else if(strcmp((const char *)in, "ClsLight3") == 0)
		{
			pss->state = CLEAR_BIT(pss->state, 4);
			CLOUD_RADIO_DEBUG("Receive ClsLight3 command.");
			close_operation(GPJ0_4, "ClsLight3", LIGHT3_BIT);
		}
		else if(strcmp((const char *)in, "OpnLight3") == 0)
		{
			pss->state = SET_BIT(pss->state, 4);
			CLOUD_RADIO_DEBUG("Receive OpnLight3 command.");
			open_operation(GPJ0_4, "OpnLight3", LIGHT3_BIT);
		}
		else if(strcmp((const char *)in, "ClsLight4") == 0)
		{
			pss->state = CLEAR_BIT(pss->state, 5);
			CLOUD_RADIO_DEBUG("Receive ClsLight4 command.");
			close_operation(GPJ0_5, "ClsLight4", LIGHT4_BIT);
		}
		else if(strcmp((const char *)in, "OpnLight4") == 0)
		{
			pss->state = SET_BIT(pss->state, 5);
			CLOUD_RADIO_DEBUG("Receive OpnLight4 command.");
			open_operation(GPJ0_5, "OpnLight4", LIGHT4_BIT);
		}
		else if(strcmp((const char *)in, "Sync") == 0)
		{
			pss->state = 0;
			CLOUD_RADIO_DEBUG("Receive Sync command.");
		}
		else
		{
			CLOUD_RADIO_DEBUG("Other message.");
		}
#		if 0
		else if(strcmp((const char *)in, "FreqFwd") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSeek;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 0;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive frequency_forward command.");
		}
		else if(strcmp((const char *)in, "SeekUp") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSeek;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 4;
			payload[1] = 4;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive SeekUp command.");
		}
		else if(strcmp((const char *)in, "SeekDown") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSeek;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 3;
			payload[1] = 3;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive SeekDown command.");
		}
		else if(strcmp((const char *)in, "Store1") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInStoreStation;
			vmf_msg_len = MSG_STATION_OPERATION_LEN;
			payload[0] = 1;
			payload[1] = 0;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Store1 command.");
		}
		else if(strcmp((const char *)in, "Store2") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInStoreStation;
			vmf_msg_len = MSG_STATION_OPERATION_LEN;
			payload[0] = 2;
			payload[1] = 0;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Store2 command.");
		}
		else if(strcmp((const char *)in, "Store3") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInStoreStation;
			vmf_msg_len = MSG_STATION_OPERATION_LEN;
			payload[0] = 3;
			payload[1] = 0;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Store3 command.");
		}
		else if(strcmp((const char *)in, "Store4") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInStoreStation;
			vmf_msg_len = MSG_STATION_OPERATION_LEN;
			payload[0] = 4;
			payload[1] = 0;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Store4 command.");
		}
		else if(strcmp((const char *)in, "Select1") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSelectStation;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 1;
			payload[1] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Select1 command.");
		}
		else if(strcmp((const char *)in, "Select2") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSelectStation;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 2;
			payload[1] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Select2 command.");
		}
		else if(strcmp((const char *)in, "Select3") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSelectStation;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 3;
			payload[1] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Select3 command.");
		}
		else if(strcmp((const char *)in, "Select4") == 0)
		{
			vmf_group_id = AMFM_IN;
			vmf_msg_id = AmfmInSelectStation;
			vmf_msg_len = MSG_INFO_LEN;
			payload[0] = 4;
			payload[1] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive Select4 command.");
		}
		else if(strcmp((const char *)in, "VolDown") == 0)
		{
			vmf_group_id = AV_AM_SERVICE_GROUP;
			vmf_msg_id = AV_AM_REQUEST_EV;
			vmf_msg_len = VOL_LEN;
			payload[0] = 2;
			payload[1] = 115;
			payload[2] = 1;
			payload[3] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive VolDown command.");
		}
		else if(strcmp((const char *)in, "VolUp") == 0)
		{
			vmf_group_id = AV_AM_SERVICE_GROUP;
			vmf_msg_id = AV_AM_REQUEST_EV;
			vmf_msg_len = VOL_LEN;
			payload[0] = 2;
			payload[1] = 115;
			payload[2] = 0;
			payload[3] = 1;
			require_msg_send();
			CLOUD_RADIO_DEBUG("Receive VolUp command.");
		}
		else
		{
			CLOUD_RADIO_ERROR("Receive wrong msg %s.", (const char *)in);
		}
#endif
		
		//memcpy(payload, &((unsigned char *)in)[4], 16);

		//DEBUG("GroupID is %d, MsgID is %d.\n", vmf_group_id, vmf_msg_id);

		break;
	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}

/* lws-mirror_protocol */

#define MAX_MESSAGE_QUEUE 32

struct per_session_data__lws_mirror {
	struct libwebsocket *wsi;
	int ringbuffer_tail;
};

struct a_message {
	void *payload;
	size_t len;
};

static struct a_message ringbuffer[MAX_MESSAGE_QUEUE];
static int ringbuffer_head;

static int
callback_lws_mirror(struct libwebsocket_context *context,
			struct libwebsocket *wsi,
			enum libwebsocket_callback_reasons reason,
					       void *user, void *in, size_t len)
{
	int n;
	struct per_session_data__lws_mirror *pss = (struct per_session_data__lws_mirror *)user;

	switch (reason) {

	case LWS_CALLBACK_ESTABLISHED:
		lwsl_info("callback_lws_mirror: LWS_CALLBACK_ESTABLISHED\n");
		pss->ringbuffer_tail = ringbuffer_head;
		pss->wsi = wsi;
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		lwsl_notice("mirror protocol cleaning up\n");
		for (n = 0; n < sizeof ringbuffer / sizeof ringbuffer[0]; n++)
			if (ringbuffer[n].payload)
				free(ringbuffer[n].payload);
		break;

	case LWS_CALLBACK_SERVER_WRITEABLE:
		if (close_testing)
			break;
		while (pss->ringbuffer_tail != ringbuffer_head) {

			n = libwebsocket_write(wsi, (unsigned char *)
				   ringbuffer[pss->ringbuffer_tail].payload +
				   LWS_SEND_BUFFER_PRE_PADDING,
				   ringbuffer[pss->ringbuffer_tail].len,
								LWS_WRITE_TEXT);
			if (n < ringbuffer[pss->ringbuffer_tail].len) {
				lwsl_err("ERROR %d writing to mirror socket\n", n);
				return -1;
			}
			if (n < ringbuffer[pss->ringbuffer_tail].len)
				lwsl_err("mirror partial write %d vs %d\n",
				       n, ringbuffer[pss->ringbuffer_tail].len);

			if (pss->ringbuffer_tail == (MAX_MESSAGE_QUEUE - 1))
				pss->ringbuffer_tail = 0;
			else
				pss->ringbuffer_tail++;

			if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 15))
				libwebsocket_rx_flow_allow_all_protocol(
					       libwebsockets_get_protocol(wsi));

			// lwsl_debug("tx fifo %d\n", (ringbuffer_head - pss->ringbuffer_tail) & (MAX_MESSAGE_QUEUE - 1));

			if (lws_send_pipe_choked(wsi)) {
				libwebsocket_callback_on_writable(context, wsi);
				break;
			}
			/*
			 * for tests with chrome on same machine as client and
			 * server, this is needed to stop chrome choking
			 */
			usleep(1);
		}
		break;

	case LWS_CALLBACK_RECEIVE:

		if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) == (MAX_MESSAGE_QUEUE - 1)) {
			lwsl_err("dropping!\n");
			goto choke;
		}

		if (ringbuffer[ringbuffer_head].payload)
			free(ringbuffer[ringbuffer_head].payload);

		ringbuffer[ringbuffer_head].payload =
				malloc(LWS_SEND_BUFFER_PRE_PADDING + len +
						  LWS_SEND_BUFFER_POST_PADDING);
		ringbuffer[ringbuffer_head].len = len;
		memcpy((char *)ringbuffer[ringbuffer_head].payload +
					  LWS_SEND_BUFFER_PRE_PADDING, in, len);
		if (ringbuffer_head == (MAX_MESSAGE_QUEUE - 1))
			ringbuffer_head = 0;
		else
			ringbuffer_head++;

		if (((ringbuffer_head - pss->ringbuffer_tail) &
				  (MAX_MESSAGE_QUEUE - 1)) != (MAX_MESSAGE_QUEUE - 2))
			goto done;

choke:
		lwsl_debug("LWS_CALLBACK_RECEIVE: throttling %p\n", wsi);
		libwebsocket_rx_flow_control(wsi, 0);

//		lwsl_debug("rx fifo %d\n", (ringbuffer_head - pss->ringbuffer_tail) & (MAX_MESSAGE_QUEUE - 1));
done:
		libwebsocket_callback_on_writable_all_protocol(
					       libwebsockets_get_protocol(wsi));
		break;

	/*
	 * this just demonstrates how to use the protocol filter. If you won't
	 * study and reject connections based on header content, you don't need
	 * to handle this callback
	 */

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		dump_handshake_info(wsi);
		/* you could return non-zero here and kill the connection */
		break;

	default:
		break;
	}

	return 0;
}


/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */

	{
		"http-only",		/* name */
		callback_http,		/* callback */
		sizeof (struct per_session_data__http),	/* per_session_data_size */
		0,			/* max frame size / rx buffer */
	},
	{
		"dumb-increment-protocol",
		callback_dumb_increment,
		sizeof(struct per_session_data__dumb_increment),
		20,
	},
	{
		"lws-mirror-protocol",
		callback_lws_mirror,
		sizeof(struct per_session_data__lws_mirror),
		128,
	},
	{ NULL, NULL, 0, 0 } /* terminator */
};

void sighandler(int sig)
{
	force_exit = 1;
	msgctl(tx_msg_id, IPC_RMID, NULL);
	/* Destroy the rx message queue*/
	msgctl(rx_msg_id, IPC_RMID, NULL);
	CLOUD_RADIO_DEBUG("Server exit safety.\n");
}

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "debug",	required_argument,	NULL, 'd' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "interface",  required_argument,	NULL, 'i' },
	{ "closetest",  no_argument,		NULL, 'c' },
#ifndef LWS_NO_DAEMONIZE
	{ "daemonize", 	no_argument,		NULL, 'D' },
#endif
	{ "resource_path", required_argument,		NULL, 'r' },
	{ NULL, 0, 0, 0 }
};


static int hardware_init()
{
	int ret = -1;
#if 0
	hal_gpio_init();

	ret = hal_gpio_open(&gpio_handle1, GPJ0_0, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_0 opened failed. return %d.\n", ret);
	}
	ret = hal_gpio_open(&gpio_handle2, GPJ0_1, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_1 opened failed. return %d.\n", ret);
	}
	ret = hal_gpio_open(&gpio_handle3, GPJ0_2, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_2 opened failed. return %d.\n", ret);
	}
	ret = hal_gpio_open(&gpio_handle4, GPJ0_3, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_3 opened failed. return %d.\n", ret);
	}
	ret = hal_gpio_open(&gpio_handle5, GPJ0_4, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_4 opened failed. return %d.\n", ret);
	}
	ret = hal_gpio_open(&gpio_handle6, GPJ0_5, HAL_GPIO_DIRECTION_OUTPUT, 0, NULL);
	if(ret != HAL_STATUS_OK)
	{
		CLOUD_RADIO_DEBUG("GPJ0_5 opened failed. return %d.\n", ret);
	}
#else
	ret = gpio_open(GPJ0_0, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_0 opened failed. return %d.\n", ret);
	}
	ret = gpio_write(GPJ0_0, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}
	
	ret = gpio_open(GPJ0_1, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_1 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_1, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

	ret = gpio_open(GPJ0_2, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_2 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_2, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

	ret = gpio_open(GPJ0_3, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_3 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_3, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

	ret = gpio_open(GPJ0_4, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_3 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_4, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

	ret = gpio_open(GPJ0_5, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_3 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_5, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

	ret = gpio_open(GPJ0_6, GPIO_DIRECTION_OUTPUT);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("GPJ0_3 opened failed. return %d.\n", ret);
	}	ret = gpio_write(GPJ0_6, HAL_GPIO_STATE_HIGH);
	if(ret < 0)
	{
		CLOUD_RADIO_DEBUG("gpio_write GPJ0 return %d.", ret);
	}

#endif
}

void open_operation(unsigned8 port, unsigned8 * operation, int index)
{
	static char send_buf[20];
	msg_queue tx;
	
	if(!(CHECK_BIT(gpio_state, index)))		//false when it need close
	{
		if(gpio_write(port, HAL_GPIO_STATE_HIGH) > 0)
		{
			gpio_state = SET_BIT(gpio_state, index);
			CLOUD_RADIO_DEBUG("Set %d bit.", index);

		}
	}
	else
	{
		CLOUD_RADIO_DEBUG("Don't need to open again.");
	}
}

void close_operation(unsigned8 port, unsigned8 * operation, int index)
{
	static char send_buf[20];
	msg_queue tx;
	
	if(CHECK_BIT(gpio_state, index))		//true when it need close
	{
		if(gpio_write(port, HAL_GPIO_STATE_LOW) > 0)
		{
			gpio_state = CLEAR_BIT(gpio_state, index);
			CLOUD_RADIO_DEBUG("Clear %d bit.", index);
		}
	}
	else
	{
		CLOUD_RADIO_DEBUG("Don't need to close again.");
	}
}

int main(int argc, char **argv)
{
	char cert_path[1024];
	char key_path[1024];
	int n = 0;
	int use_ssl = 0;
	struct libwebsocket_context *context;
	int opts = 0;
	char interface_name[128] = "";
	const char *iface = NULL;
#ifndef WIN32
	int syslog_options = LOG_PID | LOG_PERROR;
#endif
	unsigned int oldus = 0;
	struct lws_context_creation_info info;

	int debug_level = 7;
#ifndef LWS_NO_DAEMONIZE
	int daemonize = 0;
#endif
	pthread_t sync_thread;
	proc_args_t argument;

	//vmf_init();
	hardware_init();
	CLOUD_RADIO_DEBUG("hardware_init.");

	/* Message queue for the data */
	if((tx_msg_id = msgget(tx_msg_id, 0655 | IPC_CREAT | IPC_EXCL | IPC_NOWAIT)) == -1)
	{
		CLOUD_RADIO_ERROR("CAN Tx message queue create failed.");
		CLOUD_RADIO_ERROR("errno is %d(%s).", errno, strerror(errno));
	}
	else
	{
		CLOUD_RADIO_DEBUG("CAN Tx message queue create success.");
		CLOUD_RADIO_DEBUG("tx_msg_id is %d.", tx_msg_id);
	}

	/* Message queue for the data */
	if((rx_msg_id = msgget(rx_msg_id, 0655 | IPC_CREAT | IPC_EXCL | IPC_NOWAIT)) == -1)
	{
		CLOUD_RADIO_ERROR("CAN Rx message queue create failed.");
		CLOUD_RADIO_ERROR("errno is %d(%s).", errno, strerror(errno));
	}
	else
	{
		CLOUD_RADIO_DEBUG("CAN Rx message queue create success.");
		CLOUD_RADIO_DEBUG("rx_msg_id is %d.", rx_msg_id);
	}

	
	//pthread_create(&vmf_thread, NULL, &vmf_receive_msg, &argument);

	


	
	memset(&info, 0, sizeof info);
	info.port = 7681;

	while (n >= 0) {
		n = getopt_long(argc, argv, "ci:hsp:d:Dr:", options, NULL);
		if (n < 0)
			continue;
		switch (n) {
#ifndef LWS_NO_DAEMONIZE
		case 'D':
			daemonize = 1;
			#ifndef WIN32
			syslog_options &= ~LOG_PERROR;
			#endif
			break;
#endif
		case 'd':
			debug_level = atoi(optarg);
			break;
		case 's':
			use_ssl = 1;
			break;
		case 'p':
			info.port = atoi(optarg);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			iface = interface_name;
			break;
		case 'c':
			close_testing = 1;
			fprintf(stderr, " Close testing mode -- closes on "
					   "client after 50 dumb increments"
					   "and suppresses lws_mirror spam\n");
			break;
		case 'r':
			resource_path = optarg;
			printf("Setting resource path to \"%s\"\n", resource_path);
			break;
		case 'h':
			fprintf(stderr, "Usage: test-server "
					"[--port=<p>] [--ssl] "
					"[-d <log bitfield>] "
					"[--resource_path <path>]\n");
			exit(1);
		}
	}

#if !defined(LWS_NO_DAEMONIZE) && !defined(WIN32)
	/* 
	 * normally lock path would be /var/lock/lwsts or similar, to
	 * simplify getting started without having to take care about
	 * permissions or running as root, set to /tmp/.lwsts-lock
	 */
	if (daemonize && lws_daemonize("/tmp/.lwsts-lock")) {
		fprintf(stderr, "Failed to daemonize\n");
		return 1;
	}
#endif

	signal(SIGINT, sighandler);
	CLOUD_RADIO_DEBUG("Register exit func.");


	/* tell the library what debug level to emit and to send it to syslog */
	lws_set_log_level(debug_level, lwsl_emit_syslog);

	lwsl_notice("libwebsockets test server - "
			"(C) Copyright 2010-2013 Andy Green <andy@warmcat.com> - "
						    "licensed under LGPL2.1\n");
#ifdef EXTERNAL_POLL
	max_poll_elements = getdtablesize();
	pollfds = malloc(max_poll_elements * sizeof (struct pollfd));
	fd_lookup = malloc(max_poll_elements * sizeof (int));
	if (pollfds == NULL || fd_lookup == NULL) {
		lwsl_err("Out of memory pollfds=%d\n", max_poll_elements);
		return -1;
	}
#endif

	info.iface = iface;
	info.protocols = protocols;
#ifdef LWS_NO_EXTENSIONS
	info.extensions = libwebsocket_get_internal_extensions();
#endif
	if (!use_ssl) {
		info.ssl_cert_filepath = NULL;
		info.ssl_private_key_filepath = NULL;
	} else {
		if (strlen(resource_path) > sizeof(cert_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(cert_path, "%s/libwebsockets-test-server.pem",
								resource_path);
		if (strlen(resource_path) > sizeof(key_path) - 32) {
			lwsl_err("resource path too long\n");
			return -1;
		}
		sprintf(key_path, "%s/libwebsockets-test-server.key.pem",
								resource_path);

		info.ssl_cert_filepath = cert_path;
		info.ssl_private_key_filepath = key_path;
	}
	info.gid = -1;
	info.uid = -1;
	info.options = opts;

	context = libwebsocket_create_context(&info);
	if (context == NULL) {
		lwsl_err("libwebsocket init failed\n");
		return -1;
	}
	n = 0;
	while (n >= 0 && !force_exit) {
		struct timeval tv;

		gettimeofday(&tv, NULL);

		/*
		 * This provokes the LWS_CALLBACK_SERVER_WRITEABLE for every
		 * live websocket connection using the DUMB_INCREMENT protocol,
		 * as soon as it can take more packets (usually immediately)
		 */

		if (((unsigned int)tv.tv_usec - oldus) > (50000*10)) {
			libwebsocket_callback_on_writable_all_protocol(&protocols[PROTOCOL_DUMB_INCREMENT]);
			oldus = tv.tv_usec;
		}

#ifdef EXTERNAL_POLL

		/*
		 * this represents an existing server's single poll action
		 * which also includes libwebsocket sockets
		 */

		n = poll(pollfds, count_pollfds, 50);
		if (n < 0)
			continue;


		if (n)
			for (n = 0; n < count_pollfds; n++)
				if (pollfds[n].revents)
					/*
					* returns immediately if the fd does not
					* match anything under libwebsockets
					* control
					*/
					if (libwebsocket_service_fd(context,
								  &pollfds[n]) < 0)
						goto done;
#else
		/*
		 * If libwebsockets sockets are all we care about,
		 * you can use this api which takes care of the poll()
		 * and looping through finding who needed service.
		 *
		 * If no socket needs service, it'll return anyway after
		 * the number of ms in the second argument.
		 */

		n = libwebsocket_service(context, 50);
#endif
	}

#ifdef EXTERNAL_POLL
done:
#endif

	libwebsocket_context_destroy(context);

	lwsl_notice("libwebsockets-test-server exited cleanly\n");

#ifndef WIN32
	closelog();
#endif

	return 0;
}



