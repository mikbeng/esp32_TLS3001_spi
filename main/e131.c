/* sACN (Streaming ACN) in standard E1.31 is used for sending light data in DMX512 format */
#include "e131.h"

#include "lwip/api.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "lwip/netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

static const char *TAG = "E1.31";
e131_packet_t e131packet; /* Packet buffer */

uint16_t reverse(uint16_t num) {
	return (num>>8) | (num<<8);
}

void e131task(void *pvParameters) {
	struct netconn *conn;
	err_t err;

	/* Create a new connection handle */
	conn = netconn_new(NETCONN_UDP);
	if(!conn) {
		printf("Error: Failed to allocate socket.\n");
		return;
	}

	/* Bind to port with default IP address */
	err = netconn_bind(conn, IP_ADDR_ANY, E131_DEFAULT_PORT);
	if(err != ERR_OK) {
		printf("Error: Failed to bind socket. err=%d\n", err);
		return;
	}

	ip_addr_t multiaddr;
	IP_ADDR4(&multiaddr, 239, 255, 0, 1); //IPv4 local scope multicast

	err = netconn_join_leave_group(conn, &multiaddr, &netif_default->ip_addr, NETCONN_JOIN);
	if(err != ERR_OK) {
		printf("Error: Join Multicast Group. err=%d\n", err);
		return;
	}

	printf("Listening for connections.\n");

	while(1) {
		struct netbuf *buf;

		err = netconn_recv(conn, &buf);
		if(err != ERR_OK) {
			printf("Error: Failed to receive packet. err=%d\n", err);
			continue;
		}

		if(buf->p->tot_len == sizeof(e131packet.raw)) {
			//If packet is 638 bytes we handle it as a correct package and copy it to e131packet struct
			memcpy(e131packet.raw, buf->p->payload, buf->p->tot_len);
			e131packet.universe = reverse(e131packet.universe);
			ESP_LOGI(TAG, "Universe %d channel 1 %d", e131packet.universe, e131packet.property_values[1]);
		} else {
			printf("Wrong packet size.\n\n");
		}

		netbuf_delete(buf);
	}
}

void e131init() {
	xTaskCreate(&e131task, "E131_task", 4096, NULL, 5, NULL);
}

