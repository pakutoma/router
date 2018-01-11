#include "log.h"
#include "parse_xml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_tag_name(char *ptr, char *name);
xml_node_t *parse_node(char *begin, char *end);
xml_node_t *create_leaf_node(char *begin, char *end);
char *get_end_tag(char *begin_name, char *begin, char *end);

void free_xml_tree(xml_node_t *node) {
    if (node->name != NULL) {
        free_xml_tree(node->children);
        if (node->next != NULL) {
            free_xml_tree(node->next);
        }
    } else {
        free(node->content);
    }
    free(node);
}

xml_node_t *parse_xml(char *filename) {
    FILE *fp = fopen(filename, "r");
    char xml_data[10000] = {0};
    char *begin = xml_data;
    char *ptr = begin;
    char buf[100] = {0};
    fgets(buf, 100, fp);
    while (fgets(buf, 100, fp) != NULL) {
        sscanf(buf, "%s", ptr);
        while (*ptr != '\0') {
            ptr++;
        }
    }
    return parse_node(begin, ptr);
}

xml_node_t *parse_node(char *begin, char *end) {
    char name_buf[100];
    char *child_begin = begin;
    char *child_end = NULL;
    xml_node_t top_node;
    top_node.next = NULL;

    while (child_begin < end) {
        if ((child_begin = get_tag_name(child_begin, name_buf)) == NULL) {
            return create_leaf_node(begin, end);
        }
        if ((child_end = get_end_tag(name_buf, child_begin, end)) == NULL) {
            return NULL;
        }
        xml_node_t *current_node = &top_node;
        while (current_node->next != NULL) {
            current_node = current_node->next;
        }
        if ((current_node->next = calloc(1, sizeof(xml_node_t))) == NULL) {
            log_perror("calloc");
            return NULL;
        }
        current_node = current_node->next;
        current_node->next = NULL;
        current_node->content = NULL;
        if ((current_node->name = calloc(strlen(name_buf) + 1, sizeof(char))) == NULL) {
            log_perror("calloc");
            return 0;
        }
        strcpy(current_node->name, name_buf);
        current_node->children = parse_node(child_begin, child_end);
        child_begin = child_end + (strlen(name_buf) + 3);
    }
    return top_node.next;
}

xml_node_t *create_leaf_node(char *begin, char *end) {
    xml_node_t *node;
    if ((node = calloc(1, sizeof(xml_node_t))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    node->children = NULL;
    node->name = NULL;
    node->next = NULL;
    if ((node->content = calloc(end - begin + 1, sizeof(char))) == NULL) {
        log_perror("calloc");
        return NULL;
    }
    memcpy(node->content, begin, end - begin);
    *(node->content + (end - begin) + 1) = '\0';
    return node;
}

char *get_tag_name(char *ptr, char *name) {
    if (*ptr != '<') {
        return NULL;
    }
    ptr++;
    if (*ptr == '/') {
        ptr++;
    }
    for (size_t i = 0; i < 100; i++) {
        if (*ptr == '>') {
            *name = '\0';
            return ptr + 1;
        }
        *name = *ptr;
        ptr++;
        name++;
    }
    return NULL;
}

char *get_end_tag(char *begin_name, char *begin, char *end) {
    char *ptr = begin;
    while (ptr != end) {
        if (*ptr == '<') {
            char name[100] = {0};
            char *end_tag = get_tag_name(ptr, name);
            if (*(ptr + 1) == '/' && strcmp(begin_name, name) == 0) {
                return end_tag - (strlen(begin_name) + 3);
            }
        }
        ptr++;
    }
    return NULL;
}
