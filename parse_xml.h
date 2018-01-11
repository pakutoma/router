#pragma once

typedef struct xml_node {
    struct xml_node *next;
    struct xml_node *children;
    char *name;
    char *content;
} xml_node_t;

xml_node_t *parse_xml(char *filename);
void free_xml_tree(xml_node_t *node);
