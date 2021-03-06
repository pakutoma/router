#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>
#include <net/ethernet.h>

typedef struct {
    struct in6_addr adv_address_prefix;
    uint32_t adv_address_prefix_length;
    uint32_t adv_valid_life_time;
    bool adv_on_link_flag;
    uint32_t adv_preferred_life_time;
    bool adv_autonomous_flag;
} adv_prefix_t;

typedef struct {
    bool is_router;
    bool adv_send_advertisements;
    uint32_t max_rtr_adv_interval;
    uint32_t min_rtr_adv_interval;
    bool adv_managed_flag;
    bool adv_other_config_flag;
    uint32_t adv_link_mtu;
    uint32_t adv_reachable_time;
    uint32_t adv_retrans_timer;
    uint8_t adv_cur_hop_limit;
    uint16_t adv_default_lifetime;
    adv_prefix_t *adv_prefix_list;
    uint32_t adv_prefix_list_length;
    struct in6_addr *adv_recursive_dns_server_list;
    uint32_t adv_recursive_dns_server_list_length;
    char **adv_dns_search_list;
    uint32_t adv_dns_search_list_length;
} adv_settings_t;

typedef struct {
    char *netif_name;
    int sock_desc;
    uint8_t hw_addr[ETH_ALEN];
    struct in_addr addr;
    struct in_addr subnet;
    struct in_addr netmask;
    struct in6_addr *addr6_list;
    struct in6_addr *subnet6_list;
    struct in6_addr *netmask6_list;
    uint32_t addr6_list_length;
    adv_settings_t adv_settings;
} device_t;

typedef struct {
    bool enable_log_stdout;
    bool enable_log_error;
} log_mode_t;

typedef struct {
    uint32_t bucket;
    uint32_t token_per_second;
} ratelimit_t;

typedef struct {
    struct in_addr next_router_addr;
    struct in6_addr next_router_ipv6_addr;
    uint32_t gateway_device_index;
    device_t *devices;
    uint32_t devices_length;
    log_mode_t log_mode;
    ratelimit_t ratelimit;
} settings_t;

device_t *find_device_by_macaddr(uint8_t macaddr[ETH_ALEN]);
device_t *find_device_by_ipaddr(uint32_t ipaddr);
device_t *find_device_by_ipv6addr(struct in6_addr *ipaddr);
int find_device_index_by_macaddr(uint8_t macaddr[ETH_ALEN]);
int find_device_index_by_sock_desc(int sock_desc);
bool is_my_device_ipv6addr(struct in6_addr *ipaddr);
int init_settings(char *filepath);
uint8_t calc_netmask(int count, int maskbit);
device_t *get_device(int index);
uint32_t get_devices_length();
struct in_addr get_next_router_addr();
struct in6_addr get_next_router_ipv6_addr();
uint32_t get_gateway_device_index();
uint32_t get_ratelimit_bucket();
uint32_t get_ratelimit_token_per_second();
