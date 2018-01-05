#include "log.h"
#include "settings.h"
#include <arpa/inet.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>

int parse_settings(settings_t *settings, xmlNode *node);
int parse_if_settings(device_t *device, xmlNode *node);
int parse_adv_prefix(adv_prefix_t *adv_prefix, xmlNode *node);
int construct_dns_search_suffix(char *domain, char **dns_message);
int count_children(xmlNode *node);

int read_settings_from_file(settings_t *settings, char *filepath) {
    xmlDoc *doc;
    doc = xmlParseFile(filepath);
    if (doc == NULL) {
        log_error("Document not parsed successfully. \n");
        return -1;
    }
    xmlNode *root;
    root = xmlDocGetRootElement(doc);
    if (root == NULL) {
        log_error("empty document.\n");
        xmlFreeDoc(doc);
        return -1;
    }
    parse_settings(settings, root);
    xmlFreeDoc(doc);
    return 0;
}

int parse_settings(settings_t *settings, xmlNode *root) {
    xmlNode *node = root->children;
    while (node != NULL) {
        if (xmlStrcmp(node->name, BAD_CAST "Interfaces") == 0) {
            settings->devices_length = count_children(node->children);
            if ((settings->devices = calloc(settings->devices_length, sizeof(device_t))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xmlNode *if_root = node->children;
            int i = 0;
            while (if_root != NULL) {
                if (if_root->type == XML_ELEMENT_NODE) {
                    if (parse_if_settings(&settings->devices[i], if_root) == -1) {
                        for (int j = 0; j < i; j++) {
                            free(settings->devices[j].netif_name);
                        }
                        free(settings);
                        return -1;
                    }
                    i++;
                }
                if_root = if_root->next;
            }
        } else if (xmlStrcmp(node->name, BAD_CAST "NextRouterAddress") == 0) {
            inet_aton((char *)node->children->content, &settings->next_router_addr);
        } else if (xmlStrcmp(node->name, BAD_CAST "NextRouterIPv6Address") == 0) {
            inet_pton(AF_INET6, (char *)node->children->content, &settings->next_router_ipv6_addr);
        } else if (xmlStrcmp(node->name, BAD_CAST "LogMode") == 0) {
            xmlNode *log_node = node->children;
            while (log_node != NULL) {
                if (xmlStrcmp(log_node->name, BAD_CAST "EnableLogStdout") == 0) {
                    settings->log_mode.enable_log_stdout = xmlStrcmp(log_node->children->content, BAD_CAST "true") == 0;
                } else if (xmlStrcmp(log_node->name, BAD_CAST "EnableLogError") == 0) {
                    settings->log_mode.enable_log_error = xmlStrcmp(log_node->children->content, BAD_CAST "true") == 0;
                }
                log_node = log_node->next;
            }
        }
        node = node->next;
    }
    return 0;
}

int parse_if_settings(device_t *device, xmlNode *if_root) {
    xmlNode *node = if_root->children;
    while (node != NULL) {
        if (xmlStrcmp(node->name, BAD_CAST "InterfaceName") == 0) {
            if ((device->netif_name = calloc(xmlStrlen(node->children->content) + 1, sizeof(uint8_t))) == NULL) {
                log_error("calloc");
                return -1;
            }
            strcpy(device->netif_name, (char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "IsRouter") == 0) {
            device->adv_settings.is_router = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvSendAdvertisements") == 0) {
            device->adv_settings.adv_send_advertisements = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
        } else if (xmlStrcmp(node->name, BAD_CAST "MaxRtrAdvInterval") == 0) {
            device->adv_settings.max_rtr_adv_interval = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "MinRtrAdvInterval") == 0) {
            device->adv_settings.min_rtr_adv_interval = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvManagedFlag") == 0) {
            device->adv_settings.adv_managed_flag = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvOtherConfigFlag") == 0) {
            device->adv_settings.adv_other_config_flag = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvLinkMTU") == 0) {
            device->adv_settings.adv_link_mtu = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvReachableTime") == 0) {
            device->adv_settings.adv_reachable_time = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvReachableTime") == 0) {
            device->adv_settings.adv_reachable_time = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvRetransTimer") == 0) {
            device->adv_settings.adv_retrans_timer = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvCurHopLimit") == 0) {
            device->adv_settings.adv_cur_hop_limit = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvDefaultLifetime") == 0) {
            device->adv_settings.adv_default_lifetime = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvPrefixList") == 0) {
            device->adv_settings.adv_prefix_list_length = count_children(node->children);
            if ((device->adv_settings.adv_prefix_list = calloc(device->adv_settings.adv_prefix_list_length, sizeof(adv_prefix_t))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xmlNode *prefix_root = node->children;
            int i = 0;
            while (prefix_root != NULL) {
                if (prefix_root->type == XML_ELEMENT_NODE) {
                    if (parse_adv_prefix(&device->adv_settings.adv_prefix_list[i], prefix_root) == -1) {
                        return -1;
                    }
                    i++;
                }
                prefix_root = prefix_root->next;
            }
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvRecursiveDNSServerList") == 0) {
            device->adv_settings.adv_recursive_dns_server_list_length = count_children(node->children);
            if ((device->adv_settings.adv_recursive_dns_server_list = calloc(device->adv_settings.adv_recursive_dns_server_list_length, sizeof(struct in6_addr))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xmlNode *server_root = node->children;
            int i = 0;
            while (server_root != NULL) {
                if (server_root->type == XML_ELEMENT_NODE) {
                    inet_pton(AF_INET6, (char *)server_root->children->content, &device->adv_settings.adv_recursive_dns_server_list[i]);
                    i++;
                }
                server_root = server_root->next;
            }
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvDNSSearchList") == 0) {
            device->adv_settings.adv_dns_search_list_length = count_children(node->children);
            if ((device->adv_settings.adv_dns_search_list = calloc(device->adv_settings.adv_dns_search_list_length, sizeof(char *))) == NULL) {
                log_perror("calloc");
                return -1;
            }

            xmlNode *suffix_root = node->children;
            int i = 0;
            while (suffix_root != NULL) {
                if (suffix_root->type == XML_ELEMENT_NODE) {
                    if (construct_dns_search_suffix((char *)suffix_root->children->content, &device->adv_settings.adv_dns_search_list[i]) == -1) {
                        return -1;
                    }
                    i++;
                }
                suffix_root = suffix_root->next;
            }
        }
        node = node->next;
    }
    return 0;
}

int parse_adv_prefix(adv_prefix_t *adv_prefix, xmlNode *prefix_root) {
    xmlNode *node = prefix_root->children;
    while (node != NULL) {
        if (xmlStrcmp(node->name, BAD_CAST "AdvAddressPrefix") == 0) {
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
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvValidLifetime") == 0) {
            adv_prefix->adv_valid_life_time = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvOnLinkFlag") == 0) {
            adv_prefix->adv_on_link_flag = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvPreferredLifetime") == 0) {
            adv_prefix->adv_preferred_life_time = atoi((char *)node->children->content);
        } else if (xmlStrcmp(node->name, BAD_CAST "AdvAutonomousFlag") == 0) {
            adv_prefix->adv_autonomous_flag = xmlStrcmp(node->children->content, BAD_CAST "true") == 0;
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

int count_children(xmlNode *node) {
    int count = 0;
    while (node != NULL) {
        if (node->type == XML_ELEMENT_NODE) {
            count++;
        }
        node = node->next;
    }
    return count;
}
