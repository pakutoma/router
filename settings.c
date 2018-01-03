#define __STDC_FORMAT_MACROS
#include "init_device.h"
#include "log.h"
#include "read_settings.h"
#include "settings.h"
#include <arpa/inet.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <string.h>

static settings_t settings;

void print_settings();
const char *domain_to_string(char *domain, char *buf, size_t size);

int init_settings(char *filepath) {
    if (read_settings_from_file(&settings, filepath) == -1) {
        return -1;
    }
    bool exists_next_router_gateway = false;
    for (int i = 0; i < settings.devices_length; i++) {
        if (init_device(&settings.devices[i]) == -1) {
            return -1;
        }
        if (settings.devices[i].subnet.s_addr == (settings.next_router_addr.s_addr & settings.devices[i].netmask.s_addr)) {
            exists_next_router_gateway = true;
            settings.gateway_device_index = i;
        }
    }
    if (!exists_next_router_gateway) {
        log_error("couldn't find gateway device.");
        return -1;
    }
    set_log_mode(settings.log_mode.enable_log_stdout, settings.log_mode.enable_log_error);
    print_settings();
    return 0;
}

device_t *get_device(int index) {
    return &settings.devices[index];
}

uint32_t get_devices_length() {
    return settings.devices_length;
}

struct in_addr get_next_router_addr() {
    return settings.next_router_addr;
}

uint32_t get_gateway_device_index() {
    return settings.gateway_device_index;
}

device_t *find_device_by_macaddr(uint8_t macaddr[ETH_ALEN]) {
    for (int i = 0; i < settings.devices_length; i++) {
        if (memcmp(settings.devices[i].hw_addr, macaddr, sizeof(uint8_t) * ETH_ALEN) == 0) {
            return &settings.devices[i];
        }
    }
    return NULL;
}

device_t *find_device_by_ipaddr(uint32_t ipaddr) {
    for (int i = 0; i < settings.devices_length; i++) {
        if (settings.devices[i].subnet.s_addr == (ipaddr & settings.devices[i].netmask.s_addr)) {
            return &settings.devices[i];
        }
    }
    return NULL;
}

int find_device_index_by_macaddr(uint8_t macaddr[ETH_ALEN]) {
    for (int i = 0; i < settings.devices_length; i++) {
        if (memcmp(settings.devices[i].hw_addr, macaddr, sizeof(uint8_t) * ETH_ALEN) == 0) {
            return i;
        }
    }
    return -1;
}

int find_device_index_by_sock_desc(int sock_desc) {
    for (int i = 0; i < settings.devices_length; i++) {
        if (settings.devices[i].sock_desc == sock_desc) {
            return i;
        }
    }
    return -1;
}

void print_settings() {
    char buf[253];
    log_stdout("---%s---\n", __func__);
    log_stdout("next_router_addr: %s\n", inet_ntoa(settings.next_router_addr));
    log_stdout("next_router_ipv6_addr: %s\n", inet_ntop(AF_INET6, &settings.next_router_ipv6_addr, buf, 253));
    log_stdout("gateway_device_index: %" PRIu32 "\n", settings.gateway_device_index);
    log_stdout("devices_length: %" PRIu32 "\n", settings.devices_length);
    log_stdout("devices:\n");
    for (int i = 0; i < settings.devices_length; i++) {
        log_stdout("    device%d:\n", i);
        log_stdout("        netif_name: %s\n", settings.devices[i].netif_name);
        log_stdout("        sock_desc: %d\n", settings.devices[i].sock_desc);
        log_stdout("        hw_addr: %02x:%02x:%02x:%02x:%02x:%02x\n", settings.devices[i].hw_addr[0], settings.devices[i].hw_addr[1], settings.devices[i].hw_addr[2], settings.devices[i].hw_addr[3], settings.devices[i].hw_addr[4], settings.devices[i].hw_addr[5]);
        log_stdout("        addr: %s\n", inet_ntoa(settings.devices[i].addr));
        log_stdout("        subnet: %s\n", inet_ntoa(settings.devices[i].subnet));
        log_stdout("        netmask: %s\n", inet_ntoa(settings.devices[i].netmask));
        log_stdout("        adv_settings:\n");
        log_stdout("            is_router: %s\n", settings.devices[i].adv_settings.is_router ? "true" : "false");
        log_stdout("            adv_send_advertisements: %s\n", settings.devices[i].adv_settings.adv_send_advertisements ? "true" : "false");
        log_stdout("            max_rtr_adv_interval: %d\n", settings.devices[i].adv_settings.max_rtr_adv_interval);
        log_stdout("            min_rtr_adv_interval: %d\n", settings.devices[i].adv_settings.min_rtr_adv_interval);
        log_stdout("            adv_managed_flag: %s\n", settings.devices[i].adv_settings.adv_managed_flag ? "true" : "false");
        log_stdout("            adv_other_config_flag: %s\n", settings.devices[i].adv_settings.adv_other_config_flag ? "true" : "false");
        log_stdout("            adv_link_mtu: %d\n", settings.devices[i].adv_settings.adv_link_mtu);
        log_stdout("            adv_reachable_time: %d\n", settings.devices[i].adv_settings.adv_reachable_time);
        log_stdout("            adv_retrans_timer: %d\n", settings.devices[i].adv_settings.adv_retrans_timer);
        log_stdout("            adv_cur_hop_limit: %d\n", settings.devices[i].adv_settings.adv_cur_hop_limit);
        log_stdout("            adv_default_life_time: %d\n", settings.devices[i].adv_settings.adv_default_life_time);
        log_stdout("            adv_prefix_list_length: %d\n", settings.devices[i].adv_settings.adv_prefix_list_length);
        log_stdout("            prefix_list:\n");
        for (int j = 0; j < settings.devices[i].adv_settings.adv_prefix_list_length; j++) {
            log_stdout("                prefix%d:\n", j);
            log_stdout("                    adv_address_prefix: %s\n", inet_ntop(AF_INET6, &settings.devices[i].adv_settings.adv_prefix_list[j].adv_address_prefix, buf, 253));
            log_stdout("                    adv_address_prefix_length: %d\n", settings.devices[i].adv_settings.adv_prefix_list[j].adv_address_prefix_length);
            log_stdout("                    adv_valid_life_time: %d\n", settings.devices[i].adv_settings.adv_prefix_list[j].adv_valid_life_time);
            log_stdout("                    adv_on_link_flag: %s\n", settings.devices[i].adv_settings.adv_prefix_list[j].adv_on_link_flag ? "true" : "false");
            log_stdout("                    adv_preferred_life_time: %d\n", settings.devices[i].adv_settings.adv_prefix_list[j].adv_preferred_life_time);
            log_stdout("                    adv_autonomous_flag: %s\n", settings.devices[i].adv_settings.adv_prefix_list[j].adv_autonomous_flag ? "true" : "false");
            for (int k = 0; k < settings.devices[i].adv_settings.adv_prefix_list[j].adv_recursive_dns_server_list_length; k++) {
                log_stdout("                        adv_recursive_dns_server: %s\n", inet_ntop(AF_INET6, &settings.devices[i].adv_settings.adv_prefix_list[j].adv_recursive_dns_server_list[k], buf, 253));
            }
            for (int k = 0; k < settings.devices[i].adv_settings.adv_prefix_list[j].adv_dns_search_list_length; k++) {
                log_stdout("                        adv_dns_search_suffix: %s\n", domain_to_string(settings.devices[i].adv_settings.adv_prefix_list[j].adv_dns_search_list[k], buf, 253));
            }
        }
    }
    log_stdout("log_mode:\n");
    log_stdout("    enable_log_stdout: %s\n", settings.log_mode.enable_log_stdout ? "true" : "false");
    log_stdout("    enable_log_error: %s\n", settings.log_mode.enable_log_error ? "true" : "false");
    log_stdout("---end %s---\n", __func__);
}

const char *domain_to_string(char *domain, char *buf, size_t size) {
    if (strlen(domain) - 2 > size) {
        return NULL;
    }
    char *buf_ptr = buf;
    char *domain_ptr = domain;
    while (*domain_ptr != 0) {
        int len = *domain_ptr;
        domain_ptr++;
        memcpy(buf_ptr, domain_ptr, len);
        domain_ptr += len;
        buf_ptr += len;
        *buf_ptr = '.';
        buf_ptr++;
    }
    *(--buf_ptr) = 0;
    return buf;
}
