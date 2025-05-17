#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ctype.h>
#include <stddef.h>

#include <mcpc/mcpc.h>
#include <mcpc/_c23_keywords.h>
#include <mcpc/_c23_uchar.h>


#include <tree_sitter/api.h>

typedef enum lang
  { NONE, C, CPP, RUST, GO, RUBY, PYTHON, JAVA }
lang_t;

typedef uint8_t u8_t;

#define u8c1(a) u8##"" a
#define u8c2(a, b) u8##"" a b

inline static bool
u8streq (const char8_t *a, const char8_t *b)
{
  const char *const aa = (const char *const) a;
  const char *const bb = (const char *const) b;
  return strcmp (aa, bb) == 0;
}

inline static bool
beg_with_alph (const char8_t *u8s)
{
  const char *s = (const char *) u8s;
  return s[0] == '/' || isalpha ((int) s[0]);
}

typedef TSLanguage *(*ts_new_lang_t) ();

ts_new_lang_t
detect_lang (lang_t l)
{
  extern TSLanguage *tree_sitter_c (void);
  extern TSLanguage *tree_sitter_cpp (void);
  extern TSLanguage *tree_sitter_rust (void);
  extern TSLanguage *tree_sitter_go (void);
  extern TSLanguage *tree_sitter_ruby (void);
  extern TSLanguage *tree_sitter_python (void);
  extern TSLanguage *tree_sitter_java (void);

  ts_new_lang_t ret = nullptr;

  if (false);
  else if (l == C)
    ret = tree_sitter_c;
  else if (l == CPP)
    ret = tree_sitter_cpp;
  else if (l == RUST)
    ret = tree_sitter_rust;
  else if (l == GO)
    ret = tree_sitter_go;
  else if (l == RUBY)
    ret = tree_sitter_ruby;
  else if (l == PYTHON)
    ret = tree_sitter_python;
  else if (l == JAVA)
    ret = tree_sitter_java;

  return ret;
}


// Clean up a string for display (convert control chars and handle special cases)
void
clean_string_for_display (char *str, int len)
{
  for (int i = 0; i < len; i++)
    {
      // Replace newlines and tabs with spaces for single-line display
      if (str[i] == '\n' || str[i] == '\t' || str[i] == '\r')
	{
	  str[i] = ' ';
	}
      // Replace other control characters
      else if (str[i] < 32)
	{
	  str[i] = ' ';
	}
    }
}

// Check if node is named (has semantic meaning in the language)
bool
is_named_node (TSNode node)
{
  return ts_node_is_named (node);
}

// Function to print AST node in s-expression format
int
print_node_sexp (TSNode node, const char8_t *source_code, int depth,
		 char8_t *ret, size_t ret_cap, size_t *ret_len)
{
  if (ts_node_is_null (node))
    {
      return 0;
    }

  int printed = 0;

  // Get node text boundaries
  uint32_t start_byte = ts_node_start_byte (node);
  uint32_t end_byte = ts_node_end_byte (node);
  int text_len = end_byte - start_byte;

  // Get node type
  const char *node_type = ts_node_type (node);

  // Get child count
  uint32_t child_count = ts_node_named_child_count (node);
  uint32_t all_count = ts_node_child_count (node);

  // Special handling for error nodes
  if (strcmp (node_type, "ERROR") == 0)
    {
      printed =
	snprintf (ret + *ret_len, ret_cap - *ret_len,
		  "(ERROR \"Syntax error detected\")");
      if (printed >= ret_cap - *ret_len)
	return 23;
      *ret_len += printed;
      return 1;
    }

  // For nodes without named children, print as leaf nodes with their text
  if (child_count == 0)
    {
      // Only print text for reasonably sized tokens
      if (text_len > 0 && text_len < 100)
	{
	  // char *node_text = alloca (text_len + 1);
	  char node_text[text_len + 1];
	  if (node_text)
	    {
	      strncpy (node_text, source_code + start_byte, text_len);
	      node_text[text_len] = '\0';

	      // Clean up text for display
	      clean_string_for_display (node_text, text_len);

	      printed =
		snprintf (ret + *ret_len, ret_cap - *ret_len, " (%s \"%s\")",
			  node_type, node_text);
	      if (printed >= ret_cap - *ret_len)
		return 23;
	      *ret_len += printed;
	    }
	  else
	    {
	      printed =
		snprintf (ret + *ret_len, ret_cap - *ret_len, " (%s)",
			  node_type);
	      if (printed >= ret_cap - *ret_len)
		return 23;
	      *ret_len += printed;
	    }
	}
      else if (text_len == 0)
	{
	  // Empty node
	  printed =
	    snprintf (ret + *ret_len, ret_cap - *ret_len, " (%s \"\")",
		      node_type);
	  if (printed >= ret_cap - *ret_len)
	    return 23;
	  *ret_len += printed;
	}
      else
	{
	  // Node text too long, show truncated
	  printed =
	    snprintf (ret + *ret_len, ret_cap - *ret_len,
		      " (%s \"<too long>\")", node_type);
	  if (printed >= ret_cap - *ret_len)
	    return 23;
	  *ret_len += printed;
	}
    }
  else
    {
      // For non-leaf nodes, print the opening tag
      printed =
	snprintf (ret + *ret_len, ret_cap - *ret_len, " (%s", node_type);
      if (printed >= ret_cap - *ret_len)
	return 23;
      *ret_len += printed;

      // Process all named children (more semantically meaningful)
      for (uint32_t i = 0; i < child_count; i++)
	{
	  TSNode child = ts_node_named_child (node, i);
	  if (!ts_node_is_null (child))
	    {
	      int res =
		print_node_sexp (child, source_code, depth + 1, ret, ret_cap,
				 ret_len);
	      if (res != 0)
		return res;
	    }
	}

      // Print the closing tag
      printed = snprintf (ret + *ret_len, ret_cap - *ret_len, ")");
      if (printed >= ret_cap - *ret_len)
	return 23;
      *ret_len += printed;
    }

  return 0;
}


int
doit (lang_t lang_typ, char8_t *code, size_t code_len, char8_t *astse,
      size_t astse_cap, size_t *astse_len)
{

  // Initialize the parser
  TSParser *parser = ts_parser_new ();
  if (!parser)
    {
      goto badret;
    }



  ts_new_lang_t tgt_lang_new = nullptr;

  tgt_lang_new = detect_lang (lang_typ);

  // Set the language to C
  TSLanguage *tgt_lang = tgt_lang_new ();
  if (!tgt_lang)
    {
      goto badret;
    }

  // Set parser properties
  ts_parser_set_language (parser, tgt_lang);

  // Parse the source code
  TSTree *tree = ts_parser_parse_string (parser, NULL, code, code_len);
  if (!tree)
    {
      goto badret;
    }

  // Get the root node
  TSNode root_node = ts_tree_root_node (tree);

  // Check if the root node is valid
  if (ts_node_is_null (root_node))
    {
      goto badret;
    }

  // Print the AST in s-expression format
  int res = print_node_sexp (root_node, code, 0, astse, astse_cap, astse_len);
  if (res != 0)
    return res;

  // Clean up
  ts_tree_delete (tree);
  ts_parser_delete (parser);

  return 0;

badret:
  ts_tree_delete (tree);
  ts_parser_delete (parser);

  return 2;
}


static void
cb_calc (const mcpc_tool_t *tool, mcpc_ucbr_t *ucbr)
{
  size_t code_cap = 1024;
  char8_t lang[8];
  char8_t *code = nullptr;
  size_t code_len = 0;
  char8_t *astse = nullptr;
  size_t astse_cap = 1024;
  size_t astse_len = 0;
  mcpc_errcode_t ec = MCPC_EC_0;

  assert (0 ==
	  mcpc_tool_get_tpropval_u8str (tool, u8c1 ("lang"), lang, 16,
					nullptr));

  code = malloc (code_cap);

  ec =
    mcpc_tool_get_tpropval_u8str (tool, u8c1 ("code"), code, code_cap,
				  &code_len);
  while (ec == MCPC_EC_BUFCAP)
    {
      code_cap += code_cap;
      code = realloc (code, code_cap);
      code_len = 0;
      ec =
	mcpc_tool_get_tpropval_u8str (tool, u8c1 ("code"), code, code_cap,
				      &code_len);
    }

  lang_t src_lang = NONE;

  if (false);
  else if (u8streq (lang, "cpp"))
    src_lang = CPP;
  else if (u8streq (lang, "Cpp"))
    src_lang = CPP;
  else if (u8streq (lang, "CPP"))
    src_lang = CPP;
  else if (u8streq (lang, "cxx"))
    src_lang = CPP;
  else if (u8streq (lang, "CXX"))
    src_lang = CPP;
  else if (u8streq (lang, "c++"))
    src_lang = CPP;
  else if (u8streq (lang, "C++"))
    src_lang = CPP;
  else if (u8streq (lang, "cc"))
    src_lang = CPP;
  else if (u8streq (lang, "c"))
    src_lang = C;
  else if (u8streq (lang, "C"))
    src_lang = C;
  else if (u8streq (lang, "rs"))
    src_lang = RUST;
  else if (u8streq (lang, "rust"))
    src_lang = RUST;
  else if (u8streq (lang, "Rust"))
    src_lang = RUST;
  else if (u8streq (lang, "py"))
    src_lang = PYTHON;
  else if (u8streq (lang, "python"))
    src_lang = PYTHON;
  else if (u8streq (lang, "Python"))
    src_lang = PYTHON;
  else if (u8streq (lang, "PYTHON"))
    src_lang = PYTHON;
  else if (u8streq (lang, "go"))
    src_lang = GO;
  else if (u8streq (lang, "GO"))
    src_lang = GO;
  else if (u8streq (lang, "Go"))
    src_lang = GO;
  else if (u8streq (lang, "golang"))
    src_lang = GO;
  else if (u8streq (lang, "rb"))
    src_lang = RUBY;
  else if (u8streq (lang, "ruby"))
    src_lang = RUBY;
  else if (u8streq (lang, "Ruby"))
    src_lang = RUBY;
  else if (u8streq (lang, "java"))
    src_lang = JAVA;
  else if (u8streq (lang, "Java"))
    src_lang = JAVA;
  else if (u8streq (lang, "JAVA"))
    src_lang = JAVA;
  else
    {
      mcpc_ucbr_toolcall_add_errmsg_printf8	//
	(ucbr, u8c1 ("The language %s is not supported "), lang);
      return;
    }

  astse = malloc (astse_cap);
  astse_len = 0;
  int ourec = 0;
  ourec = doit (src_lang, code, code_len, astse, astse_cap, &astse_len);
  while (ourec == 23)
    {
      astse_cap += astse_cap;
      astse = realloc (astse, astse_cap);
      astse_len = 0;
      ourec = doit (src_lang, code, code_len, astse, astse_cap, &astse_len);
    }

  mcpc_ucbr_toolcall_add_text_printf8 (ucbr,
				       u8c1
				       ("Abstract Syntax Tree(AST) for the source code as a S-expression :    %.*s"),
				       astse_len, astse);

  free (code);
  free (astse);
}


mcpc_tool_t *
new_tool_calc ()
{
  const char8_t *name = u8c1 ("code-to-tree");
  const char8_t *desc =
    u8c1
    ("This tool can take source code as input, and print out the corresponding Abstract Syntax Tree(AST) representation in S-Expression style, it supports various programming language: C, C++, Rust, Go, Ruby, Python, and so on.");
  mcpc_tool_t *tool = mcpc_tool_new2 (name, desc);

  {
    const char8_t *name = u8c1 ("lang");
    const mcpc_anytype_t type = MCPC_U8STR;
    const char8_t *desc = u8c1 ("The language that input source code uses");
    mcpc_toolprop_t *prop = mcpc_toolprop_new2 (name, desc, type);
    mcpc_tool_add_toolprop (tool, prop);
  }
  {
    const char8_t *name = u8c1 ("code");
    const mcpc_anytype_t type = MCPC_U8STR;
    const char8_t *desc = u8c1 ("The source code");
    mcpc_toolprop_t *prop = mcpc_toolprop_new2 (name, desc, type);
    mcpc_tool_add_toolprop (tool, prop);
  }

  mcpc_tool_set_call_cb (tool, &cb_calc);

  return tool;
}


int
Xmain (int argc, const char *argv[])
{
  u8_t in_src = 0;		// 1stdin 2file
  char in_fpath[100];

  for (int i = 1; i < argc; i++)
    {
      const char8_t *curr_arg = (const char8_t *) argv[i];
      const char8_t *nex_arg = nullptr;
      if (i < argc - 1)
	nex_arg = (const char8_t *) argv[i + 1];
      if (nex_arg != nullptr)
	{
	  if (u8streq (curr_arg, u8c1 ("-i")))
	    {
	      if (u8streq (nex_arg, u8c1 ("-")))
		{
		  in_src = 1;
		}
	      else if (beg_with_alph ((const char8_t *) nex_arg))
		{
		  // TODO nonascii filename?
		  /* memcpy (in_fpath, nex_arg, strlen (nex_arg)); */
		  strcpy (in_fpath, (const char *) nex_arg);
		  in_src = 2;
		}
	    }
	}
    }

  assert (in_src);

  FILE *in_stream = nullptr;
  switch (in_src)
    {
    case 1:
      in_stream = stdin;
      break;
    case 2:
      in_stream = fopen (in_fpath, "r");
      if (!in_stream)
	{
	  fprintf (stdout, "file:%s, err:%s", in_fpath, strerror (errno));
	}
      break;
    }
  assert (in_stream);

  mcpc_server_t *server = mcpc_server_new_iostrm (in_stream, stdout);
  mcpc_server_capa_enable_prmt (server);
  mcpc_server_capa_enable_tool (server);
  mcpc_server_capa_enable_rsc (server);
  mcpc_server_capa_enable_complt (server);

  mcpc_tool_t *tool = new_tool_calc ();
  assert (0 == mcpc_server_add_tool (server, tool));

  mcpc_errcode_t ec = mcpc_server_start (server);

  mcpc_server_close (server);
  fclose (in_stream);

  return ec;
}

int
main (int argc, const char *argv[])
{
  mcpc_server_t *server = mcpc_server_new_iostrm (stdin, stdout);
  mcpc_server_capa_enable_prmt (server);
  mcpc_server_capa_enable_tool (server);
  mcpc_server_capa_enable_rsc (server);
  mcpc_server_capa_enable_complt (server);

  mcpc_tool_t *tool = new_tool_calc ();
  assert (0 == mcpc_server_add_tool (server, tool));

  mcpc_errcode_t ec = mcpc_server_start (server);

  mcpc_server_close (server);
  
  return ec;
}
