#pragma once
#include "gumbo.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Enum denoting the type of node.  This determines the type of the node.v
 * union.
 */
typedef enum {
  /** Document node.  v will be a GumboDocument. */
  GUMBO_NODE_DOCUMENT,
  /** Element node.  v will be a GumboElement. */
  GUMBO_NODE_ELEMENT,
  /** Text node.  v will be a GumboText. */
  GUMBO_NODE_TEXT,
  /** CDATA node. v will be a GumboText. */
  GUMBO_NODE_CDATA,
  /** Comment node.  v will be a GumboText, excluding comment delimiters. */
  GUMBO_NODE_COMMENT,
  /** Text node, where all contents is whitespace.  v will be a GumboText. */
  GUMBO_NODE_WHITESPACE,
  /** Template node.  This is separate from GUMBO_NODE_ELEMENT because many
   * client libraries will want to ignore the contents of template nodes, as
   * the spec suggests.  Recursing on GUMBO_NODE_ELEMENT will do the right thing
   * here, while clients that want to include template contents should also
   * check for GUMBO_NODE_TEMPLATE.  v will be a GumboElement.  */
  GUMBO_NODE_TEMPLATE
} GumboNodeType;

/**
 * Forward declaration of GumboNode so it can be used recursively in
 * GumboNode.parent.
 */
typedef struct GumboInternalNode GumboNode;

/**
 * http://www.whatwg.org/specs/web-apps/current-work/complete/dom.html#quirks-mode
 */
typedef enum {
  GUMBO_DOCTYPE_NO_QUIRKS,
  GUMBO_DOCTYPE_QUIRKS,
  GUMBO_DOCTYPE_LIMITED_QUIRKS
} GumboQuirksModeEnum;

/**
 * Namespaces.
 * Unlike in X(HT)ML, namespaces in HTML5 are not denoted by a prefix.  Rather,
 * anything inside an <svg> tag is in the SVG namespace, anything inside the
 * <math> tag is in the MathML namespace, and anything else is inside the HTML
 * namespace.  No other namespaces are supported, so this can be an enum only.
 */
typedef enum {
  GUMBO_NAMESPACE_HTML,
  GUMBO_NAMESPACE_SVG,
  GUMBO_NAMESPACE_MATHML
} GumboNamespaceEnum;

/**
 * Parse flags.
 * We track the reasons for parser insertion of nodes and store them in a
 * bitvector in the node itself.  This lets client code optimize out nodes that
 * are implied by the HTML structure of the document, or flag constructs that
 * may not be allowed by a style guide, or track the prevalence of incorrect or
 * tricky HTML code.
 */
typedef enum {
  /**
   * A normal node - both start and end tags appear in the source, nothing has
   * been reparented.
   */
  GUMBO_INSERTION_NORMAL = 0,

  /**
   * A node inserted by the parser to fulfill some implicit insertion rule.
   * This is usually set in addition to some other flag giving a more specific
   * insertion reason; it's a generic catch-all term meaning "The start tag for
   * this node did not appear in the document source".
   */
  GUMBO_INSERTION_BY_PARSER = 1 << 0,

  /**
   * A flag indicating that the end tag for this node did not appear in the
   * document source.  Note that in some cases, you can still have
   * parser-inserted nodes with an explicit end tag: for example, "Text</html>"
   * has GUMBO_INSERTED_BY_PARSER set on the <html> node, but
   * GUMBO_INSERTED_END_TAG_IMPLICITLY is unset, as the </html> tag actually
   * exists.  This flag will be set only if the end tag is completely missing;
   * in some cases, the end tag may be misplaced (eg. a </body> tag with text
   * afterwards), which will leave this flag unset and require clients to
   * inspect the parse errors for that case.
   */
  GUMBO_INSERTION_IMPLICIT_END_TAG = 1 << 1,

  // Value 1 << 2 was for a flag that has since been removed.

  /**
   * A flag for nodes that are inserted because their presence is implied by
   * other tags, eg. <html>, <head>, <body>, <tbody>, etc.
   */
  GUMBO_INSERTION_IMPLIED = 1 << 3,

  /**
   * A flag for nodes that are converted from their end tag equivalents.  For
   * example, </p> when no paragraph is open implies that the parser should
   * create a <p> tag and immediately close it, while </br> means the same thing
   * as <br>.
   */
  GUMBO_INSERTION_CONVERTED_FROM_END_TAG = 1 << 4,

  /** A flag for nodes that are converted from the parse of an <isindex> tag. */
  GUMBO_INSERTION_FROM_ISINDEX = 1 << 5,

  /** A flag for <image> tags that are rewritten as <img>. */
  GUMBO_INSERTION_FROM_IMAGE = 1 << 6,

  /**
   * A flag for nodes that are cloned as a result of the reconstruction of
   * active formatting elements.  This is set only on the clone; the initial
   * portion of the formatting run is a NORMAL node with an IMPLICIT_END_TAG.
   */
  GUMBO_INSERTION_RECONSTRUCTED_FORMATTING_ELEMENT = 1 << 7,

  /** A flag for nodes that are cloned by the adoption agency algorithm. */
  GUMBO_INSERTION_ADOPTION_AGENCY_CLONED = 1 << 8,

  /** A flag for nodes that are moved by the adoption agency algorithm. */
  GUMBO_INSERTION_ADOPTION_AGENCY_MOVED = 1 << 9,

  /**
   * A flag for nodes that have been foster-parented out of a table (or
   * should've been foster-parented, if verbatim mode is set).
   */
  GUMBO_INSERTION_FOSTER_PARENTED = 1 << 10,
} GumboParseFlags;

/**
 * Information specific to document nodes.
 */
typedef struct {
  /**
   * An array of GumboNodes, containing the children of this element.  This will
   * normally consist of the <html> element and any comment nodes found.
   * Pointers are owned.
   */
  GumboVector /* GumboNode* */ children;

  // True if there was an explicit doctype token as opposed to it being omitted.
  bool has_doctype;

  // Fields from the doctype token, copied verbatim.
  const char* name;
  const char* public_identifier;
  const char* system_identifier;

  /**
   * Whether or not the document is in QuirksMode, as determined by the values
   * in the GumboTokenDocType template.
   */
  GumboQuirksModeEnum doc_type_quirks_mode;
} GumboDocument;

/**
 * The struct used to represent TEXT, CDATA, COMMENT, and WHITESPACE elements.
 * This contains just a block of text and its position.
 */
typedef struct {
  /**
   * The text of this node, after entities have been parsed and decoded.  For
   * comment/cdata nodes, this does not include the comment delimiters.
   */
  const char* text;

  /**
   * The original text of this node, as a pointer into the original buffer.  For
   * comment/cdata nodes, this includes the comment delimiters.
   */
  GumboStringPiece original_text;

  /**
   * The starting position of this node.  This corresponds to the position of
   * original_text, before entities are decoded.
   * */
  GumboSourcePosition start_pos;
} GumboText;

/**
 * The struct used to represent all HTML elements.  This contains information
 * about the tag, attributes, and child nodes.
 */
typedef struct {
  /**
   * An array of GumboNodes, containing the children of this element.  Pointers
   * are owned.
   */
  GumboVector /* GumboNode* */ children;

  /** The GumboTag enum for this element. */
  GumboTag tag;

  /** The GumboNamespaceEnum for this element. */
  GumboNamespaceEnum tag_namespace;

  /**
   * A GumboStringPiece pointing to the original tag text for this element,
   * pointing directly into the source buffer.  If the tag was inserted
   * algorithmically (for example, <head> or <tbody> insertion), this will be a
   * zero-length string.
   */
  GumboStringPiece original_tag;

  /**
   * A GumboStringPiece pointing to the original end tag text for this element.
   * If the end tag was inserted algorithmically, (for example, closing a
   * self-closing tag), this will be a zero-length string.
   */
  GumboStringPiece original_end_tag;

  /** The source position for the start of the start tag. */
  GumboSourcePosition start_pos;

  /** The source position for the start of the end tag. */
  GumboSourcePosition end_pos;

  /**
   * An array of GumboAttributes, containing the attributes for this tag in the
   * order that they were parsed.  Pointers are owned.
   */
  GumboVector /* GumboAttribute* */ attributes;
} GumboElement;

/**
 * A supertype for GumboElement and GumboText, so that we can include one
 * generic type in lists of children and cast as necessary to subtypes.
 */
struct GumboInternalNode {
  /** The type of node that this is. */
  GumboNodeType type;

  /** Pointer back to parent node.  Not owned. */
  GumboNode* parent;

  /** The index within the parent's children vector of this node. */
  size_t index_within_parent;

  /**
   * A bitvector of flags containing information about why this element was
   * inserted into the parse tree, including a variety of special parse
   * situations.
   */
  GumboParseFlags parse_flags;

  /** The actual node data. */
  union {
    GumboDocument document;  // For GUMBO_NODE_DOCUMENT.
    GumboElement element;    // For GUMBO_NODE_ELEMENT.
    GumboText text;          // For everything else.
  } v;
};


/**
 * The type for an allocator function.  Takes the 'userdata' member of the
 * GumboParser struct as its first argument.  Semantics should be the same as
 * malloc, i.e. return a block of size_t bytes on success or NULL on failure.
 * Allocating a block of 0 bytes behaves as per malloc.
 */
// TODO(jdtang): Add checks throughout the codebase for out-of-memory condition.
typedef void* (*GumboAllocatorFunction)(void* userdata, size_t size);

/**
 * The type for a deallocator function.  Takes the 'userdata' member of the
 * GumboParser struct as its first argument.
 */
typedef void (*GumboDeallocatorFunction)(void* userdata, void* ptr);

#ifdef __cplusplus
}
#endif