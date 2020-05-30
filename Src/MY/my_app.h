#ifndef __MY_APP_H
#define __MY_APP_H

#include "cc.h"
#include "err.h"
#include "netif.h"

typedef enum PRINTF_AS{
	HEX=0,
	DEC=1
}PRINTF_AS;

const char *state_cgi_handler(int index, int n_params, char *params[], char *values[]);
const char *ctl_cgi_handler1(int index, int n_params, char *params[], char *values[]);
void init_lwip();
void usb_polling();
void on_packet(const char *data, int size);
u16_t ssi_handler(int index, char *insert, int ins_len);

err_t netif_init_cb(struct netif *netif);

err_t linkoutput_fn(struct netif *netif, struct pbuf *p);
err_t output_fn(struct netif *netif, struct pbuf *p, ip_addr_t *ipaddr);

void ip2printf(u32_t addr);
void pu8_to_prn(uint8_t *addr , uint8_t len , uint8_t *delimiter, int8_t dir , PRINTF_AS type);

#endif
