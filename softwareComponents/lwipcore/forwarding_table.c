#include <fwtable/forwarding_table.h>

#include <lwip/ip6_addr.h>
#include <lwip/ip6.h>
#include "string.h"

#ifndef FW_TABLE_SIZE
#define FW_TABLE_SIZE 16
#endif

static int fw_table_size = 0;
static struct rt_entry fw_table[FW_TABLE_SIZE];

static int edge_segment;
static char mask_str[48];
static int pos;

int cmp_masks(const struct rt_entry* a, const struct rt_entry* b) {
	return a->mask > b->mask;
}

void ip_print_table(void) {
	for (int i = 0; i < fw_table_size; i++) {
        printf("ip: %s/%d gw: %s\n", ip6addr_ntoa(&fw_table[i].addr)
		                           , (int) fw_table[i].mask, fw_table[i].gw_name);
	}
}

uint8_t turn_on_n_bits(uint8_t n) {
    if (n == 0 || n >= 8)
        return 0;

    return (1 << (8 - n)) + turn_on_n_bits(n - 1);
}

void mask_to_address(uint8_t mask, ip6_addr_t* m) {
	ip6_addr_set_zero(m);
    ip6_addr_t full;

    for (int i = 0; i < 4; i++) {
        full.addr[i] = UINT32_MAX;
    }

    memcpy(m, full.addr, mask / 8);
    /* there can be some left over when the mask is not byte padded */
    uint8_t leftover = mask % 8;
    if (leftover != 0) {
        *(((uint8_t*) m->addr) + mask / 8) = turn_on_n_bits(leftover);
    }

	ip6_addr_set_zone(m, IP6_UNKNOWN); /* Options: IP6_UNKNOWN = 0, IP6_UNICAST = 1, IP6_MULTICAST = 2 */
}

void ip6_addr_and(ip6_addr_t* ip, const ip6_addr_t* mask) {
	for (int i = 0; i < 4; i++)
		ip->addr[i] &= mask->addr[i];
}

void ip_mask_and(const ip6_addr_t* ip, uint8_t mask, ip6_addr_t* result) {
	ip6_addr_t ip_mask;
	mask_to_address(mask, &ip_mask);
	ip6_addr_copy(*result, *ip);
	ip6_addr_and(result, &ip_mask);
}

void fill_rt_entry(struct rt_entry* r, const ip6_addr_t* addr, uint8_t mask, const char* netif_name) {
	ip6_addr_t ip;
	ip_mask_and(addr, mask, &ip);
	ip6_addr_copy(r->addr, ip);
	r->mask = mask;
	memcpy(r->gw_name, netif_name, GW_NAME_SIZE);
}


struct rt_entry* ip_find_route_entry(const ip6_addr_t* ip) {
	ip6_addr_t searched;
	for (int i = 0; i < fw_table_size; i++) {
		ip_mask_and(ip, fw_table[i].mask, &searched);
		if (ip6_addr_cmp_zoneless(&searched, &fw_table[i].addr)) {
			return &fw_table[i];
        }
    }

	return NULL;
}

void swap(struct rt_entry* e1, struct rt_entry* e2) {
    struct rt_entry tmp = *e1;
    *e1 = *e2;
    *e2 = tmp;
}

void check_order_of_last(void) {
    int last_pos = fw_table_size - 1;
    while (last_pos > 0 && fw_table[ last_pos ].mask > fw_table[ last_pos - 1 ].mask) {
        swap(&fw_table[ last_pos ], &fw_table[ last_pos - 1 ]);
        last_pos--;
    }
}

int ip_add_route(const ip6_addr_t* addr, uint8_t mask, const char* netif_name) {
	struct rt_entry* entry = ip_find_route_entry(addr);

	if (entry == NULL || entry->mask != mask) {
        fw_table_size++;
        if (fw_table_size >= FW_TABLE_SIZE) {
            LWIP_DEBUGF(IP6_DEBUG | LWIP_DBG_LEVEL_WARNING, ("forwarding table is full\n"));
            return 0;
        }
		fill_rt_entry(&fw_table[fw_table_size - 1], addr, mask, netif_name);
		check_order_of_last();
		return 1;
	}
    return 0;
}

struct netif* ip_find_route(const ip6_addr_t* ip, const ip6_addr_t* src) {
	struct rt_entry* entry = ip_find_route_entry(ip);

    return entry && !ip6_addr_isany_val(*src) ? netif_find(entry->gw_name) : NULL;
}

const ip6_addr_t* ip_find_gw(struct netif* netif, const ip6_addr_t* dest) {
    printf("ip_find_gw for netif %s%d and address %s\n", netif->name, netif->num, ip6addr_ntoa(dest));
    struct rt_entry* entry = ip_find_route_entry(dest);
    struct netif* n = entry != NULL ? netif_find(entry->gw_name) : NULL;

    if (netif != NULL) {
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++) {
            if (!ip_addr_isany(&netif->ip6_addr[i])) {
                return ip_2_ip6(&netif->ip6_addr[i]);
            }
        }
    }

    return NULL;
}

void remove_at(int index) {
    while (index < fw_table_size - 1) {
        fw_table[index] = fw_table[index + 1];
        index++;
    }
    --fw_table_size;
}

int ip_rm_route(const ip6_addr_t* ip, uint8_t mask) {
    ip6_addr_t searched;
	ip_mask_and(ip, mask, &searched);
    for (int i = 0; i < fw_table_size; i++) {
        if (fw_table[i].mask == mask && ip6_addr_cmp(&fw_table[i].addr, &searched)) {
            remove_at(i);
            return 1;
        }
    }

	return 0;
}

int ip_rm_route_if(const char* netif_name) {
	int size  = fw_table_size;
    int index = fw_table_size - 1;

	while (index >= 0 && index < fw_table_size) {
        if (strcmp(netif_name, fw_table[index].gw_name) == 0) {
            remove_at(index);
        }
        --index;
    }

	return size != fw_table_size ? 1 : 0;
}

void ip_update_route(const ip6_addr_t* ip, uint8_t mask, const char* new_gw) {
    ip6_addr_t searched;
	ip_mask_and(ip, mask, &searched);
	int i = 0;
	while (fw_table[i].mask <= mask) {
		if (mask == fw_table[i].mask && ip6_addr_cmp_zoneless(&fw_table[i].addr, &searched)) {
			memcpy(fw_table[i].gw_name, new_gw, GW_NAME_SIZE);
			break;
		}
		i++;
	}
}

void ip_clear() { fw_table_size = 0; }
