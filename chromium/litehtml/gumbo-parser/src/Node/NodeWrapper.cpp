#include "NodeWrapper.h"

extern "C" {

NodeWrapper* create_node(gumbo::NodeType type) {
  return new NodeWrapper{new gumbo::Node(type)};
}

void add_child(NodeWrapper* parent, NodeWrapper* child) {
  if (parent && child) {
    parent->node->children.push_back(std::shared_ptr<gumbo::Node>(child->node));
  }
}

void delete_node(NodeWrapper* node) {
  if (node) {
    delete node->node;
    delete node;
  }
}
}
