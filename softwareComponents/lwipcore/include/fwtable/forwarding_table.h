#ifndef LWIP_FORWARDING_TABLE_H
#define LWIP_FORWARDING_TABLE_H

#include <lwip/ip_addr.h>
#include <lwip/ip.h>
#include <lwip/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GW_NAME_SIZE
#define GW_NAME_SIZE 5
#endif

struct rt_entry {
	ip6_addr_t addr;
	uint8_t mask;
	char gw_name[GW_NAME_SIZE];
};

int ip_add_route(const ip6_addr_t* ip, uint8_t mask, const char* gw);

struct netif* ip_find_route(const ip6_addr_t* dest, const ip6_addr_t*);

const ip6_addr_t* ip_find_gw(struct netif* netif, const ip6_addr_t* dest);

int ip_rm_route(const ip6_addr_t* ip, uint8_t mask);

int ip_rm_route_if(const char* netif_name);

void ip_update_route(const ip6_addr_t* ip, uint8_t mask, const char* new_gw);

void ip_print_table(void);


/* helper function */
void mask_to_address(uint8_t mask, ip6_addr_t* m);

#ifdef __cplusplus
}
#endif

#endif /* LWIP_FORWARDING_TABLE_H */
