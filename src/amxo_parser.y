/****************************************************************************
**
** Copyright (c) 2020 SoftAtHome
**
** Redistribution and use in source and binary forms, with or
** without modification, are permitted provided that the following
** conditions are met:
**
** 1. Redistributions of source code must retain the above copyright
** notice, this list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above
** copyright notice, this list of conditions and the following
** disclaimer in the documentation and/or other materials provided
** with the distribution.
**
** Subject to the terms and conditions of this license, each
** copyright holder and contributor hereby grants to those receiving
** rights under this license a perpetual, worldwide, non-exclusive,
** no-charge, royalty-free, irrevocable (except for failure to
** satisfy the conditions of this license) patent license to make,
** have made, use, offer to sell, sell, import, and otherwise
** transfer this software, where such license applies only to those
** patent claims, already acquired or hereafter acquired, licensable
** by such copyright holder or contributor that are necessarily
** infringed by:
**
** (a) their Contribution(s) (the licensed copyrights of copyright
** holders and non-copyrightable additions of contributors, in
** source or binary form) alone; or
**
** (b) combination of their Contribution(s) with the work of
** authorship to which such Contribution(s) was added by such
** copyright holder or contributor, if, at the time the Contribution
** is added, such addition causes such combination to be necessarily
** infringed. The patent license shall not apply to any other
** combinations which include the Contribution.
**
** Except as expressly stated above, no rights or licenses from any
** copyright holder or contributor is granted under this license,
** whether expressly, by implication, estoppel or otherwise.
**
** DISCLAIMER
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
** CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
** INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
** MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
** DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
** USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
** AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
** LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
** ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
**
****************************************************************************/
%define api.pure full
%locations
%defines
%define parse.error verbose
%parse-param {void *ctx}
%lex-param {void *ctx}

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_common.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_function.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_parser.tab.h"

%}


%union
{
	int64_t integer;
  int64_t bitmap;
	amxo_txt_t cptr;
  bool boolean;
  amxc_var_t value;
}

%token <integer> INCLUDE
%token <integer> IMPORT
%token <integer> AS
%token <integer> INSTANCE
%token <integer> PARAMETER
%token <integer> DEFINE
%token <integer> CONFIG
%token <integer> POPULATE
%token <integer> OBJECT
%token <integer> EXTEND
%token <integer> EOF_TOKEN
%token <integer> DIGIT
%token <integer> TYPE
%token <integer> LDFLAG
%token <integer> ENTRY
%token <integer> COUNTER
%token <integer> WITH
%token <integer> ON
%token <integer> DEFAULT
%token <integer> EVENT
%token <integer> FILTER
%token <integer> OF
%token <integer> CALL
%token <integer> ACTION
%token <integer> ACTION_KW
%token <integer> DEP_ACTION
%token <cptr>    STRING
%token <cptr>    MULTILINECOMMENT
%token <cptr>    TEXT
%token <cptr>    RESOLVER
%token <boolean> BOOL
%token <bitmap>  SET_ATTRIBUTE
%token <bitmap>  UNSET_ATTRIBUTE

%token <integer> CONSTRAINT
%token <integer> MIN
%token <integer> MAX
%token <integer> RANGE
%token <integer> ENUM
%token <integer> CUSTOM

%type <integer> stream sections section config_options config_option
%type <integer> include import ldflags entry_point
%type <integer> defines populates populate object_populate event_populate
%type <integer> define object_def object_def_header multi object_body object_content
%type <integer> parameter_def counted
%type <integer> function_def arguments argument_def add_mib
%type <integer> object_pop_header object_pop_body object_pop_content parameter
%type <integer> action_header action deprecated_action dep_action
%type <bitmap>  attributes unset_attributes
%type <value>   value
%type <cptr>    name path filter

%{
    int yylex(YYSTYPE* lvalp, YYLTYPE* llocp, void * yyscanner);
    void yyerror(YYLTYPE* locp, void* scanner, const char* err);
    void yywarning(YYLTYPE* locp, void* scanner, const char* err);
    amxo_parser_t *yyget_extra ( void * yyscanner );

    void yyerror(YYLTYPE* locp, void* scanner, const char* err) {
        amxo_parser_t *context = (amxo_parser_t *)yyget_extra(scanner);
        if (context->status == amxd_status_ok) {
            if (!amxc_string_is_empty(&context->msg)) {
                amxo_parser_printf("ERROR %s@%s:line %d\n", 
                                   amxc_string_get(&context->msg, 0),
                                   context->file,
                                   locp->first_line);
                amxc_string_clean(&context->msg);
            } else {
                amxo_parser_printf("ERROR %s@%s:line %d\n", 
                                   err,
                                   context->file,
                                   locp->first_line);
            }
        } else {
            if (!amxc_string_is_empty(&context->msg)) {
                amxo_parser_printf("ERROR %d - %s@%s:line %d\n",
                                   context->status,
                                   amxc_string_get(&context->msg, 0),
                                   context->file,
                                   locp->first_line);
                amxc_string_clean(&context->msg);
            } else {
                amxo_parser_printf("ERROR %d - %s - %s@%s:line %d\n",
                                   context->status,
                                   amxd_status_string(context->status),
                                   err,
                                   context->file,
                                   locp->first_line);
            }
        }
    }

    void yywarning(YYLTYPE* locp, void* scanner, const char* err) {
        amxo_parser_t *context = (amxo_parser_t *)yyget_extra(scanner);
        if (!amxc_string_is_empty(&context->msg)) {
            amxo_parser_printf("WARNING %s@%s:line %d\n",
                               amxc_string_get(&context->msg, 0),
                               context->file,
                               locp->first_line);
            amxc_string_clean(&context->msg);
        } else {
            amxo_parser_printf("WARNING %s@%s:line %d\n",
                               err,
                               context->file,
                               locp->first_line);
        }
    }

    #define scanner x->scanner
    #define parser_ctx ((amxo_parser_t *)yyget_extra(ctx))
    #define YY_ASSERT_ACTION(c,m,a) if(c) { yyerror(&yylloc, ctx, m); a; YYERROR; }
    #define YY_ASSERT(c,m) if(c) { yyerror(&yylloc, ctx, m); YYERROR; }
    #define YY_WARNING(c,m) if(c) { yywarning(&yylloc, ctx, m); }
    #define NOOP

    const uint64_t amxo_object_attrs = ((1 << attr_readonly) | 
                                    (1 << attr_persistent) | 
                                    (1 << attr_private));

    const uint64_t amxo_param_attrs = ((1 << attr_persistent) | 
                                    (1 << attr_private) | 
                                    (1 << attr_template) |
                                    (1 << attr_instance) |
                                    (1 << attr_variable) |
                                    (1 << attr_readonly) |
                                    (1 << attr_key));

    const uint64_t amxo_func_attrs = ((1 << attr_private) | 
                                    (1 << attr_template) | 
                                    (1 << attr_instance));

    const uint64_t amxo_arg_attrs = ((1 << attr_in) | 
                                    (1 << attr_out) | 
                                    (1 << attr_mandatory) |
                                    (1 << attr_strict));

%}

%start stream

%%

stream: /* empty */ { NOOP; }
  | sections 
  | EOF_TOKEN
  ;

sections
  : section sections
  | section
  ;

section
  : include
  | import
  | CONFIG '{' '}' {
      amxo_hooks_end_section(parser_ctx, 0);
    }
  | CONFIG '{' config_options '}' {
      amxo_hooks_end_section(parser_ctx, 0);
    }
  | DEFINE '{' '}' {
      amxo_hooks_end_section(parser_ctx, 1);
    }
  | DEFINE '{' defines '}' {
      amxo_hooks_end_section(parser_ctx, 1);
    }
  | POPULATE '{' '}' {
      amxo_hooks_end_section(parser_ctx, 2);
    }
  | POPULATE '{' populates '}' {
      amxo_hooks_end_section(parser_ctx, 2);
    }
  | define
  ;
 
config_options
  : config_option config_options
  | config_option
  ;

config_option
  : STRING '=' data ';' {
      $1.txt[$1.length] = 0;
      amxo_hooks_set_config(parser_ctx, $1.txt, parser_ctx->data);
      YY_ASSERT_ACTION(amxo_parser_set_config(parser_ctx, $1.txt, parser_ctx->data) != 0,
                       $1.txt,
                       amxc_var_delete(&parser_ctx->data);
                      );
      amxc_var_delete(&parser_ctx->data);
      $$ = 0;
    }
  ;

include
  : INCLUDE TEXT ';' {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_include(parser_ctx, $2.txt);
      YY_ASSERT(retval != 0 && !(retval == 2 && $1 == token_optional_include),
                $2.txt); 
    }
  ;

import
  : IMPORT TEXT ';' {
      $2.txt[$2.length] = 0;
      YY_ASSERT(amxo_resolver_import_open(parser_ctx, $2.txt, $2.txt, RTLD_LAZY) != 0,
                $2.txt);
    }
  | IMPORT TEXT AS STRING ';' {
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      YY_ASSERT(amxo_resolver_import_open(parser_ctx, $2.txt, $4.txt, RTLD_LAZY) != 0,
                $2.txt);
    }
  | IMPORT TEXT ldflags ';' {
      $2.txt[$2.length] = 0;
      YY_ASSERT(amxo_resolver_import_open(parser_ctx, $2.txt, $2.txt, $3) != 0,
                $2.txt);
    }
  | IMPORT TEXT ldflags AS STRING ';' {
      $2.txt[$2.length] = 0;
      $5.txt[$5.length] = 0;
      YY_ASSERT(amxo_resolver_import_open(parser_ctx, $2.txt, $5.txt, $3) != 0,
                $2.txt);
    }
  ;
   
ldflags
  : LDFLAG ldflags {
      $$ = $1 | $2;
    }
  | LDFLAG

defines
  : define defines
  | define
  ;

populates
  : populate populates
  | populate
  ;

define
  : object_def
  | entry_point
  ; 

object_def
  : object_def_header ';' {
      int type = amxd_object_get_type(parser_ctx->object);
      YY_WARNING(type == amxd_object_mib, "Empty MIB object defined");
      amxo_parser_pop_object(parser_ctx);
    }
  | object_def_header '{' '}' {
      int type = amxd_object_get_type(parser_ctx->object);
      YY_WARNING(type == amxd_object_mib, "Empty MIB object defined");
      amxo_parser_pop_object(parser_ctx);
    }
  | object_def_header '{' object_body '}' {
      amxo_parser_pop_object(parser_ctx);
    }
  ;

entry_point
  : ENTRY name '.' name ';' {
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      YY_ASSERT(amxo_parser_call_entry_point(parser_ctx, $2.txt, $4.txt) != 0,
                $2.txt);

    }
  ;

object_def_header
  : OBJECT name {
      amxd_object_type_t type = ($1 == token_mib)?amxd_object_mib:amxd_object_singleton;
      $2.txt[$2.length] = 0;
      YY_ASSERT(!amxo_parser_create_object(parser_ctx, $2.txt, 0, type), $2.txt);
    }
  | attributes OBJECT name {
      amxd_object_type_t type = ($1 == token_mib)?amxd_object_mib:amxd_object_singleton;
      $3.txt[$3.length] = 0;
      YY_WARNING(type == amxd_object_mib, "Attributes declared on mib object are ignored");
      if (type != amxd_object_mib) {
        YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      }
      YY_ASSERT(!amxo_parser_create_object(parser_ctx, $3.txt, $1, amxd_object_singleton), $3.txt);
    }
  | OBJECT name multi {
      $2.txt[$2.length] = 0;
      YY_ASSERT($1 == token_mib, "Mib objects can not be multi-instance");
      YY_ASSERT(!amxo_parser_create_object(parser_ctx, $2.txt, 0, amxd_object_template), $2.txt);
    }
  | attributes OBJECT name multi {
      $3.txt[$3.length] = 0;
      YY_ASSERT($2 == token_mib, "Mib objects can not be multi-instance");
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_ASSERT(!amxo_parser_create_object(parser_ctx, $3.txt, $1, amxd_object_template), $3.txt);
    }
  ;

multi
  : '[' DIGIT ']' { 
      $$ = $2;
    }
  | '[' ']' {
      $$ = INT64_MAX;
    }
  ;

object_body
  : object_body object_content
  | object_content
  ;

object_content
  : object_def
  | parameter_def
  | function_def
  | counted
  | action
  | dep_action
  | add_mib
  ;

parameter_def
  : param_header ';' { 
      YY_ASSERT(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  | param_header '{' '}' {
      YY_ASSERT(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  | param_header '{' param_body '}' {
      YY_ASSERT(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  ; 

param_header
  : TYPE name {
      $2.txt[$2.length] = 0;
      YY_ASSERT(!amxo_parser_push_param(parser_ctx, $2.txt, 0, $1), $2.txt);
    }
  | attributes TYPE name {
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs), $3.txt);
      YY_ASSERT(!amxo_parser_push_param(parser_ctx, $3.txt, $1, $2), $3.txt);
    }
  | TYPE name '=' value {
      $2.txt[$2.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_push_param(parser_ctx, $2.txt, 0, $1),
                       $2.txt,
                       amxc_var_clean(&$4));
      YY_ASSERT_ACTION(!amxo_parser_set_param(parser_ctx, $2.txt, &$4),
                       $2.txt,
                       amxc_var_clean(&$4));
      amxc_var_clean(&$4);
    }
  | attributes TYPE name '=' value {
      bool key_attr_is_set = ($1 & (1 << attr_key));
      $3.txt[$3.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                       $3.txt,
                       amxc_var_clean(&$5));
      YY_ASSERT_ACTION(key_attr_is_set,
                       "Key parameters can not have a default value",
                       amxc_var_clean(&$5));
      YY_ASSERT_ACTION(!amxo_parser_push_param(parser_ctx, $3.txt, $1, $2),
                       $3.txt,
                       amxc_var_clean(&$5));
      YY_ASSERT_ACTION(!amxo_parser_set_param(parser_ctx, $3.txt, &$5),
                       $3.txt,
                       amxc_var_clean(&$5));
      amxc_var_clean(&$5);
    }
  ;

param_body
  : param_body param_content
  | param_content
  ;

param_content
  : action
  | dep_action
  | CONSTRAINT param_constraint ';'
  | DEFAULT value ';' {
      YY_ASSERT_ACTION(!amxo_parser_set_param(parser_ctx, NULL, &$2),
                       amxd_param_get_name(parser_ctx->param),
                       amxc_var_clean(&$2));
      amxc_var_clean(&$2);
  }
  ;

action
  : action_header action_function ';' {
      int retval = amxo_parser_set_action(parser_ctx, $1);
      YY_ASSERT(retval < 0, "Action");
      YY_WARNING(retval > 0, "Action");
    }
  | action_header action_function data ';' {
      int retval = amxo_parser_set_action(parser_ctx, $1);
      YY_ASSERT(retval < 0, "Action");
      YY_WARNING(retval > 0, "Action");
    }
  ;

action_header
  : ON ACTION_KW name {
      if ($3.length != 0) {
        $3.txt[$3.length] = 0;
      }
      $$ = amxo_parser_get_action_id(parser_ctx, $3.txt);
      YY_ASSERT($$ < 0 , $3.txt);
    }
  | ON ACTION_KW ACTION {
      $$ = $3;
    }
  ;

action_function
  : CALL name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, "auto");
      YY_ASSERT(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | CALL name RESOLVER {
      $2.txt[$2.length] = 0;
      $3.txt[$3.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, $3.txt);
      YY_ASSERT(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }

param_constraint
    : MIN DIGIT { // deprecated - must be removed at 01/01/2022
      int retval = amxo_parser_resolve_internal(parser_ctx, "check_minimum", "auto");
      YY_ASSERT(retval < 0, "check_minimum");
      YY_WARNING(retval > 0, "check_minimum");
      amxc_var_new(&parser_ctx->data);
      amxc_var_set(int64_t, parser_ctx->data, $2);
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT(retval < 0, "check_minimum");
      YY_WARNING(retval > 0, "check_minimum");
    }
  | MAX DIGIT { // deprecated - must be removed at 01/01/2022
      int retval = amxo_parser_resolve_internal(parser_ctx, "check_maximum", "auto");
      YY_ASSERT(retval < 0, "check_maximum");
      YY_WARNING(retval > 0, "check_maximum");
      amxc_var_new(&parser_ctx->data);
      amxc_var_set(int64_t, parser_ctx->data, $2);
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT(retval < 0, "check_maximum");
      YY_WARNING(retval > 0, "check_maximum");
    }
  | RANGE '[' DIGIT ',' DIGIT ']' { // deprecated - must be removed at 01/01/2022
      int retval = amxo_parser_resolve_internal(parser_ctx, "check_range", "auto");
      YY_ASSERT(retval < 0, "check_range");
      YY_WARNING(retval > 0, "check_range");
      amxc_var_new(&parser_ctx->data);
      amxc_var_set_type(parser_ctx->data, AMXC_VAR_ID_HTABLE);
      amxc_var_add_key(int64_t, parser_ctx->data, "min", $3);
      amxc_var_add_key(int64_t, parser_ctx->data, "max", $5);
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT(retval < 0, "check_range");
      YY_WARNING(retval > 0, "check_range");
    }
  | ENUM '[' values ']' { // deprecated - must be removed at 01/01/2022
      int retval = amxo_parser_resolve_internal(parser_ctx, "check_enum", "auto");
      YY_ASSERT(retval < 0, "check_enum");
      YY_WARNING(retval > 0, "check_enum");
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT(retval < 0, "check_enum");
      YY_WARNING(retval > 0, "check_enum");
    }
  | CUSTOM name { // deprecated - must be removed at 01/01/2022
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, "auto");
      YY_ASSERT(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | CUSTOM name IMPORT name { // deprecated - must be removed at 01/01/2022
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      char *resolver = amxo_parser_build_import_resolver_data($2.txt, $4.txt);
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, resolver);
      YY_ASSERT_ACTION(retval < 0, $2.txt, free(resolver));
      YY_WARNING(retval > 0, $2.txt);
      retval = amxo_parser_set_action(parser_ctx, action_validate);
      YY_ASSERT_ACTION(retval < 0, $2.txt, free(resolver));
      YY_WARNING(retval > 0, $2.txt);
      free(resolver);
    }
  ;

dep_action
  : deprecated_action WITH name ';' { // deprecated - must be removed at 01/01/2022
      $3.txt[$3.length] = 0; 
      int retval = amxo_parser_resolve_internal(parser_ctx, $3.txt, "auto");
      YY_ASSERT(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
      if ($1 == action_write) {
        YY_WARNING(!amxo_parser_subscribe_item(parser_ctx), $3.txt);
      } else {
        retval = amxo_parser_set_action(parser_ctx, $1);
        YY_ASSERT(retval < 0, $3.txt);
        YY_WARNING(retval > 0, $3.txt);
      }
    }
  | deprecated_action WITH name IMPORT name ';' { // deprecated - must be removed at 01/01/2022
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      char *resolver = amxo_parser_build_import_resolver_data($3.txt, $5.txt);
      int retval = amxo_parser_resolve_internal(parser_ctx, $3.txt, resolver);
      YY_ASSERT_ACTION(retval < 0, $3.txt, free(resolver));
      YY_WARNING(retval > 0, $3.txt);
      if ($1 == action_write) {
        YY_WARNING(!amxo_parser_subscribe_item(parser_ctx), $3.txt);
      } else {
        retval = amxo_parser_set_action(parser_ctx, $1);
        YY_ASSERT_ACTION(retval < 0, $3.txt, free(resolver));
        YY_WARNING(retval > 0, $3.txt);
      }
      free(resolver);
    }
  ;

deprecated_action
  : DEP_ACTION { // deprecated - must be removed at 01/01/2022
      $$ = $1;
    }
  | ACTION { // deprecated - must be removed at 01/01/2022
      $$ = $1;
    }
  | OF { // deprecated - must be removed at 01/01/2022
      $$ = action_add_inst;
    }
  ;

function_def
  : function_header '(' ')' ';' {
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, "auto");
      YY_ASSERT(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  | function_header '(' arguments ')' ';' { 
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, "auto");
      YY_ASSERT(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    } 
  | function_header '(' ')' RESOLVER ';' {
      if ($4.length != 0) {
        $4.txt[$4.length] = 0;
      }
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, $4.txt);
      YY_ASSERT(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  | function_header '(' arguments ')' RESOLVER';' { 
      if ($5.length != 0) {
        $5.txt[$5.length] = 0;
      }
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, $5.txt);
      YY_ASSERT(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  ;

function_header
  : TYPE name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_push_func(parser_ctx, $2.txt, 0, $1);
      YY_ASSERT(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | attributes TYPE name {
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_func_attrs), $3.txt);
      int retval = amxo_parser_push_func(parser_ctx, $3.txt, $1, $2);
      YY_ASSERT(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  ;

arguments
  : argument_def ',' arguments
  | argument_def
  ;

argument_def
  : TYPE name { 
      $2.txt[$2.length] = 0;
      YY_ASSERT(!amxo_parser_add_arg(parser_ctx, $2.txt, 0, $1, NULL), $2.txt);
    }
  | attributes TYPE name { 
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_arg_attrs), $3.txt);
      YY_ASSERT(!amxo_parser_add_arg(parser_ctx, $3.txt, $1, $2, NULL), $3.txt);
    }
  | TYPE name '=' value {
      $2.txt[$2.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_add_arg(parser_ctx, $2.txt, 0, $1, &$4),
                       $2.txt,
                       amxc_var_clean(&$4));
      amxc_var_clean(&$4);
    }
  | attributes TYPE name '=' value {
      $3.txt[$3.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_check_attr(parser_ctx, $1, amxo_arg_attrs),
                       $3.txt,
                       amxc_var_clean(&$5));
      YY_ASSERT_ACTION(!amxo_parser_add_arg(parser_ctx, $3.txt, $1, $2, &$5),
                       $3.txt,
                       amxc_var_clean(&$5));
      amxc_var_clean(&$5);
    }
  ;

counted
  : COUNTER WITH name ';' {
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_set_counter(parser_ctx, $3.txt), $3.txt);
    }
  ;

add_mib
  : EXTEND IMPORT OBJECT name ';' {
      $4.txt[$4.length] = 0;
      YY_ASSERT($3 != token_mib, $4.txt);
      YY_ASSERT(!amxo_parser_add_mib(parser_ctx, $4.txt), $4.txt);
    } 
  | EXTEND WITH OBJECT name ';' {
      $4.txt[$4.length] = 0;
      YY_ASSERT($3 != token_mib, $4.txt);
      YY_ASSERT(!amxo_parser_add_mib(parser_ctx, $4.txt), $4.txt);
    } 
  ;

populate
  : object_populate
  | event_populate
  ; 

event_populate
  : ON EVENT TEXT CALL name ';' {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $5.txt, "auto");
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, NULL, NULL);
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
    }
  | ON EVENT TEXT CALL name RESOLVER ';' {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      $6.txt[$6.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $5.txt, $6.txt);
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, NULL, NULL);
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
    }
  | ON EVENT TEXT OF TEXT CALL name';' {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      $7.txt[$7.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $7.txt, "auto");
      YY_ASSERT(retval < 0, $7.txt);
      YY_WARNING(retval > 0, $7.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, $5.txt, NULL);
      YY_ASSERT(retval < 0, $7.txt);
      YY_WARNING(retval > 0, $7.txt);
    }
  | ON EVENT TEXT OF TEXT CALL name RESOLVER';' {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      $7.txt[$7.length] = 0;
      $8.txt[$8.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $7.txt, $8.txt);
      YY_ASSERT(retval < 0, $7.txt);
      YY_WARNING(retval > 0, $7.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, $5.txt, NULL);
      YY_ASSERT(retval < 0, $7.txt);
      YY_WARNING(retval > 0, $7.txt);
    }
  | ON EVENT TEXT CALL name FILTER filter {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      $7.txt[$7.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $5.txt, "auto");
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, NULL, $7.txt);
      YY_ASSERT(retval < 0, $7.txt);
      YY_WARNING(retval > 0, $7.txt);
    }
  | ON EVENT TEXT CALL name RESOLVER FILTER filter  {
      $3.txt[$3.length] = 0;
      $5.txt[$5.length] = 0;
      $6.txt[$6.length] = 0;
      $8.txt[$8.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $5.txt, $6.txt);
      YY_ASSERT(retval < 0, $5.txt);
      YY_WARNING(retval > 0, $5.txt);
      retval = amxo_parser_subscribe(parser_ctx, $3.txt, NULL, $8.txt);
      YY_ASSERT(retval < 0, $8.txt);
      YY_WARNING(retval > 0, $8.txt);
    }
  ;

filter
  : TEXT ';' {
      $$ = $1;
    }
  ;

object_populate
  : object_pop_header ';' { 
      YY_ASSERT(!amxo_parser_pop_object(parser_ctx), "Validation");
    }
  | object_pop_header '{' '}' { 
      YY_ASSERT(!amxo_parser_pop_object(parser_ctx), "Validation"); 
    }
  | object_pop_header '{' object_pop_body '}' {
      YY_ASSERT(!amxo_parser_pop_object(parser_ctx), "Validation");
    }
  ;

object_pop_header
  : OBJECT path { 
      $2.txt[$2.length] = 0;
      YY_ASSERT(!amxo_parser_push_object(parser_ctx, $2.txt), $2.txt);
    }
  | unset_attributes attributes OBJECT path { 
      $4.txt[$4.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $4.txt);
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $2, amxo_object_attrs), $4.txt);
      YY_ASSERT(!amxo_parser_push_object(parser_ctx, $4.txt), $4.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $1, false), $4.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $2, true), $4.txt);
    }
  | attributes unset_attributes OBJECT path { 
      $4.txt[$4.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $4.txt);
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $2, amxo_object_attrs), $4.txt);
      YY_ASSERT(!amxo_parser_push_object(parser_ctx, $4.txt), $4.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $1, true), $4.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $2, false), $4.txt);
    }
  | unset_attributes OBJECT path { 
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_ASSERT(!amxo_parser_push_object(parser_ctx, $3.txt), $3.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $1, false),  $3.txt);
    }
  | attributes OBJECT path { 
      $3.txt[$3.length] = 0;
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_ASSERT(!amxo_parser_push_object(parser_ctx, $3.txt), $3.txt);
      YY_ASSERT(!amxo_parser_set_object_attrs(parser_ctx, $1, true), $3.txt);
    }
  | INSTANCE OF '(' DIGIT ',' name ')' {
      $6.txt[$6.length] = 0;
      YY_ASSERT(!amxo_parser_add_instance(parser_ctx, $4, $6.txt), $6.txt);
    }
  | INSTANCE OF '(' DIGIT ',' name ',' data_options ')' {
      $6.txt[$6.length] = 0;
      YY_ASSERT(!amxo_parser_add_instance(parser_ctx, $4, $6.txt), $6.txt);
    }
  | INSTANCE OF '(' data_options ')' {
      YY_ASSERT(!amxo_parser_add_instance(parser_ctx, 0, NULL), "");
    }
  | INSTANCE OF '(' ')' {
      YY_ASSERT(!amxo_parser_add_instance(parser_ctx, 0, NULL), "");
    }
  ;

object_pop_body
  : object_pop_body object_pop_content 
  | object_pop_content
  ;

object_pop_content
  : parameter {
      amxo_parser_pop_param(parser_ctx); 
    }
  | unset_attributes attributes parameter {
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $2, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $1, false),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $2, true),
                amxd_param_get_name(parser_ctx->param));
      amxo_parser_pop_param(parser_ctx); 
    }
  | attributes unset_attributes parameter {
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $2, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $1, true), 
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $2, false),
                amxd_param_get_name(parser_ctx->param));
      amxo_parser_pop_param(parser_ctx); 
    }
  | unset_attributes parameter {
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $1, false),
                amxd_param_get_name(parser_ctx->param));
      amxo_parser_pop_param(parser_ctx); 
    }
  | attributes parameter {
      YY_ASSERT(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                amxd_param_get_name(parser_ctx->param));
      YY_ASSERT(!amxo_parser_set_param_attrs(parser_ctx, $1, true),
                amxd_param_get_name(parser_ctx->param));
      amxo_parser_pop_param(parser_ctx); 
    }
  | object_populate
  | add_mib
  ;

parameter
  : PARAMETER name '=' value ';' {
      $2.txt[$2.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_set_param(parser_ctx, $2.txt, &$4),
                       $2.txt,
                       amxc_var_clean(&$4));
      amxc_var_clean(&$4);
    }
  | PARAMETER name ';' {
      $2.txt[$2.length] = 0;
      YY_ASSERT(!amxo_parser_set_param(parser_ctx, $2.txt, NULL), $2.txt);
    }
  ;

path
  : name '.' path {
    $1.txt[$1.length] = '.';
    $$.txt = $1.txt;
    $$.length = strlen($1.txt);
  }
  | name {
    $1.txt[$1.length] = 0;
  }
  ;

unset_attributes
  : UNSET_ATTRIBUTE unset_attributes{
      $$ = $1 | $2; 
    }
  | UNSET_ATTRIBUTE 
  ;

attributes
  : SET_ATTRIBUTE attributes { 
      $$ = $1 | $2; 
    }
  | SET_ATTRIBUTE
  ;

name
  : STRING
  | TEXT
  ;

data
  : value { 
      amxc_var_new(&parser_ctx->data); 
      amxc_var_copy(parser_ctx->data, &$1);
      amxc_var_clean(&$1);
    }
  | '{' data_options '}' 
  | '[' values ']'
  ;

data_options
  : {
      amxc_var_new(&parser_ctx->data); 
      amxc_var_set_type(parser_ctx->data, AMXC_VAR_ID_HTABLE);
    }
  | data_option ',' data_options
  | data_option
  ;

data_option
  : name '=' value {
      $1.txt[$1.length] = 0;
      YY_ASSERT_ACTION(!amxo_parser_set_data_option(parser_ctx, $1.txt, &$3),
                       $1.txt,
                       amxc_var_clean(&$3));
      amxc_var_clean(&$3);
    }
  ;

values
  : {
      amxc_var_new(&parser_ctx->data); 
      amxc_var_set_type(parser_ctx->data, AMXC_VAR_ID_LIST);
    }
  | value ',' values {
      YY_ASSERT_ACTION(!amxo_parser_set_data_option(parser_ctx, NULL, &$1),
                       "array",
                       amxc_var_clean(&$1));
      amxc_var_clean(&$1);
    }
  | value {
      YY_ASSERT_ACTION(!amxo_parser_set_data_option(parser_ctx, NULL, &$1),
                       "array",
                       amxc_var_clean(&$1));
      amxc_var_clean(&$1);
    }
  ;

value
  : TEXT { 
      $1.txt[$1.length] = 0;
      amxc_string_t txt;
      amxc_string_init(&txt, 0);
      amxc_string_push_buffer(&txt, $1.txt, $1.length + 1);
      amxc_string_trim(&txt, NULL);
      amxc_var_init(&$$); 
      amxc_var_set(cstring_t, &$$, amxc_string_take_buffer(&txt)); 
    }
  | STRING { 
      $1.txt[$1.length] = 0;
      amxc_var_init(&$$);
      amxc_var_set(cstring_t, &$$, $1.txt);
    }
  | DIGIT { 
      amxc_var_init(&$$);
      amxc_var_set(int64_t, &$$, $1);
    }
  | BOOL { 
      amxc_var_init(&$$);
      amxc_var_set(bool, &$$, $1);
    }
  ;

%%
