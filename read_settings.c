#include "log.h"
#include "parse_xml.h"
#include "settings.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int parse_settings(settings_t *settings, xml_node_t *node);
int parse_if_settings(device_t *device, xml_node_t *node);
int parse_adv_prefix(adv_prefix_t *adv_prefix, xml_node_t *node);
int construct_dns_search_suffix(char *domain, char **dns_message);
int register_ipv6_address_list(device_t *device, xml_node_t *node);
int count_children(xml_node_t *node);
uint8_t calc_netmask(int count, int maskbit);

int read_settings_from_file(settings_t *settings, char *filepath) {
    xml_node_t *root;
    root = parse_xml(filepath);
    if (root == NULL) {
        log_error("empty document.\n");
        return -1;
    }
    parse_settings(settings, root);
    free_xml_tree(root);
    return 0;
}

int parse_settings(settings_t *settings, xml_node_t *root) {
    xml_node_t *node = root->children;
    while (node != NULL) {
        if (strcmp(node->name, "Interfaces") == 0) {
            settings->devices_length = count_children(node->children);
            if ((settings->devices = calloc(settings->devices_length, sizeof(device_t))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xml_node_t *if_root = node->children;
            int i = 0;
            while (if_root != NULL) {
                if (parse_if_settings(&settings->devices[i], if_root) == -1) {
                    for (int j = 0; j < i; j++) {
                        free(settings->devices[j].netif_name);
                    }
                    free(settings);
                    return -1;
                }
                i++;
                if_root = if_root->next;
            }
        } else if (strcmp(node->name, "NextRouterAddress") == 0) {
            inet_aton((char *)node->children->content, &settings->next_router_addr);
        } else if (strcmp(node->name, "NextRouterIPv6Address") == 0) {
            inet_pton(AF_INET6, (char *)node->children->content, &settings->next_router_ipv6_addr);
        } else if (strcmp(node->name, "LogMode") == 0) {
            xml_node_t *log_node = node->children;
            while (log_node != NULL) {
                if (strcmp(log_node->name, "EnableLogStdout") == 0) {
                    settings->log_mode.enable_log_stdout = strcmp(log_node->children->content, "true") == 0;
                } else if (strcmp(log_node->name, "EnableLogError") == 0) {
                    settings->log_mode.enable_log_error = strcmp(log_node->children->content, "true") == 0;
                }
                log_node = log_node->next;
            }
        }
        node = node->next;
    }
    return 0;
}

int parse_if_settings(device_t *device, xml_node_t *if_root) {
    xml_node_t *node = if_root->children;
    while (node != NULL) {
        if (strcmp(node->name, "InterfaceName") == 0) {
            if ((device->netif_name = calloc(strlen(node->children->content) + 1, sizeof(uint8_t))) == NULL) {
                log_error("calloc");
                return -1;
            }
            strcpy(device->netif_name, (char *)node->children->content);
        } else if (strcmp(node->name, "IPv6AddressList") == 0) {
            if (register_ipv6_address_list(device, node) == -1) {
                return -1;
            }
        } else if (strcmp(node->name, "IsRouter") == 0) {
            device->adv_settings.is_router = strcmp(node->children->content, "true") == 0;
        } else if (strcmp(node->name, "AdvSendAdvertisements") == 0) {
            device->adv_settings.adv_send_advertisements = strcmp(node->children->content, "true") == 0;
        } else if (strcmp(node->name, "MaxRtrAdvInterval") == 0) {
            device->adv_settings.max_rtr_adv_interval = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "MinRtrAdvInterval") == 0) {
            device->adv_settings.min_rtr_adv_interval = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvManagedFlag") == 0) {
            device->adv_settings.adv_managed_flag = strcmp(node->children->content, "true") == 0;
        } else if (strcmp(node->name, "AdvOtherConfigFlag") == 0) {
            device->adv_settings.adv_other_config_flag = strcmp(node->children->content, "true") == 0;
        } else if (strcmp(node->name, "AdvLinkMTU") == 0) {
            device->adv_settings.adv_link_mtu = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvReachableTime") == 0) {
            device->adv_settings.adv_reachable_time = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvReachableTime") == 0) {
            device->adv_settings.adv_reachable_time = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvRetransTimer") == 0) {
            device->adv_settings.adv_retrans_timer = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvCurHopLimit") == 0) {
            device->adv_settings.adv_cur_hop_limit = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvDefaultLifetime") == 0) {
            device->adv_settings.adv_default_lifetime = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvPrefixList") == 0) {
            device->adv_settings.adv_prefix_list_length = count_children(node->children);
            if ((device->adv_settings.adv_prefix_list = calloc(device->adv_settings.adv_prefix_list_length, sizeof(adv_prefix_t))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xml_node_t *prefix_root = node->children;
            int i = 0;
            while (prefix_root != NULL) {
                if (prefix_root->name != NULL) {
                    if (parse_adv_prefix(&device->adv_settings.adv_prefix_list[i], prefix_root) == -1) {
                        return -1;
                    }
                    i++;
                }
                prefix_root = prefix_root->next;
            }
        } else if (strcmp(node->name, "AdvRecursiveDNSServerList") == 0) {
            device->adv_settings.adv_recursive_dns_server_list_length = count_children(node->children);
            if ((device->adv_settings.adv_recursive_dns_server_list = calloc(device->adv_settings.adv_recursive_dns_server_list_length, sizeof(struct in6_addr))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xml_node_t *server_root = node->children;
            int i = 0;
            while (server_root != NULL) {
                inet_pton(AF_INET6, (char *)server_root->children->content, &device->adv_settings.adv_recursive_dns_server_list[i]);
                i++;
                server_root = server_root->next;
            }
        } else if (strcmp(node->name, "AdvDNSSearchList") == 0) {
            device->adv_settings.adv_dns_search_list_length = count_children(node->children);
            if ((device->adv_settings.adv_dns_search_list = calloc(device->adv_settings.adv_dns_search_list_length, sizeof(char *))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xml_node_t *suffix_root = node->children;
            int i = 0;
            while (suffix_root != NULL) {
                if (construct_dns_search_suffix((char *)suffix_root->children->content, &device->adv_settings.adv_dns_search_list[i]) == -1) {
                    return -1;
                }
                i++;
                suffix_root = suffix_root->next;
            }
        }
        node = node->next;
    }
    return 0;
}

int parse_adv_prefix(adv_prefix_t *adv_prefix, xml_node_t *prefix_root) {
    xml_node_t *node = prefix_root->children;
    while (node != NULL) {
        if (strcmp(node->name, "AdvAddressPrefix") == 0) {
            char address[40];
            char *slash_ptr;
            if ((slash_ptr = strchr((char *)node->children->content, '/')) == NULL) {
                log_error("address prefix length is not defined.\n");
                return -1;
            }
            int address_len = slash_ptr - (char *)node->children->content;
            strncpy(address, (char *)node->children->content, address_len);
            *(address + address_len) = '\0';
            adv_prefix->adv_address_prefix_length = atoi(slash_ptr + 1);
            inet_pton(AF_INET6, address, &adv_prefix->adv_address_prefix);
        } else if (strcmp(node->name, "AdvValidLifetime") == 0) {
            adv_prefix->adv_valid_life_time = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvOnLinkFlag") == 0) {
            adv_prefix->adv_on_link_flag = strcmp(node->children->content, "true") == 0;
        } else if (strcmp(node->name, "AdvPreferredLifetime") == 0) {
            adv_prefix->adv_preferred_life_time = atoi((char *)node->children->content);
        } else if (strcmp(node->name, "AdvAutonomousFlag") == 0) {
            adv_prefix->adv_autonomous_flag = strcmp(node->children->content, "true") == 0;
        }
        node = node->next;
    }
    return 0;
}

int construct_dns_search_suffix(char *string, char **domain) {
    const int LABEL_LIMIT = 63;
    const int DOMAIN_LIMIT = 253;
    if (strlen(string) > DOMAIN_LIMIT) {
        log_error("domain is too long.");
        return -1;
    }

    char *head = string;
    char *tail = NULL;
    char domain_buf[255];
    char *buf_ptr = domain_buf;
    bool is_end = false;
    while (!is_end) {
        tail = strchr(head, '.');
        uint32_t label_len;
        if (tail != NULL) {
            label_len = tail - head;
        } else {
            label_len = strlen(head);
            is_end = true;
        }
        if (label_len > LABEL_LIMIT) {
            log_error("domain label is too long.");
            return -1;
        }
        *buf_ptr = (char)label_len;
        buf_ptr++;
        memcpy(buf_ptr, head, label_len);
        buf_ptr += label_len;
        head = tail + 1;
    }
    *buf_ptr = 0;
    int domain_len = strlen(domain_buf) + 1;
    if ((*domain = calloc(domain_len, sizeof(char))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    strcpy(*domain, domain_buf);
    return 0;
}

int count_children(xml_node_t *node) {
    int count = 0;
    while (node != NULL) {
        count++;
        node = node->next;
    }
    return count;
}

int register_ipv6_address_list(device_t *device, xml_node_t *node) {
    device->addr6_list_length = count_children(node->children);
    if ((device->addr6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    if ((device->subnet6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        free(device->addr6_list);
        log_perror("calloc");
        return -1;
    }
    if ((device->netmask6_list = calloc(device->addr6_list_length, sizeof(struct in6_addr))) == NULL) {
        free(device->addr6_list);
        free(device->subnet6_list);
        log_perror("calloc");
        return -1;
    }

    int index = 0;
    xml_node_t *addr_node = node->children;
    while (addr_node != NULL) {
        char *slash_ptr;
        if ((slash_ptr = strchr((char *)addr_node->children->content, '/')) == NULL) {
            log_error("address prefix length is not defined.\n");
            return -1;
        }
        int address_len = slash_ptr - (char *)addr_node->children->content;
        char address[40];
        strncpy(address, (char *)addr_node->children->content, address_len);
        *(address + address_len) = '\0';
        int netmask_length = atoi(slash_ptr + 1);
        inet_pton(AF_INET6, address, &device->addr6_list[index]);
        for (int i = 0; i < 16; i++) {
            device->netmask6_list[index].s6_addr[i] = calc_netmask(i, netmask_length);
            device->subnet6_list[index].s6_addr[i] = device->addr6_list[index].s6_addr[i] & device->netmask6_list[index].s6_addr[i];
        }
        index++;
        addr_node = addr_node->next;
    }

    return 0;
}
