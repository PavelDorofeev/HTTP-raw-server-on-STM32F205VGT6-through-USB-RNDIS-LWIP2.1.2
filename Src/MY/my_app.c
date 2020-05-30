#include "my_app.h"
#include "cc.h"
#include "time.h"

//#include <stdlib.h>
//#include <stdio.h>
#include "usbd_rndis_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usb_conf.h"
#include "usbd_core.h"
//#include "stm32f4_discovery.h"
//#include "stm32f4_discovery_lis302dl.h"
#include "netif/etharp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/icmp.h"
#include "lwip/udp.h"
#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/inet.h"
#include "lwip/dns.h"
//!!#include "lwip/tcp_impl.h"
#include "lwip/tcp.h"
#include "time.h"
#include "httpd.h"

static uint8_t hwaddr[6]  = {0x20,0x89,0x84,0x6A,0x96,00};//0xAB};
static uint8_t ipaddr[4]  = {192, 168, 7, 1};
static uint8_t netmask[4] = {255, 255, 255, 0};
static uint8_t gateway[4] = {0, 0, 0, 0};



//#define NUM_DHCP_ENTRY 3
/*
static dhcp_entry_t entries[NUM_DHCP_ENTRY] =
{
		// mac    ip address        subnet mask        lease time
		{ {0}, {192, 168, 7, 3}, {255, 255, 255, 0}, 24 * 60 * 60 },
		{ {0}, {192, 168, 7, 4}, {255, 255, 255, 0}, 24 * 60 * 60 },
		{ {0}, {192, 168, 7, 5}, {255, 255, 255, 0}, 24 * 60 * 60 }
};
 */
/*!!static dhcp_config_t dhcp_config =
{
		{192, 168, 7, 1}, 67, // server address, port
		{192, 168, 7, 1},     // dns server
		"stm",                // dns suffix
		NUM_DHCP_ENTRY,       // num entry
		entries               // entries
};*/

const char *state_cgi_handler(int index, int n_params, char *params[], char *values[])
{
	//printf("\nstate_cgi_handler return 'state.shtml'\n");
	return "/state.shtml";
}

volatile uint8_t wifi_login[100]={'q','u','q','u','\0'};

const char *ctl_cgi_handler(int index, int n_params, char *params[], char *values[])
{
	int i;

	printf("ctl_cgi_handler index=%x\n",index);

	for (i = 0; i < n_params; i++)
	{
		if (strcmp(params[i], "wifi_login") == 0)
		{
			printf(" %s : %s \n",params[i],values[i]);

			char *src= values[i];

			uint16_t ii =0;
			for (ii = 0; ii < strlen(src); ii++)
			{
				wifi_login[ii] = *(char *)(src + ii);
			}
			wifi_login[ii] = '\0';

			printf(" %s : %s \n",params[i],wifi_login);

		}
	}
	return "/state1.shtml";
}

void httpd_cgi_handler(
		struct fs_file *file,
		const char* uri,
		int iNumParams,
		char **pcParam,
		char **pcValue
#if defined(LWIP_HTTPD_FILE_STATE) && LWIP_HTTPD_FILE_STATE
		, void *connection_state
#endif /* LWIP_HTTPD_FILE_STATE */
)
{

}

int whoIs=0;

const char *ctl_cgi_handler1(int index, int n_params, char *params[], char *values[])
{
	int i;

	printf("ctl_cgi_handler1 index=%x\n",index);

	whoIs = index-1;

	for (i = 0; i < n_params; i++)
	{
		if (strcmp(params[i], "wifi_login") == 0)
		{
			printf(" %s : %s \n",params[i],values[i]);

			char *src= values[i];

			uint16_t ii =0;
			for (ii = 0; ii < strlen(src); ii++)
			{
				wifi_login[ii] = *(char *)(src + ii);
			}
			wifi_login[ii] = '\0';

			printf(" %s : %s \n",params[i],wifi_login);

		}
	}
	return "/state1.shtml";
}

static const tCGI cgi_uri_table[] =
{
		{ "/state.cgi", state_cgi_handler },
		{ "/ctl.cgi",   ctl_cgi_handler },
		{ "/ctl1.cgi",   ctl_cgi_handler1 },
};


u16_t ssi_handler(int index, char *insert, int ins_len)
{
	int res;

	if (ins_len < 32)
		return 0;

	printf("ssi_handler\n");

	aaa:
	switch (index)
	{
	case 0: // systick
		res = snprintf(insert, ins_len, "%u", (unsigned) mtime());
		break;
	case 1: // wifilogin
		res = snprintf(insert, ins_len, "%s", wifi_login);//"%s", wifi_login	);
		break;
	case 2: // acc
	{
		int32_t acc[3];
		//LIS302DL_ReadACC(acc);
		res = snprintf(insert, ins_len, "%i, %i, %i", acc[0], acc[1], acc[2]);
		break;
	}
	case 3: // ledg
		res = 1;
		break;
	case 4: // ledo
		res = 1;
		break;
	case 5: // ledr
		res = 1;
		break;
	case 6 :
		index = whoIs;
		printf("\n-----written------whoIs =%d\n",whoIs);
		goto aaa;
		break;
	}

	return res;
}

//struct netif netif_data;

bool dns_query_proc(const char *name, ip_addr_t *addr)
{
	if (strcmp(name, "run.stm") == 0 || strcmp(name, "www.run.stm") == 0)
	{
		addr->addr = *(uint32_t *)ipaddr;
		return true;
	}
	return false;

}

struct netif netif_myData;


void init_periph(void)
{

}

uint32_t sys_now()
{
	return (int64_t)mtime();
}

static uint8_t receivedFromUSB[RNDIS_MTU + 14];
static int receivedFromUSBSize = 0;

void on_packet(const char *data, int size)
{
	memcpy(receivedFromUSB, data, size);
	receivedFromUSBSize = size;
}

/*
TIMER_PROC(tcp_timer, TCP_TMR_INTERVAL * 1000, 1, NULL)
{
	tcp_tmr();
}*/

void ip2printf(u32_t addr)
{
	printf("%d.%d.%d.%d",
			addr & 0xFF,
			(addr>>8) & 0xFF,
			(addr>>16) & 0xFF,
			(addr>>24) & 0xFF);
}

void pu8_to_prn(uint8_t *addr , uint8_t len , uint8_t *delimiter, int8_t dir , PRINTF_AS type)
{
	if(dir == -1)
	{
		for(int8_t ii=(len-1); ii >=0; ii--)
		{
			if(ii == 0)
				delimiter=" ";

			if(type == HEX)
				printf("%0.2x%s" , *(uint8_t*)(addr + ii) , delimiter);
			else if(type == DEC)
				printf("%d%s" , *(uint8_t*)(addr + ii) , delimiter);

		}
	}
	else if(dir == 1)
	{
		for(int8_t ii=0; ii < len; ii++)
		{
			if(ii == (len -1))
				delimiter=" ";

			if(type == HEX)
				printf("%0.2x%s" , *(uint8_t*)(addr + ii) , delimiter);
			else if(type == DEC)
				printf("%d%s" , *(uint8_t*)(addr + ii) , delimiter);
		}
	}

}


void usb_polling()
{

	struct pbuf *frame;

	__disable_irq();

	if (receivedFromUSBSize == 0)
	{
		__enable_irq();
		return;
	}

	frame = pbuf_alloc(PBUF_RAW , receivedFromUSBSize , PBUF_POOL);

	if (frame == NULL)
	{
		__enable_irq();
		return;
	}
	//printf("%d \n",receivedFromUSBSize);

	memcpy(frame->payload, receivedFromUSB, receivedFromUSBSize);

	frame->len = receivedFromUSBSize;

	receivedFromUSBSize = 0;

	__enable_irq();

	ethernet_input(frame, &netif_myData);

	pbuf_free(frame);

}

static int outputs = 0;

err_t output_fn(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr)
{
	return etharp_output(netif, p, ipaddr);
}

err_t netif_init_cb(struct netif *netif)
{
	LWIP_ASSERT("netif != NULL", (netif != NULL));

	netif->mtu = RNDIS_MTU;
	netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP | NETIF_FLAG_UP;
	netif->state = NULL;
	netif->name[0] = 'E';
	netif->name[1] = 'X';
	netif->linkoutput = linkoutput_fn; // arp answer
	netif->output = output_fn;

	return ERR_OK;
}
err_t linkoutput_fn(struct netif *netif, struct pbuf *p)
{

	struct eth_hdr *payloadStruct = (struct eth_hdr *)p->payload;

	LWIP_DEBUGF(ETHARP_DEBUG | LWIP_DBG_TRACE,
			("ethernet_output: src:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X:%0.2X --> dest:%0.2X:%0.2X:%"X8_F":%"X8_F":%"X8_F":%"X8_F" type:%"X16_F"\n",
					(unsigned char)payloadStruct->src.addr[0],  (unsigned char)payloadStruct->src.addr[1],  (unsigned char)payloadStruct->src.addr[2],
					(unsigned char)payloadStruct->src.addr[3],  (unsigned char)payloadStruct->src.addr[4],  (unsigned char)payloadStruct->src.addr[5],
					(unsigned char)payloadStruct->dest.addr[0], (unsigned char)payloadStruct->dest.addr[1], (unsigned char)payloadStruct->dest.addr[2],
					(unsigned char)payloadStruct->dest.addr[3], (unsigned char)payloadStruct->dest.addr[4], (unsigned char)payloadStruct->dest.addr[5],
					lwip_htons(payloadStruct->type)));

	int i;
	struct pbuf *q;
	static char data[RNDIS_MTU + 14 + 4];
	int size = 0;
	for (i = 0; i < 200; i++)
	{
		if (rndis_can_send()) break;
		msleep(1);
	}
	for(q = p; q != NULL; q = q->next)
	{
		if (size + q->len > RNDIS_MTU + 14)
			return ERR_ARG;
		memcpy(data + size, (char *)q->payload, q->len);
		size += q->len;
	}
	if (!rndis_can_send())
		return ERR_USE;

	rndis_send(data, size);

	outputs++;
	return ERR_OK;
}








