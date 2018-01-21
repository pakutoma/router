#include "create_ndp.h"
#include "log.h"
#include "ndp_waiting_list.h"
#include "send_queue.h"
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <netinet/in.h>
#include <netinet/ip6.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*
 rfc4861 10. Protocol Constants
 Node constants:
 (millisecond->second)
*/
#define MAX_MULTICAST_SOLICIT 3
#define MAX_UNICAST_SOLICIT 3
#define MAX_ANYCAST_DELAY_TIME 1
#define MAX_NEIGHBOR_ADVERTISEMENT 3
#define REACHABLE_TIME 30
#define RETRANS_TIMER 1
#define DELAY_FIRST_PROBE_TIME 5
#define MIN_RANDOM_FACTOR 0.5
#define MAX_RANDOM_FACTOR 1.5

#define IPV6ADDR_SIZE 16
#define RENEW_REACHABLE_TIME_INTERVAL 10800
#define MAX_ACK_NUMBER_INDEX 10

typedef enum {
    INCOMPLETE,
    REACHABLE,
    STALE,
    DELAY,
    PROBE
} neighbor_cache_entry_status_t;

typedef struct neighbor_cache_entry {
    struct in6_addr ipaddr;
    struct ether_addr macaddr;
    neighbor_cache_entry_status_t status;
    struct neighbor_cache_entry *next;
    time_t timer;
    uint8_t retransmit_counter;
    bool is_router;
} neighbor_cache_entry_t;

void remove_timeout_cache();
void create_entry(struct in6_addr *new_ipaddr, struct ether_addr *new_macaddr, neighbor_cache_entry_status_t status);
struct ether_addr *find_linklayer_option(struct ip6_hdr *ip6_header);
neighbor_cache_entry_t *find_entry_by_ipaddr(struct in6_addr *ipaddr);
void print_neighbor_cache();

static neighbor_cache_entry_t *head = NULL;
static uint32_t reachable_time = REACHABLE_TIME;
static uint32_t renew_reachable_time_timer;

int init_neighbor_cache() {
    if ((head = calloc(1, sizeof(neighbor_cache_entry_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    head->next = NULL;
    reachable_time = rand() % (uint32_t)(REACHABLE_TIME * MAX_RANDOM_FACTOR - REACHABLE_TIME * MIN_RANDOM_FACTOR) + (uint32_t)(REACHABLE_TIME * MIN_RANDOM_FACTOR);
    renew_reachable_time_timer = time(NULL);
    return 0;
}

bool query_macaddr(struct in6_addr *ipaddr, struct ether_addr *macaddr) {
    neighbor_cache_entry_t *entry = find_entry_by_ipaddr(ipaddr);
    if (entry == NULL) {
        create_entry(ipaddr, NULL, INCOMPLETE);
        ether_frame_t *ns_frame = create_neighbor_solicitation(ipaddr, true);
        if (ns_frame != NULL) {
            enqueue_send_queue(ns_frame);
        }
        return false;
    }

    switch (entry->status) {
        case REACHABLE:
            *macaddr = entry->macaddr;
            return true;
        case INCOMPLETE:
            return false;
        case STALE:
            entry->status = DELAY;
            entry->timer = time(NULL);
            *macaddr = entry->macaddr;
            return true;
        case DELAY:
            *macaddr = entry->macaddr;
            return true;
        case PROBE:
            return false;
    }
    return false;
}

void create_entry(struct in6_addr *new_ipaddr, struct ether_addr *new_macaddr, neighbor_cache_entry_status_t status) {
    neighbor_cache_entry_t *new_entry;
    if ((new_entry = calloc(1, sizeof(neighbor_cache_entry_t))) == NULL) {
        log_perror("calloc");
        return;
    }
    new_entry->next = head->next;
    new_entry->ipaddr = *new_ipaddr;
    if (new_macaddr != NULL) {
        new_entry->macaddr = *new_macaddr;
    }
    new_entry->status = status;
    new_entry->timer = time(NULL);
    new_entry->is_router = 0;
    new_entry->retransmit_counter = 0;
    head->next = new_entry;
}

neighbor_cache_entry_t *find_entry_by_ipaddr(struct in6_addr *ipaddr) {
    neighbor_cache_entry_t *entry = head->next;
    while (entry != NULL) {
        if (memcmp(&entry->ipaddr.s6_addr, &ipaddr->s6_addr, IPV6ADDR_SIZE * sizeof(uint8_t)) == 0) {
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

void analyze_ndp_packet(ether_frame_t *ndp_frame) {
    //cf. RFC4861 Appendix C: State Machine for the Reachability State

    struct ip6_hdr *ip6_header = (struct ip6_hdr *)ndp_frame->payload;
    struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
    time_t now = time(NULL);
    if (icmp6_header->icmp6_type == ND_NEIGHBOR_ADVERT) {
        neighbor_cache_entry_t *entry = find_entry_by_ipaddr(&ip6_header->ip6_src);
        if (entry == NULL) {
            return;
        }

        struct nd_neighbor_advert *na_header = (struct nd_neighbor_advert *)icmp6_header;
        struct ether_addr *src_macaddr = find_linklayer_option(ip6_header);
        if (src_macaddr == NULL) {
            entry->is_router = na_header->nd_na_flags_reserved & ND_NA_FLAG_ROUTER;
            return;
        }

        if (entry->status == INCOMPLETE) {
            entry->macaddr = *src_macaddr;
            send_waiting_frame(&entry->ipaddr, &entry->macaddr);
            if (na_header->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED) {
                entry->status = REACHABLE;
                entry->timer = now;
            } else {
                entry->status = STALE;
            }
        } else {
            if (!(na_header->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED) && (na_header->nd_na_flags_reserved & ND_NA_FLAG_OVERRIDE)) {
                if (memcmp(entry->macaddr.ether_addr_octet, src_macaddr->ether_addr_octet, ETH_ALEN * sizeof(uint8_t)) != 0) {
                    entry->macaddr = *src_macaddr;
                    entry->status = STALE;
                }
            }
            if ((na_header->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED) && !(na_header->nd_na_flags_reserved & ND_NA_FLAG_OVERRIDE)) {
                if (memcmp(entry->macaddr.ether_addr_octet, src_macaddr->ether_addr_octet, ETH_ALEN * sizeof(uint8_t)) == 0) {
                    entry->status = REACHABLE;
                    entry->timer = now;
                } else {
                    if (entry->status == REACHABLE) {
                        entry->status = STALE;
                    }
                }
            }
            if ((na_header->nd_na_flags_reserved & ND_NA_FLAG_SOLICITED) && (na_header->nd_na_flags_reserved & ND_NA_FLAG_OVERRIDE)) {
                if (memcmp(entry->macaddr.ether_addr_octet, src_macaddr->ether_addr_octet, ETH_ALEN * sizeof(uint8_t)) != 0) {
                    entry->macaddr = *src_macaddr;
                }
                entry->status = REACHABLE;
                entry->timer = now;
            }
        }
    } else {
        struct ether_addr *src_macaddr = find_linklayer_option(ip6_header);
        if (src_macaddr == NULL) {
            return;
        }

        neighbor_cache_entry_t *entry = find_entry_by_ipaddr(&ip6_header->ip6_src);
        if (entry == NULL) {
            create_entry(&ip6_header->ip6_src, src_macaddr, STALE);
        } else {
            if (entry->status == INCOMPLETE) {
                send_waiting_frame(&entry->ipaddr, &entry->macaddr);
                entry->status = STALE;
            } else {
                if (memcmp(entry->macaddr.ether_addr_octet, src_macaddr->ether_addr_octet, ETH_ALEN * sizeof(uint8_t)) != 0) {
                    entry->macaddr = *src_macaddr;
                    entry->status = STALE;
                }
            }
        }
    }
}

struct ether_addr *find_linklayer_option(struct ip6_hdr *ip6_header) {
    struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(ip6_header + 1);
    uint16_t length = ntohs(ip6_header->ip6_plen);
    uint16_t current = 0;
    struct nd_opt_hdr *option_ptr;
    switch (icmp6_header->icmp6_type) {
        case ND_ROUTER_ADVERT:
            option_ptr = (struct nd_opt_hdr *)(((struct nd_router_advert *)icmp6_header) + 1);
            current += sizeof(struct nd_router_advert);
            break;
        case ND_ROUTER_SOLICIT:
            option_ptr = (struct nd_opt_hdr *)(((struct nd_router_solicit *)icmp6_header) + 1);
            current += sizeof(struct nd_router_solicit);
            break;
        case ND_NEIGHBOR_ADVERT:
            option_ptr = (struct nd_opt_hdr *)(((struct nd_neighbor_advert *)icmp6_header) + 1);
            current += sizeof(struct nd_neighbor_advert);
            break;
        case ND_NEIGHBOR_SOLICIT:
            option_ptr = (struct nd_opt_hdr *)(((struct nd_neighbor_solicit *)icmp6_header) + 1);
            current += sizeof(struct nd_neighbor_solicit);
            break;
        default:
            return NULL;
    }
    while (current < length) {
        if (option_ptr->nd_opt_type == ND_OPT_SOURCE_LINKADDR || option_ptr->nd_opt_type == ND_OPT_TARGET_LINKADDR) {
            return (struct ether_addr *)(option_ptr + 1);
        }
        current += option_ptr->nd_opt_len * 8;
        option_ptr = (struct nd_opt_hdr *)(((uint8_t *)option_ptr) + option_ptr->nd_opt_len * 8);
    }
    return NULL;
}

void update_status() {
    print_neighbor_cache();
    neighbor_cache_entry_t *entry = head;
    time_t now = time(NULL);
    if (renew_reachable_time_timer - now > RENEW_REACHABLE_TIME_INTERVAL) {
        reachable_time = rand() % (uint32_t)(REACHABLE_TIME * MAX_RANDOM_FACTOR - REACHABLE_TIME * MIN_RANDOM_FACTOR) + (uint32_t)(REACHABLE_TIME * MIN_RANDOM_FACTOR);
        renew_reachable_time_timer = now;
    }

    while (entry->next != NULL) {
        switch (entry->status) {
            case REACHABLE:
                if (now - entry->next->timer > reachable_time) {
                    entry->next->status = STALE;
                }
                break;
            case INCOMPLETE:
                if (now - entry->next->timer > RETRANS_TIMER) {
                    if (entry->next->retransmit_counter < MAX_MULTICAST_SOLICIT) {
                        ether_frame_t *ns_frame = create_neighbor_solicitation(&entry->next->ipaddr, true);
                        if (ns_frame != NULL) {
                            enqueue_send_queue(ns_frame);
                        }
                        entry->next->retransmit_counter++;
                        entry->next->timer = now;
                    } else {
                        // TODO: send ICMPv6 address unreachable
                        neighbor_cache_entry_t *obsolete_entry = entry->next;
                        entry->next = entry->next->next;
                        free(obsolete_entry);
                        if (entry->next == NULL) {
                            return;
                        }
                    }
                }
                break;
            case DELAY:
                if (now - entry->next->timer > DELAY_FIRST_PROBE_TIME) {
                    entry->next->status = PROBE;
                    entry->next->timer = now;
                    ether_frame_t *ns_frame = create_neighbor_solicitation(&entry->next->ipaddr, false);
                    if (ns_frame != NULL) {
                        enqueue_send_queue(ns_frame);
                    }
                }
                break;
            case STALE:
                break;
            case PROBE:
                if (now - entry->next->timer > RETRANS_TIMER) {
                    if (entry->next->retransmit_counter < MAX_UNICAST_SOLICIT) {
                        ether_frame_t *ns_frame = create_neighbor_solicitation(&entry->next->ipaddr, false);
                        if (ns_frame != NULL) {
                            enqueue_send_queue(ns_frame);
                        }
                        entry->next->retransmit_counter++;
                        entry->next->timer = now;
                    } else {
                        neighbor_cache_entry_t *obsolete_entry = entry->next;
                        entry->next = entry->next->next;
                        free(obsolete_entry);
                        if (entry->next == NULL) {
                            return;
                        }
                    }
                }
                break;
        }
        entry = entry->next;
    }
}

void print_neighbor_cache() {
    neighbor_cache_entry_t *entry = head->next;
    log_stdout("---%s---\n", __func__);
    time_t now = time(NULL);
    char buf[100];
    while (entry != NULL) {
        log_stdout("addr: %s\n", inet_ntop(AF_INET6, &entry->ipaddr, buf, 100));
        log_stdout("status: %d(%d sec)\n", entry->status, now - entry->timer);
        entry = entry->next;
    }
    log_stdout("---end %s---\n", __func__);
}
