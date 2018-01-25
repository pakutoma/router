#include "create_icmpv6.h"
#include "ether_frame.h"
#include "log.h"
#include "send_queue.h"
#include <net/ethernet.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define REASSEMBLY_TIMEOUT 60

typedef struct reassembly_node {
    struct reassembly_node *next;
    uint32_t id;
    time_t timer;
    ether_frame_t *ether_frame;
} reassembly_node_t;

typedef struct {
    ether_frame_t *ether_frame;
    struct ip6_hdr *ip6_header;
    uint8_t next_header;
    struct ip6_frag *fragment_header;
    uint8_t *payload;
    int payload_size;
    int per_fragment_header_size;
} fragment_info_t;

static reassembly_node_t *head;
static uint32_t fragment_id;

void delete_next_node(reassembly_node_t *node, bool is_retain_frame);
fragment_info_t get_fragment_info(ether_frame_t *ether_frame);
reassembly_node_t *create_new_node(fragment_info_t *fragment_info);
int add_fragment(reassembly_node_t *node, fragment_info_t *fragment_info);
uint8_t get_tail_per_fragment_header(struct ip6_hdr *ip6_header, void **tail);

int init_reassembly_list() {
    if ((head = calloc(1, sizeof(reassembly_node_t))) == NULL) {
        log_perror("calloc");
        return -1;
    }
    head->next = NULL;
    fragment_id = rand();
    return 0;
}

void count_reassembly_time() {
    reassembly_node_t *node = head;
    reassembly_node_t *obsolete_node = NULL;
    time_t now = time(NULL);
    while (node->next != NULL) {
        if ((now - node->next->timer) > REASSEMBLY_TIMEOUT) {
            obsolete_node = node->next;
            node->next = node->next->next;
            ether_frame_t *parameter_problem_frame;
            if ((parameter_problem_frame = create_icmpv6_error(obsolete_node->ether_frame, ICMP6_TIME_EXCEEDED, ICMP6_TIME_EXCEED_REASSEMBLY, 0)) == NULL) {
                return;
            }
            enqueue_send_queue(parameter_problem_frame);
            free(obsolete_node->ether_frame);
            free(obsolete_node);
            if (node->next == NULL) {
                return;
            }
        }
        node = node->next;
    }
}

bool exists_fragment_header(ether_frame_t *ether_frame) {
    struct ip6_hdr *ip6_header = (struct ip6_hdr *)ether_frame->payload;
    void *tail;
    return get_tail_per_fragment_header(ip6_header, &tail) == 44;
}

ether_frame_t *reassemble_fragment_packet(ether_frame_t *ether_frame) {
    if (!exists_fragment_header(ether_frame)) {
        free(ether_frame->payload);
        free(ether_frame);
        return NULL;
    }
    fragment_info_t fragment_info = get_fragment_info(ether_frame);

    if (fragment_info.fragment_header->ip6f_offlg & IP6F_MORE_FRAG && fragment_info.fragment_header->ip6f_reserved % 8 != 0) {
        ether_frame_t *parameter_problem_frame;
        if ((parameter_problem_frame = create_icmpv6_error(ether_frame, ICMP6_PARAM_PROB, ICMP6_PARAMPROB_OPTION, fragment_info.per_fragment_header_size + 16)) == NULL) {
            free(ether_frame->payload);
            free(ether_frame);
            return NULL;
        }
        enqueue_send_queue(parameter_problem_frame);
        free(ether_frame->payload);
        free(ether_frame);
        return NULL;
    }

    reassembly_node_t *node = head;
    while (node->next != NULL) {
        if (node->next->id == ntohl(fragment_info.fragment_header->ip6f_ident)) {
            break;
        }
        node = node->next;
    }

    if (node->next != NULL) {
        if (add_fragment(node, &fragment_info) == -1) {
            free(ether_frame->payload);
            free(ether_frame);
            return NULL;
        }
    } else {
        reassembly_node_t *new_node;
        if ((new_node = create_new_node(&fragment_info)) == NULL) {
            free(ether_frame->payload);
            free(ether_frame);
            return NULL;
        }
        new_node->next = head->next;
        head->next = new_node;
        node = head;
    }

    if (~fragment_info.fragment_header->ip6f_offlg & IP6F_MORE_FRAG) {
        ether_frame_t *finished_frame = node->next->ether_frame;
        delete_next_node(node, true);
        free(ether_frame->payload);
        free(ether_frame);
        return finished_frame;
    }
    free(ether_frame->payload);
    free(ether_frame);
    return NULL;
}

int send_large_frame(ether_frame_t *ether_frame, int mtu) {
    fragment_info_t fragment_info = get_fragment_info(ether_frame);
    int payload_mtu = mtu - (fragment_info.per_fragment_header_size + sizeof(struct ip6_frag));
    int fragment_count = fragment_info.payload_size / payload_mtu;
    int modulo = fragment_info.payload_size % payload_mtu;
    if (modulo > 0) {
        fragment_count++;
    }
    struct ip6_frag *ip6frag_header;
    if ((ip6frag_header = calloc(1, sizeof(struct ip6_frag))) == NULL) {
        return -1;
    }
    ip6frag_header->ip6f_ident = fragment_id;
    fragment_id++;
    ip6frag_header->ip6f_nxt = fragment_info.next_header;

    uint16_t offset = 0;
    for (size_t i = 0; i < fragment_count; i++) {
        ether_frame_t *fragment_frame;
        if ((fragment_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
            free(ip6frag_header);
            return -1;
        }
        *fragment_frame = *ether_frame;

        if (i == fragment_count - 1) {
            if (modulo > 0) {
                fragment_frame->payload_size = modulo + fragment_info.per_fragment_header_size + sizeof(struct ip6_frag);
                payload_mtu = modulo;
            } else {
                fragment_frame->payload_size = mtu;
            }
            ip6frag_header->ip6f_offlg = 0;
        } else {
            fragment_frame->payload_size = mtu;
            ip6frag_header->ip6f_offlg = IP6F_MORE_FRAG;
        }
        if ((fragment_frame->payload = calloc(fragment_frame->payload_size, sizeof(uint8_t))) == NULL) {
            free(ip6frag_header);
            free(fragment_frame);
            return -1;
        }
        ip6frag_header->ip6f_offlg += htons(offset);

        memcpy(fragment_frame->payload, fragment_info.ether_frame->payload, fragment_info.per_fragment_header_size);
        uint8_t *frag_ptr = fragment_frame->payload + fragment_info.per_fragment_header_size;
        memcpy(frag_ptr, ip6frag_header, sizeof(struct ip6_frag));
        uint8_t *payload_ptr = frag_ptr + sizeof(struct ip6_frag);
        memcpy(payload_ptr, fragment_info.payload + offset, payload_mtu);
        offset += payload_mtu;

        struct ip6_hdr *ip6_header = (struct ip6_hdr *)fragment_frame->payload;
        ip6_header->ip6_plen = htons(fragment_frame->payload_size - sizeof(struct ip6_hdr));
        ip6_header->ip6_nxt = 44;

        if (enqueue_send_queue(fragment_frame) == -1) {
            return -1;
        }
    }
    free(ether_frame->payload);
    free(ether_frame);
    return 0;
}

fragment_info_t get_fragment_info(ether_frame_t *ether_frame) {
    fragment_info_t fragment_info;
    fragment_info.ether_frame = ether_frame;
    fragment_info.ip6_header = (struct ip6_hdr *)ether_frame->payload;
    void *payload;
    fragment_info.next_header = get_tail_per_fragment_header(fragment_info.ip6_header, &payload);
    if (fragment_info.next_header == 44) {
        fragment_info.fragment_header = payload;
        fragment_info.payload = (uint8_t *)(fragment_info.fragment_header + 1);
        fragment_info.per_fragment_header_size = (uint8_t *)fragment_info.fragment_header - (uint8_t *)fragment_info.ip6_header;
        fragment_info.payload_size = ether_frame->payload_size - fragment_info.per_fragment_header_size - sizeof(struct ip6_frag);
    } else {
        fragment_info.fragment_header = NULL;
        fragment_info.payload = payload;
        fragment_info.per_fragment_header_size = fragment_info.payload - (uint8_t *)fragment_info.ip6_header;
        fragment_info.payload_size = ether_frame->payload_size - fragment_info.per_fragment_header_size;
    }
    return fragment_info;
}

uint8_t get_tail_per_fragment_header(struct ip6_hdr *ip6_header, void **tail) {
    uint8_t next = ip6_header->ip6_nxt;
    *tail = (uint8_t *)(ip6_header + 1);
    while (next == 43 || next == 0 || next == 60) {
        struct ip6_ext *ext_header = (struct ip6_ext *)tail;
        uint8_t len = ext_header->ip6e_len;
        next = ext_header->ip6e_nxt;
        *tail = ((uint8_t *)ext_header) + len;
    }
    return next;
}

void delete_next_node(reassembly_node_t *node, bool is_retain_frame) {
    reassembly_node_t *next = node->next->next;
    if (!is_retain_frame) {
        free(head->next->ether_frame);
    }
    free(head->next);
    node->next = next;
}

int add_fragment(reassembly_node_t *node, fragment_info_t *fragment_info) {
    ether_frame_t *node_frame = node->next->ether_frame;
    int add_payload_top = node_frame->payload_size;
    node_frame->payload_size += fragment_info->payload_size;
    struct ip6_hdr *node_ip6hdr = (struct ip6_hdr *)node_frame->payload;
    node_ip6hdr->ip6_plen = htons(ntohs(node_ip6hdr->ip6_plen) + fragment_info->payload_size);
    uint8_t *tmp;
    if ((tmp = realloc(node_frame->payload, node_frame->payload_size)) == NULL) {
        delete_next_node(node, false);
        return -1;
    }
    node_frame->payload = tmp;
    memcpy(node_frame->payload + add_payload_top, fragment_info->payload, fragment_info->payload_size);
    return 0;
}

reassembly_node_t *create_new_node(fragment_info_t *fragment_info) {
    reassembly_node_t *new;
    if ((new = calloc(1, sizeof(reassembly_node_t))) == NULL) {
        return NULL;
    }
    new->timer = time(NULL);
    new->next = NULL;
    new->id = ntohl(fragment_info->fragment_header->ip6f_ident);
    if ((new->ether_frame = calloc(1, sizeof(ether_frame_t))) == NULL) {
        return NULL;
    }
    *new->ether_frame = *fragment_info->ether_frame;
    new->ether_frame->payload_size = fragment_info->per_fragment_header_size + fragment_info->payload_size;
    if ((new->ether_frame->payload = calloc(new->ether_frame->payload_size, sizeof(uint8_t))) == NULL) {
        return NULL;
    }
    memcpy(new->ether_frame->payload, fragment_info->ip6_header, fragment_info->per_fragment_header_size);
    memcpy(new->ether_frame->payload + fragment_info->per_fragment_header_size, (uint8_t *)fragment_info->payload, fragment_info->payload_size);
    struct ip6_hdr *ip6_header = (struct ip6_hdr *)new->ether_frame->payload;
    ip6_header->ip6_nxt = fragment_info->fragment_header->ip6f_nxt;
    ip6_header->ip6_plen = htons(ntohs(ip6_header->ip6_plen) - sizeof(struct ip6_frag));
    return new;
}
