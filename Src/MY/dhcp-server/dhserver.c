/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 by Sergey Fetisov <fsenok@gmail.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "dhserver.h"
#include "my_app.h"

/* DHCP message type */
#define DHCP_DISCOVER       1
#define DHCP_OFFER          2
#define DHCP_REQUEST        3
#define DHCP_DECLINE        4
#define DHCP_ACK            5
#define DHCP_NAK            6
#define DHCP_RELEASE        7
#define DHCP_INFORM         8

/* DHCP options */
enum DHCP_OPTIONS
{
	DHCP_PAD                    = 0,
	DHCP_SUBNETMASK             = 1,
	DHCP_ROUTER                 = 3,
	DHCP_DNSSERVER              = 6,
	DHCP_HOSTNAME               = 12,
	DHCP_DNSDOMAIN              = 15,
	DHCP_MTU                    = 26,
	DHCP_BROADCAST              = 28,
	DHCP_PERFORMROUTERDISC      = 31,
	DHCP_STATICROUTE            = 33,
	DHCP_NISDOMAIN              = 40,
	DHCP_NISSERVER              = 41,
	DHCP_NTPSERVER              = 42,
	DHCP_VENDOR                 = 43,
	DHCP_IPADDRESS              = 50,
	DHCP_LEASETIME              = 51,
	DHCP_OPTIONSOVERLOADED      = 52,
	DHCP_MESSAGETYPE            = 53,
	DHCP_SERVERID               = 54,
	DHCP_PARAMETERREQUESTLIST   = 55,
	DHCP_MESSAGE                = 56,
	DHCP_MAXMESSAGESIZE         = 57,
	DHCP_RENEWALTIME            = 58,
	DHCP_REBINDTIME             = 59,
	DHCP_CLASSID                = 60,
	DHCP_CLIENTID               = 61,
	DHCP_USERCLASS              = 77,  /* RFC 3004 */
	DHCP_FQDN                   = 81,
	DHCP_DNSSEARCH              = 119, /* RFC 3397 */
	DHCP_CSR                    = 121, /* RFC 3442 */
	DHCP_MSCSR                  = 249, /* MS code for RFC 3442 */
	DHCP_END                    = 255
};

typedef struct
{
	uint8_t  dp_op;           /* packet opcode type */
	uint8_t  dp_htype;        /* hardware addr type */
	uint8_t  dp_hlen;         /* hardware addr length */
	uint8_t  dp_hops;         /* gateway hops */
	uint32_t dp_xid;          /* transaction ID */
	uint16_t dp_secs;         /* seconds since boot began */
	uint16_t dp_flags;
	uint8_t  dp_ciaddr[4];    /* client IP address */
	uint8_t  dp_yiaddr[4];    /* 'your' IP address */
	uint8_t  dp_siaddr[4];    /* server IP address */
	uint8_t  dp_giaddr[4];    /* gateway IP address */
	uint8_t  dp_chaddr[16];   /* client hardware address */
	uint8_t  dp_legacy[192];
	uint8_t  dp_magic[4];
	uint8_t  dp_options[275]; /* options area */
} DHCP_TYPE;

DHCP_TYPE dhcp_data;
static struct udp_pcb *pcb = NULL;
static dhcp_config_t *config = NULL;

char magic_cookie[] = {0x63,0x82,0x53,0x63};

static dhcp_entry_t *entry_by_ip(uint32_t ip)
{
	int i;
	for (i = 0; i < config->num_entry; i++)
		if (*(uint32_t *)config->entries[i].addr == ip)
			return &config->entries[i];
	return NULL;
}

static dhcp_entry_t *entry_by_mac(uint8_t *mac)
{
	int i;
	for (i = 0; i < config->num_entry; i++)
		if (memcmp(config->entries[i].mac, mac, 6) == 0)
			return &config->entries[i];
	return NULL;
}

static __inline bool is_vacant(dhcp_entry_t *entry)
{
	return memcmp("\0\0\0\0\0", entry->mac, 6) == 0;
}

static dhcp_entry_t *vacant_address()
{
	int i;
	for (i = 0; i < config->num_entry; i++)
		if (is_vacant(config->entries + i))
			return config->entries + i;
	return NULL;
}

static __inline void free_entry(dhcp_entry_t *entry)
{
	memset(entry->mac, 0, 6);
}

uint8_t *find_dhcp_option(uint8_t *attrs, int size, uint8_t attr)
{
	int i = 0;
	while ((i + 1) < size)
	{
		int next = i + attrs[i + 1] + 2;
		if (next > size) return NULL;
		if (attrs[i] == attr)
			return attrs + i;
		i = next;
	}
	return NULL;
}

int fill_options(void *dest,
		uint8_t msg_type,
		const char *domain,
		uint32_t dns,
		int lease_time,
		uint32_t serverid,
		uint32_t router,
		uint32_t subnet)
{
	uint8_t *ptr = (uint8_t *)dest;
	/* ACK message type */
	*ptr++ = 53;
	*ptr++ = 1;
	*ptr++ = msg_type;

	/* dhcp server identifier */
	*ptr++ = DHCP_SERVERID;
	*ptr++ = 4;
	*(uint32_t *)ptr = serverid;
	ptr += 4;

	/* lease time */
	*ptr++ = DHCP_LEASETIME;
	*ptr++ = 4;
	*ptr++ = (lease_time >> 24) & 0xFF;
	*ptr++ = (lease_time >> 16) & 0xFF;
	*ptr++ = (lease_time >> 8) & 0xFF;
	*ptr++ = (lease_time >> 0) & 0xFF;

	/* subnet mask */
	*ptr++ = DHCP_SUBNETMASK;
	*ptr++ = 4;
	*(uint32_t *)ptr = subnet;
	ptr += 4;

	/* router */
	if (router != 0)
	{
		*ptr++ = DHCP_ROUTER;
		*ptr++ = 4;
		*(uint32_t *)ptr = router;
		ptr += 4;
	}

	/* domain name */
	if (domain != NULL)
	{
		int len = strlen(domain);
		*ptr++ = DHCP_DNSDOMAIN;
		*ptr++ = len;
		memcpy(ptr, domain, len);
		ptr += len;
	}

	/* domain name server (DNS) */
	if (dns != 0)
	{
		*ptr++ = DHCP_DNSSERVER;
		*ptr++ = 4;
		*(uint32_t *)ptr = dns;
		ptr += 4;
	}

	/* end */
	*ptr++ = DHCP_END;
	return ptr - (uint8_t *)dest;
}

static void udp_recv_proc(void *arg, struct udp_pcb *upcb, struct pbuf *p, struct ip_addr *addr, u16_t port)
{
	uint8_t *ptr;
	dhcp_entry_t *entry;
	struct pbuf *pp;

	int n = p->len;
	if (n > sizeof(dhcp_data)) n = sizeof(dhcp_data);
	memcpy(&dhcp_data, p->payload, n);

	switch (dhcp_data.dp_options[2])
	{
	case DHCP_DISCOVER:

		printf("DHCP_DISCOVER\n");
		entry = entry_by_mac(dhcp_data.dp_chaddr);

		if (entry == NULL)
			entry = vacant_address();

		if (entry == NULL)
			break;

		dhcp_data.dp_op = 2; /* reply */
		dhcp_data.dp_secs = 0;
		dhcp_data.dp_flags = 0;

		*(uint32_t *)dhcp_data.dp_yiaddr = *(uint32_t *)entry->addr;

		printf("our dhcp  set ip : ");
		ip2printf(entry->addr);
		printf("\n");

		memcpy(dhcp_data.dp_magic, magic_cookie, 4);

		memset(dhcp_data.dp_options, 0, sizeof(dhcp_data.dp_options));

		fill_options(dhcp_data.dp_options,
				DHCP_OFFER,
				config->domain,
				*(uint32_t *)config->dns,
				entry->lease, 
				*(uint32_t *)config->addr,
				*(uint32_t *)config->addr, 
				*(uint32_t *)entry->subnet);

		pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcp_data), PBUF_POOL);

		if (pp == NULL)
			break;

		memcpy(pp->payload, &dhcp_data, sizeof(dhcp_data));

		udp_sendto(upcb, pp, IP_ADDR_BROADCAST, port);
		pbuf_free(pp);

		break;

	case DHCP_REQUEST:

		printf("DHCP_REQUEST\n");
		/* 1. find requested ipaddr in option list */

		ptr = find_dhcp_option(dhcp_data.dp_options, sizeof(dhcp_data.dp_options), DHCP_IPADDRESS);

		if (ptr == NULL)
			break;

		if (ptr[1] != 4)
			break;

		ptr += 2;

		/* 2. does hw-address registered? */
		entry = entry_by_mac(dhcp_data.dp_chaddr);

		if (entry != NULL)
			free_entry(entry);

		/* 3. find requested ipaddr */
		entry = entry_by_ip(*(uint32_t *)ptr);

		if (entry == NULL)
			break;

		if (!is_vacant(entry))
			break;

		/* 4. fill struct fields */
		memcpy(dhcp_data.dp_yiaddr, ptr, 4);
		dhcp_data.dp_op = 2; /* reply */
		dhcp_data.dp_secs = 0;
		dhcp_data.dp_flags = 0;
		memcpy(dhcp_data.dp_magic, magic_cookie, 4);

		/* 5. fill options */
		memset(dhcp_data.dp_options, 0, sizeof(dhcp_data.dp_options));


		printf(" client ip : "); // client IP address
		pu8_to_prn(dhcp_data.dp_ciaddr , 4 , "." , 1 , DEC);

		// Поле, в котором указывается IP-адрес клиента. Клиент заполняет его только в том случае,
		// если у него уже есть IP-адрес и он может ответить на ARP-запрос.
		//  Такая ситуация возможно в том случае, если клиент хочет продлить время аренды IP-адреса.

		printf("\n your ip : "); // 'your' IP address
		pu8_to_prn(dhcp_data.dp_yiaddr , 4 , "." , 1 , DEC);

		//В это поле DHCP-сервер вписывает IP-адрес, который хочет предложить клиенту.

		printf("\n server ip : "); // server IP address
		pu8_to_prn(dhcp_data.dp_siaddr , 4 , "." , 1 , DEC);

		// IP-адрес сервера. Сервер указывает свой IP-адрес, когда делает DHCPOFFER

		printf("\n gw ip : "); // gateway IP address
		pu8_to_prn(&dhcp_data.dp_giaddr[0] , 4 , "." , 1 , DEC);

		//Если используется схема с DHCP Relay Agent, то в этом поле передается его IP-адрес

		printf("\n mac (client hardware address) :");  //client hardware address
		pu8_to_prn(dhcp_data.dp_chaddr , 6 , ":" , 1 , HEX);
		// Если на канальном уровне используется протокол Ethernet,
		// то в это поле записывается MAC-адрес клиента

		fill_options(dhcp_data.dp_options,
				DHCP_ACK,
				config->domain,
				*(uint32_t *)config->dns,
				entry->lease, 
				*(uint32_t *)config->addr,
				*(uint32_t *)config->addr, 
				*(uint32_t *)entry->subnet);


		printf("\n ---- dp_options ----");

		printf("\n domain : %s " , config->domain);
		// Если у сервера есть доменное имя/имя хоста, то он может сообщить его в этом поле,
		// поле не является обязательным

		printf("\n dns : ");
		pu8_to_prn(config->dns , 4 , "." , 1 , DEC);

		printf("\n addr : ");
		pu8_to_prn(config->entries->addr , 4 , "." , 1 , DEC);

		printf("\n subnet : ");
		pu8_to_prn(config->entries->subnet , 4 , "." , 1 , DEC);

		printf("\n mac : ");
		pu8_to_prn(config->entries->subnet , 6 , ":" , 1 , HEX);

		printf("\n");


		/* 6. send ACK */

		pp = pbuf_alloc(PBUF_TRANSPORT, sizeof(dhcp_data), PBUF_POOL);

		if (pp == NULL)
			break;

		memcpy(entry->mac, dhcp_data.dp_chaddr, 6);
		memcpy(pp->payload, &dhcp_data, sizeof(dhcp_data));

		udp_sendto(upcb, pp, IP_ADDR_BROADCAST, port);

		pbuf_free(pp);
		break;

	default:
		break;
	}
	pbuf_free(p);
}

err_t dhserv_init(dhcp_config_t *c)
{
	err_t err;

	udp_init();

	dhserv_free();

	pcb = udp_new();

	if (pcb == NULL)
		return ERR_MEM;

	err = udp_bind(pcb, IP_ADDR_ANY, c->port);

	if (err != ERR_OK)
	{
		dhserv_free();
		return err;
	}

	udp_recv(pcb, udp_recv_proc, NULL);

	config = c;
	return ERR_OK;
}

void dhserv_free(void)
{
	if (pcb == NULL) return;
	udp_remove(pcb);
	pcb = NULL;
}
