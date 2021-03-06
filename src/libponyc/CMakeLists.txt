cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

project(libponyc VERSION ${PONYC_PROJECT_VERSION} LANGUAGES C CXX)

add_library(libponyc STATIC
    ponyc.c
    ast/ast.c
    ast/bnfprint.c
    ast/error.c
    ast/frame.c
    ast/id.c
    ast/lexer.c
    ast/lexint.c
    ast/parser.c
    ast/parserapi.c
    ast/printbuf.c
    ast/source.c
    ast/stringtab.c
    ast/symtab.c
    ast/token.c
    ast/treecheck.c
    codegen/codegen.c
    codegen/genbox.c
    codegen/gencall.c
    codegen/gencontrol.c
    codegen/gendesc.c
    codegen/genexe.c
    codegen/genexpr.c
    codegen/genfun.c
    codegen/genheader.c
    codegen/genident.c
    codegen/genlib.c
    codegen/genmatch.c
    codegen/genname.c
    codegen/genobj.c
    codegen/genoperator.c
    codegen/genprim.c
    codegen/genreference.c
    codegen/genserialise.c
    codegen/gentrace.c
    codegen/gentype.c
    codegen/gendebug.cc
    codegen/genjit.cc
    codegen/genopt.cc
    codegen/host.cc
    expr/array.c
    expr/call.c
    expr/control.c
    expr/ffi.c
    expr/lambda.c
    expr/literal.c
    expr/match.c
    expr/operator.c
    expr/postfix.c
    expr/reference.c
    options/options.c
    pass/docgen.c
    pass/expr.c
    pass/finalisers.c
    pass/flatten.c
    pass/import.c
    pass/names.c
    pass/pass.c
    pass/refer.c
    pass/scope.c
    pass/serialisers.c
    pass/sugar.c
    pass/syntax.c
    pass/traits.c
    pass/verify.c
    pkg/buildflagset.c
    pkg/ifdef.c
    pkg/package.c
    pkg/platformfuns.c
    pkg/program.c
    pkg/use.c
    platform/paths.c
    platform/vcvars.c
    plugin/plugin.c
    reach/paint.c
    reach/reach.c
    reach/subtype.c
    type/alias.c
    type/assemble.c
    type/cap.c
    type/compattype.c
    type/lookup.c
    type/matchtype.c
    type/reify.c
    type/safeto.c
    type/sanitise.c
    type/subtype.c
    type/typeparam.c
    type/viewpoint.c
    verify/call.c
    verify/control.c
    verify/fun.c
    verify/type.c
)

target_compile_definitions(libponyc PRIVATE
    PONY_ALWAYS_ASSERT
)

target_include_directories(libponyc
    PUBLIC ../common
    PRIVATE ../../build/libs/include
    PRIVATE ../../lib/blake2
)

if (MSVC)
    file(GLOB_RECURSE CFILES "${PROJECT_SOURCE_DIR}/*.c")
    set_source_files_properties(${CFILES} PROPERTIES LANGUAGE CXX)
endif (MSVC)
