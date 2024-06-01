#pragma once
#include <memory>
#include <string>
#include <vector>

namespace gumbo {

enum class NodeType {
  GUMBO_NODE_DOCUMENT,
  GUMBO_NODE_ELEMENT,
  GUMBO_NODE_TEXT,
  GUMBO_NODE_CDATA,
  GUMBO_NODE_COMMENT,
  GUMBO_NODE_WHITESPACE,
  GUMBO_NODE_TEMPLATE
};

class Node {
 public:
  NodeType type;
  std::vector<std::shared_ptr<Node>> children;

  Node(NodeType type) : type(type) {}
  virtual ~Node() = default;
};

class Element : public Node {
 public:
  std::string tag_name;
  std::vector<std::pair<std::string, std::string>> attributes;

  Element(const std::string& tag)
      : Node(NodeType::GUMBO_NODE_ELEMENT), tag_name(tag) {}
};

}  // namespace gumbo
