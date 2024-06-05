// Copyright 2010 Google Inc. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: jdtang@google.com (Jonathan Tang)
//
// We use Gumbo as a prefix for types, gumbo_ as a prefix for functions, and
// GUMBO_ as a prefix for enum constants (static constants get the Google-style
// kGumbo prefix).

/**
 * @file
 * @mainpage Gumbo HTML Parser
 *
 * This provides a conformant, no-dependencies implementation of the HTML5
 * parsing algorithm.  It supports only UTF8; if you need to parse a different
 * encoding, run a preprocessing step to convert to UTF8.  It returns a parse
 * tree made of the structs in this file.
 *
 * Example:
 * @code
 *    GumboOutput* output = gumbo_parse(input);
 *    do_something_with_doctype(output->document);
 *    do_something_with_html_tree(output->root);
 *    gumbo_destroy_output(&options, output);
 * @endcode
 * HTML5 Spec:
 *
 * http://www.whatwg.org/specs/web-apps/current-work/multipage/syntax.html
 */

#ifndef GUMBO_GUMBO_H_
#define GUMBO_GUMBO_H_


#include <stdbool.h>
#include <stddef.h>
#include "Vector/vectorWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A struct representing a character position within the original text buffer.
 * Line and column numbers are 1-based and offsets are 0-based, which matches
 * how most editors and command-line tools work.  Also, columns measure
 * positions in terms of characters while offsets measure by bytes; this is
 * because the offset field is often used to pull out a particular region of
 * text (which in most languages that bind to C implies pointer arithmetic on a
 * buffer of bytes), while the column field is often used to reference a
 * particular column on a printable display, which nowadays is usually UTF-8.
 */
typedef struct {
  unsigned int line;
  unsigned int column;
  unsigned int offset;
} GumboSourcePosition;

/**
 * A SourcePosition used for elements that have no source position, i.e.
 * parser-inserted elements.
 */
extern const GumboSourcePosition kGumboEmptySourcePosition;

/**
 * A struct representing a string or part of a string.  Strings within the
 * parser are represented by a char* and a length; the char* points into
 * an existing data buffer owned by some other code (often the original input).
 * GumboStringPieces are assumed (by convention) to be immutable, because they
 * may share data.  Use GumboStringBuffer if you need to construct a string.
 * Clients should assume that it is not NUL-terminated, and should always use
 * explicit lengths when manipulating them.
 */
typedef struct {
  /** A pointer to the beginning of the string.  NULL iff length == 0. */
  const char* data;

  /** The length of the string fragment, in bytes.  May be zero. */
  size_t length;
} GumboStringPiece;

/** A constant to represent a 0-length null string. */
extern const GumboStringPiece kGumboEmptyString;

/**
 * Compares two GumboStringPieces, and returns true if they're equal or false
 * otherwise.
 */
bool gumbo_string_equals(
    const GumboStringPiece* str1, const GumboStringPiece* str2);

/**
 * Compares two GumboStringPieces ignoring case, and returns true if they're
 * equal or false otherwise.
 */
bool gumbo_string_equals_ignore_case(
    const GumboStringPiece* str1, const GumboStringPiece* str2);

/**
 * Returns the first index at which an element appears in this vector (testing
 * by pointer equality), or -1 if it never does.
 */
int gumbo_vector_index_of(GumboVector* vector, const void* element);

/**
 * An enum for all the tags defined in the HTML5 standard.  These correspond to
 * the tag names themselves.  Enum constants exist only for tags which appear in
 * the spec itself (or for tags with special handling in the SVG and MathML
 * namespaces); any other tags appear as GUMBO_TAG_UNKNOWN and the actual tag
 * name can be obtained through original_tag.
 *
 * This is mostly for API convenience, so that clients of this library don't
 * need to perform a strcasecmp to find the normalized tag name.  It also has
 * efficiency benefits, by letting the parser work with enums instead of
 * strings.
 */
typedef enum {
// Load all the tags from an external source, generated from tag.in.
#include "tag_enum.h"
  // Used for all tags that don't have special handling in HTML.  Add new tags
  // to the end of tag.in so as to preserve backwards-compatibility.
  GUMBO_TAG_UNKNOWN,
  // A marker value to indicate the end of the enum, for iterating over it.
  // Also used as the terminator for varargs functions that take tags.
  GUMBO_TAG_LAST,
} GumboTag;

/**
 * Returns the normalized (usually all-lowercased, except for foreign content)
 * tag name for an GumboTag enum.  Return value is static data owned by the
 * library.
 */
const char* gumbo_normalized_tagname(GumboTag tag);

/**
 * Extracts the tag name from the original_text field of an element or token by
 * stripping off </> characters and attributes and adjusting the passed-in
 * GumboStringPiece appropriately.  The tag name is in the original case and
 * shares a buffer with the original text, to simplify memory management.
 * Behavior is undefined if a string-piece that doesn't represent an HTML tag
 * (<tagname> or </tagname>) is passed in.  If the string piece is completely
 * empty (NULL data pointer), then this function will exit successfully as a
 * no-op.
 */
void gumbo_tag_from_original_text(GumboStringPiece* text);

/**
 * Fixes the case of SVG elements that are not all lowercase.
 * http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#parsing-main-inforeign
 * This is not done at parse time because there's no place to store a mutated
 * tag name.  tag_name is an enum (which will be TAG_UNKNOWN for most SVG tags
 * without special handling), while original_tag_name is a pointer into the
 * original buffer.  Instead, we provide this helper function that clients can
 * use to rename SVG tags as appropriate.
 * Returns the case-normalized SVG tagname if a replacement is found, or NULL if
 * no normalization is called for.  The return value is static data and owned by
 * the library.
 */
const char* gumbo_normalize_svg_tagname(const GumboStringPiece* tagname);

/**
 * Converts a tag name string (which may be in upper or mixed case) to a tag
 * enum. The `tag` version expects `tagname` to be NULL-terminated
 */
GumboTag gumbo_tag_enum(const char* tagname);
GumboTag gumbo_tagn_enum(const char* tagname, unsigned int length);

/**
 * Attribute namespaces.
 * HTML includes special handling for XLink, XML, and XMLNS namespaces on
 * attributes.  Everything else goes in the generic "NONE" namespace.
 */
typedef enum {
  GUMBO_ATTR_NAMESPACE_NONE,
  GUMBO_ATTR_NAMESPACE_XLINK,
  GUMBO_ATTR_NAMESPACE_XML,
  GUMBO_ATTR_NAMESPACE_XMLNS,
} GumboAttributeNamespaceEnum;

/**
 * A struct representing a single attribute on an HTML tag.  This is a
 * name-value pair, but also includes information about source locations and
 * original source text.
 */
typedef struct {
  /**
   * The namespace for the attribute.  This will usually be
   * GUMBO_ATTR_NAMESPACE_NONE, but some XLink/XMLNS/XML attributes take special
   * values, per:
   * http://www.whatwg.org/specs/web-apps/current-work/multipage/tree-construction.html#adjust-foreign-attributes
   */
  GumboAttributeNamespaceEnum attr_namespace;

  /**
   * The name of the attribute.  This is in a freshly-allocated buffer to deal
   * with case-normalization, and is null-terminated.
   */
  const char* name;

  /**
   * The original text of the attribute name, as a pointer into the original
   * source buffer.
   */
  GumboStringPiece original_name;

  /**
   * The value of the attribute.  This is in a freshly-allocated buffer to deal
   * with unescaping, and is null-terminated.  It does not include any quotes
   * that surround the attribute.  If the attribute has no value (for example,
   * 'selected' on a checkbox), this will be an empty string.
   */
  const char* value;

  /**
   * The original text of the value of the attribute.  This points into the
   * original source buffer.  It includes any quotes that surround the
   * attribute, and you can look at original_value.data[0] and
   * original_value.data[original_value.length - 1] to determine what the quote
   * characters were.  If the attribute has no value, this will be a 0-length
   * string.
   */
  GumboStringPiece original_value;

  /** The starting position of the attribute name. */
  GumboSourcePosition name_start;

  /**
   * The ending position of the attribute name.  This is not always derivable
   * from the starting position of the value because of the possibility of
   * whitespace around the = sign.
   */
  GumboSourcePosition name_end;

  /** The starting position of the attribute value. */
  GumboSourcePosition value_start;

  /** The ending position of the attribute value. */
  GumboSourcePosition value_end;
} GumboAttribute;

/**
 * Given a vector of GumboAttributes, look up the one with the specified name
 * and return it, or NULL if no such attribute exists.  This uses a
 * case-insensitive match, as HTML is case-insensitive.
 */
GumboAttribute* gumbo_get_attribute(const GumboVector* attrs, const char* name);


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

#endif  // GUMBO_GUMBO_H_
