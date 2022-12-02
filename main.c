/**
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "pico/stdlib.h"
#include "pico_eth/ethpio_arch.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

//#include "machine_i2s.c"

#define SCK 3
#define WS 4 // needs to be SCK +1
#define SD 29
#define BPS 32 // 24 is not valid in this implementation, but INMP441 outputs 24 bits samples
#define RATE 16000

#define UDP_DST "192.168.8.112"
#define UDP_PORT 4444
#define UDP_MSG_LEN 400

void network_init(void)
{
	// Wrap the network configuration in a function to free the memory occupied by it at the end of the call
	ethpio_parameters_t config;

	config.pioNum = 0; // Pio, 0 or 1 (should be 0 for now, 1 untested)

	config.rx_pin = 13; // RX pin (RX+ : positive RX ethernet pair side)

	// The two pins of the TX must follow each other ! TX- is ALWAYS first, TX+ next
	config.tx_neg_pin = 9; // TX pin (TX- : negative TX ethernet pair side, 10 will be TX+)

	MAC_ADDR(config.mac_address, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05);

	// Network default values (When DHCP disabled or unavailable)
	IP4_ADDR(&config.default_ip_v4, 192, 168, 8, 110);      // Network IPv4
	IP4_ADDR(&config.default_netmask_v4, 255, 255, 255, 0); // Net mask
	IP4_ADDR(&config.default_gateway_v4, 192, 168, 8, 1);   // Gateway

	strcpy(config.hostname, "ethermic");

	config.enable_dhcp_client = false; // Enable DHCP client

	eth_pio_arch_init(&config); // Apply, ARP & DHCP take time to set up : network will not be available immediatly
}


int main() {
	// Board init
    set_sys_clock_khz(120000, true);
	stdio_init_all();

	// setup mic
	//machine_i2s_obj_t* i2s0 = machine_i2s_make_new(0, SCK, WS, SD, RX, BPS, STEREO, /*ringbuf_len*/SIZEOF_DMA_BUFFER_IN_BYTES, RATE);
	//int32_t buffer[I2S_RX_FRAME_SIZE_IN_BYTES /4];

	// setup network
	network_init();
	ip_addr_t addr;
	ipaddr_aton(UDP_DST, &addr);
	struct udp_pcb* pcb = udp_new_ip_type(IPADDR_TYPE_ANY);

	int counter = 1;
	while (true) {
		eth_pio_arch_poll();
		//machine_i2s_stream_read(i2s0, (void*)&buffer[0], I2S_RX_FRAME_SIZE_IN_BYTES);
		//*(int32_t*)(req +((counter*4) % p->len)) = buffer[0]; // 0=left, 1=right, right channel is empty
		
		struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, UDP_MSG_LEN, PBUF_RAM);
		char *req = (char *)p->payload;
		memset(req, 0, UDP_MSG_LEN);
		snprintf(req, UDP_MSG_LEN, "UDP test %d", counter);
		err_t er = udp_sendto(pcb, p, &addr, UDP_PORT);
		pbuf_free(p);
		if (er != ERR_OK) {
			printf("Failed to send UDP packet! error=%d\n", er);
		} else {
			printf("Sent packet %d\n", counter);
			counter++;
		}
		sleep_ms(500);
	}
}
