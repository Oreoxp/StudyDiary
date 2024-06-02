#pragma once
#include "Node/NodeWrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Input struct containing configuration options for the parser.
 * These let you specify alternate memory managers, provide different error
 * handling, etc.
 * Use kGumboDefaultOptions for sensible defaults, and only set what you need.
 */

typedef struct GumboInternalOptions {
  /** A memory allocator function.  Default: malloc. */
  GumboAllocatorFunction allocator;

  /** A memory deallocator function. Default: free. */
  GumboDeallocatorFunction deallocator;

  /**
   * An opaque object that's passed in as the first argument to all callbacks
   * used by this library.  Default: NULL.
   */
  void* userdata;

  /**
   * The tab-stop size, for computing positions in source code that uses tabs.
   * Default: 8.
   */
  int tab_stop;

  /**
   * Whether or not to stop parsing when the first error is encountered.
   * Default: false.
   */
  bool stop_on_first_error;

  /**
   * The maximum number of errors before the parser stops recording them.  This
   * is provided so that if the page is totally borked, we don't completely fill
   * up the errors vector and exhaust memory with useless redundant errors.  Set
   * to -1 to disable the limit.
   * Default: -1
   */
  int max_errors;

  /**
   * The fragment context for parsing:
   * https://html.spec.whatwg.org/multipage/syntax.html#parsing-html-fragments
   *
   * If GUMBO_TAG_LAST is passed here, it is assumed to be "no fragment", i.e.
   * the regular parsing algorithm.  Otherwise, pass the tag enum for the
   * intended parent of the parsed fragment.  We use just the tag enum rather
   * than a full node because that's enough to set all the parsing context we
   * need, and it provides some additional flexibility for client code to act as
   * if parsing a fragment even when a full HTML tree isn't available.
   *
   * Default: GUMBO_TAG_LAST
   */
  GumboTag fragment_context;

  /**
   * The namespace for the fragment context.  This lets client code
   * differentiate between, say, parsing a <title> tag in SVG vs. parsing it in
   * HTML.
   * Default: GUMBO_NAMESPACE_HTML
   */
  GumboNamespaceEnum fragment_namespace;
} GumboOptions;

/** Default options struct; use this with gumbo_parse_with_options. */
extern const GumboOptions kGumboDefaultOptions;

/** The output struct containing the results of the parse. */
typedef struct GumboInternalOutput {
  /**
   * Pointer to the document node.  This is a GumboNode of type NODE_DOCUMENT
   * that contains the entire document as its child.
   */
  GumboNode* document;

  /**
   * Pointer to the root node.  This the <html> tag that forms the root of the
   * document.
   */
  GumboNode* root;

  /**
   * A list of errors that occurred during the parse.
   * NOTE: In version 1.0 of this library, the API for errors hasn't been fully
   * fleshed out and may change in the future.  For this reason, the GumboError
   * header isn't part of the public API.  Contact us if you need errors
   * reported so we can work out something appropriate for your use-case.
   */
  GumboVector /* GumboError */ errors;
} GumboOutput;


/**
 * Parses a buffer of UTF8 text into an GumboNode parse tree.  The buffer must
 * live at least as long as the parse tree, as some fields (eg. original_text)
 * point directly into the original buffer.
 *
 * This doesn't support buffers longer than 4 gigabytes.
 */
GumboOutput* gumbo_parse(const char* buffer);

/**
 * Extended version of gumbo_parse that takes an explicit options structure,
 * buffer, and length.
 */
GumboOutput* gumbo_parse_with_options(
    const GumboOptions* options, const char* buffer, size_t buffer_length);

/** Release the memory used for the parse tree & parse errors. */
void gumbo_destroy_output(const GumboOptions* options, GumboOutput* output);


#ifdef __cplusplus
}
#endif