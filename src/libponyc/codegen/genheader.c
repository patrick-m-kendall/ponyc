#include "genheader.h"
#include "genname.h"
#include "../type/assemble.h"
#include "../type/lookup.h"
#include "../type/subtype.h"
#include "../type/reify.h"
#include "../ast/printbuf.h"
#include "../../libponyrt/mem/pool.h"
#include "ponyassert.h"
#include <string.h>

static const char* c_type_name(compile_t* c, const char* name)
{
  if(name == c->str_Bool)
    return "bool";
  else if(name == c->str_I8)
    return "int8_t";
  else if(name == c->str_I16)
    return "int16_t";
  else if(name == c->str_I32)
    return "int32_t";
  else if(name == c->str_I64)
    return "int64_t";
  else if(name == c->str_I128)
    return "__int128_t";
  else if(name == c->str_ILong)
    return "long";
  else if(name == c->str_ISize)
    return "ssize_t";
  else if(name == c->str_U8)
    return "char";
  else if(name == c->str_U16)
    return "uint16_t";
  else if(name == c->str_U32)
    return "uint32_t";
  else if(name == c->str_U64)
    return "uint64_t";
  else if(name == c->str_U128)
    return "__uint128_t";
  else if(name == c->str_ULong)
    return "unsigned long";
  else if(name == c->str_USize)
    return "size_t";
  else if(name == c->str_F32)
    return "float";
  else if(name == c->str_F64)
    return "double";

  return NULL;
}

static void print_base_type(compile_t* c, printbuf_t* buf, ast_t* type)
{
  if(ast_id(type) == TK_NOMINAL)
  {
    const char* name = genname_type(type);
    const char* c_name = c_type_name(c, name);

    if(c_name != NULL)
      printbuf(buf, c_name);
    else
      printbuf(buf, "%s*", name);
  } else {
    printbuf(buf, "void*");
  }
}

static int print_pointer_type(compile_t* c, printbuf_t* buf, ast_t* type)
{
  ast_t* typeargs = ast_childidx(type, 2);
  ast_t* elem = ast_child(typeargs);

  if(is_pointer(elem) || is_nullable_pointer(elem))
    return print_pointer_type(c, buf, elem) + 1;

  print_base_type(c, buf, elem);
  return 1;
}

static void print_type_name(compile_t* c, printbuf_t* buf, ast_t* type)
{
  if(is_pointer(type) || is_nullable_pointer(type))
  {
    int depth = print_pointer_type(c, buf, type);

    for(int i = 0; i < depth; i++)
      printbuf(buf, "*");
  } else {
    print_base_type(c, buf, type);
  }
}

static void print_params(compile_t* c, printbuf_t* buf, reach_param_t* params,
  size_t param_count, bool initial_comma)
{
  for(size_t i = 0; i < param_count; ++i)
  {
    // Print the parameter.
    if(initial_comma)
      printbuf(buf, ", ");
    else
      initial_comma = true;

    print_type_name(c, buf, params[i].type->ast_cap);

    // Smash trailing primes to underscores.
    const char* name = params[i].name;
    size_t len = strlen(name) + 1;
    size_t buf_size = len;
    char* buffer = (char*)ponyint_pool_alloc_size(buf_size);
    memcpy(buffer, name, len);

    len--;

    while(buffer[--len] == '\'')
      buffer[len] = '_';

    printbuf(buf, " %s", buffer);

    ponyint_pool_free_size(buf_size, buffer);
  }
}

static bool should_emit_fun(reach_method_t* m)
{
  // No signature for any internal function (i.e. functions without an AST), or
  // any function with a tuple argument or return value, or any function that
  // might raise an error.
  if(m->internal)
    return false;

  ast_t* can_error = ast_childidx(m->fun->ast, 5);

  if(ast_id(can_error) == TK_QUESTION)
    return false;

  if(m->result->underlying == TK_TUPLETYPE)
    return false;

  for(size_t i = 0; i < m->param_count; ++i)
  {
    if(m->params[i].type->underlying == TK_TUPLETYPE)
      return false;
  }

  return true;
}

static void print_method(compile_t* c, printbuf_t* buf, reach_type_t* t,
  reach_method_t* m)
{
  if(!should_emit_fun(m))
    return;

  ast_t* docstring = ast_childidx(m->fun->ast, 7);

  // Print the docstring if we have one.
  if(ast_id(docstring) == TK_STRING)
  {
    printbuf(buf,
      "/*\n"
      "%s"
      "*/\n",
      ast_name(docstring)
      );
  }

  ast_t* rtype = m->result->ast_cap;

  // Print the function signature.
  if((m->cap != TK_AT) || !is_none(rtype))
    print_type_name(c, buf, rtype);
  else
    printbuf(buf, "void");

  printbuf(buf, " %s", m->full_name);

  switch(ast_id(m->fun->ast))
  {
    case TK_NEW:
    case TK_BE:
    {
      if(t->underlying == TK_ACTOR)
        printbuf(buf, "__send");

      break;
    }

    default: {}
  }

  printbuf(buf, "(");
  if(m->cap != TK_AT)
  {
    print_type_name(c, buf, t->ast);
    printbuf(buf, " self");
    print_params(c, buf, m->params, m->param_count, true);
  } else {
    print_params(c, buf, m->params, m->param_count, false);
  }

  printbuf(buf, ");\n\n");
}

static void print_methods(compile_t* c, reach_type_t* t, printbuf_t* buf)
{
  size_t i = HASHMAP_BEGIN;
  reach_method_name_t* n;

  while((n = reach_method_names_next(&t->methods, &i)) != NULL)
  {
    size_t j = HASHMAP_BEGIN;
    reach_method_t* m;

    while((m = reach_mangled_next(&n->r_mangled, &j)) != NULL)
      print_method(c, buf, t, m);
  }
}

static void print_types(compile_t* c, FILE* fp, printbuf_t* buf)
{
  size_t i = HASHMAP_BEGIN;
  reach_type_t* t;

  while((t = reach_types_next(&c->reach->types, &i)) != NULL)
  {
    // Print the docstring if we have one.
    ast_t* def = (ast_t*)ast_data(t->ast);

    if (def != NULL)
    {
      ast_t* docstring = ast_childidx(def, 6);

      if(ast_id(docstring) == TK_STRING)
        fprintf(fp, "/*\n%s*/\n", ast_name(docstring));
    }

    if(!is_pointer(t->ast) && !is_nullable_pointer(t->ast) && !is_machine_word(t->ast))
    {
      // Forward declare an opaque type.
      fprintf(fp, "typedef struct %s %s;\n\n", t->name, t->name);

      // Function signature for the allocator.
      printbuf(buf,
        "/* Allocate a %s without initialising it. */\n%s* %s_Alloc();\n\n",
        t->name,
        t->name,
        t->name
        );
    }

    print_methods(c, t, buf);
  }
}

bool genheader(compile_t* c)
{
  // Open a header file.
  const char* file_h =
    suffix_filename(c, c->opt->output, "", c->filename, ".h");
  FILE* fp = fopen(file_h, "wt");

  if(fp == NULL)
  {
    errorf(c->opt->check.errors, NULL, "couldn't write to %s", file_h);
    return false;
  }

  fprintf(fp,
    "#ifndef pony_%s_h\n"
    "#define pony_%s_h\n"
    "\n"
    "/* This is an auto-generated header file. Do not edit. */\n"
    "\n"
    "#include <stdint.h>\n"
    "#include <stdbool.h>\n"
    "\n"
    "#ifdef __cplusplus\n"
    "extern \"C\" {\n"
    "#endif\n"
    "\n"
    "#ifdef _MSC_VER\n"
    "typedef struct __int128_t { uint64_t low; int64_t high; } __int128_t;\n"
    "typedef struct __uint128_t { uint64_t low; uint64_t high; } "
      "__uint128_t;\n"
    "#endif\n"
    "\n",
    c->filename,
    c->filename
    );

  if(c->primitives_init != NULL)
  {
    fprintf(fp,
      "void %s_primitives_init();\n"
      "\n",
      c->filename
      );
  }

  if(c->primitives_final != NULL)
  {
    fprintf(fp,
      "void %s_primitives_final();\n"
      "\n",
      c->filename
      );
  }

  printbuf_t* buf = printbuf_new();
  print_types(c, fp, buf);
  fwrite(buf->m, 1, buf->offset, fp);
  printbuf_free(buf);

  fprintf(fp,
    "\n"
    "#ifdef __cplusplus\n"
    "}\n"
    "#endif\n"
    "\n"
    "#endif\n"
    );

  fclose(fp);
  return true;
}
