#pragma once
#include "Node.h"

extern "C" {
typedef struct NodeWrapper {
  gumbo::Node* node;
} NodeWrapper;

NodeWrapper* create_node(gumbo::NodeType type);
void add_child(NodeWrapper* parent, NodeWrapper* child);
void delete_node(NodeWrapper* node);

}
