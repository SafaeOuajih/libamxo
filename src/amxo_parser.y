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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dlfcn.h>
#include <amxc/amxc.h>
#include <amxp/amxp.h>
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
  amxc_var_t* var;
  amxo_txt_regexp_t cregexp;
  amxo_action_t action;
}

%token <integer> REQUIRES
%token <integer> INCLUDE
%token <integer> IMPORT
%token <integer> USING
%token <integer> AS
%token <integer> INSTANCE
%token <integer> PARAMETER
%token <integer> DEFINE
%token <integer> CONFIG
%token <integer> POPULATE
%token <integer> OBJECT
%token <integer> SELECT
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
%token <integer> GLOBAL
%token <integer> CALL
%token <integer> ACTION_KW
%token <cptr>    STRING
%token <cptr>    MULTILINECOMMENT
%token <cptr>    TEXT
%token <cptr>    RESOLVER
%token <boolean> BOOL
%token <bitmap>  SET_ATTRIBUTE
%token <bitmap>  UNSET_ATTRIBUTE
%token <integer> REGEXP
%token <integer> FLAGS
%token <integer> PRINT

%type <integer> stream sections section config_options config_option
%type <integer> requires include import ldflags entry_point print
%type <integer> defines populates populate object_populate event_populate event_subscribe
%type <integer> define object_def object_def_header multi object_body object_content
%type <integer> parameter_def counted event_def
%type <integer> function_def arguments argument_def add_mib
%type <integer> object_pop_header object_pop_body object_pop_content param_pop_head parameter
%type <action>  action_header
%type <integer> action instance_id
%type <bitmap>  attributes unset_attributes
%type <cptr>    name path filter 
%type <cregexp> text_or_regexp
%type <var>     flags flag value values data_option data_options data

%{
    int yylex(YYSTYPE* lvalp, YYLTYPE* llocp, void * yyscanner);
    void yyerror(YYLTYPE* locp, void* scanner, const char* err);
    void yywarning(YYLTYPE* locp, void* scanner, const char* err);
    amxo_parser_t *yyget_extra ( void * yyscanner );

    void yyerror(YYLTYPE* locp, void* scanner, const char* err) {
        amxo_parser_t *context = (amxo_parser_t *)yyget_extra(scanner);
        if (context->status == amxd_status_ok) {
            if (!amxc_string_is_empty(&context->msg)) {
                amxo_parser_printf(context, "ERROR  : %s@%s:line %d\n", 
                                   amxc_string_get(&context->msg, 0),
                                   context->file,
                                   locp->first_line);
                amxc_string_reset(&context->msg);
            } else {
                amxo_parser_printf(context, "ERROR  : %s@%s:line %d\n", 
                                   err,
                                   context->file,
                                   locp->first_line);
            }
        } else {
            if (!amxc_string_is_empty(&context->msg)) {
                amxo_parser_printf(context, "ERROR  : %d - %s@%s:line %d\n",
                                   context->status,
                                   amxc_string_get(&context->msg, 0),
                                   context->file,
                                   locp->first_line);
                amxc_string_reset(&context->msg);
            } else {
                amxo_parser_printf(context, "ERROR  : %d - %s - %s@%s:line %d\n",
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
            amxo_parser_printf(context, "WARNING: %s@%s:line %d\n",
                               amxc_string_get(&context->msg, 0),
                               context->file,
                               locp->first_line);
            amxc_string_reset(&context->msg);
        } else {
            amxo_parser_printf(context, "WARNING: %s@%s:line %d\n",
                               err,
                               context->file,
                               locp->first_line);
        }
    }

    #define scanner x->scanner
    #define parser_ctx ((amxo_parser_t *)yyget_extra(ctx))
    #define YY_CHECK_ACTION(c,m,a) if(c) { yyerror(&yylloc, ctx, m); a; YYERROR; }
    #define YY_CHECK(c,m) if(c) { yyerror(&yylloc, ctx, m); YYERROR; }
    #define YY_WARNING(c,m) if(c) { yywarning(&yylloc, ctx, m); }
    #define NOOP

    const uint64_t amxo_object_attrs = ((1 << attr_readonly) | 
                                    (1 << attr_persistent) | 
                                    (1 << attr_private) | 
                                    (1 << attr_protected));

    const uint64_t amxo_param_attrs = ((1 << attr_persistent) | 
                                    (1 << attr_private) | 
                                    (1 << attr_template) |
                                    (1 << attr_instance) |
                                    (1 << attr_variable) |
                                    (1 << attr_readonly) |
                                    (1 << attr_key) |
                                    (1 << attr_unique) | 
                                    (1 << attr_protected) |
                                    (1 << attr_mutable));

    const uint64_t amxo_func_attrs = ((1 << attr_private) | 
                                    (1 << attr_template) | 
                                    (1 << attr_instance) | 
                                    (1 << attr_protected));

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
  | requires
  | print
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
  : path '=' data ';' {
      amxc_var_t* data = NULL;
      $1.txt[$1.length] = 0;
      YY_CHECK_ACTION(amxo_parser_set_config(parser_ctx, $1.txt, $3) != 0,
                       $1.txt,
                       amxc_var_delete(&$3);
                      );
      data = amxc_var_get_path(&parser_ctx->config, $1.txt, AMXC_VAR_FLAG_DEFAULT);
      amxo_hooks_set_config(parser_ctx, $1.txt, data);
      amxc_var_delete(&$3);
      $$ = 0;
    }
  | GLOBAL path '=' data ';' {
      amxc_var_t* data = NULL;
      $2.txt[$2.length] = 0;
      YY_CHECK_ACTION(amxo_parser_set_config(parser_ctx, $2.txt, $4) != 0,
                       $2.txt,
                       amxc_var_delete(&$4);
                      );
      amxc_string_t *name;
      amxc_string_new(&name, 0);
      amxc_string_append(name, $2.txt, $2.length);
      amxc_llist_append(&parser_ctx->global_config, &name->it);
      data = amxc_var_get_path(&parser_ctx->config, $2.txt, AMXC_VAR_FLAG_DEFAULT);
      amxo_hooks_set_config(parser_ctx, $2.txt, data);
      amxc_var_delete(&$4);
      $$ = 0;
    }
  ;

requires
  : REQUIRES TEXT ';' {
      amxc_var_t* req = NULL;
      amxc_string_t *name = NULL;

      $2.txt[$2.length] = 0;
      amxc_string_new(&name, 0);
      amxc_string_append(name, "requires", strlen("requires") + 1);
      amxc_llist_append(&parser_ctx->global_config, &name->it);

      req = amxo_parser_claim_config(parser_ctx, "requires");
      if (amxc_var_type_of(req) != AMXC_VAR_ID_LIST) {
          amxc_var_set_type(req, AMXC_VAR_ID_LIST);
      }
      amxc_var_add(cstring_t, req, $2.txt);
      $$ = 0;
    }
  ;

print
  : PRINT TEXT ';' {
    $2.txt[$2.length] = 0;
    amxo_parser_print(parser_ctx, $2.txt);
    $$ = 0;
  }

include
  : INCLUDE TEXT ';' {
      $2.txt[$2.length] = 0;
      int retval = 0;
      if ($1 != token_post_include) {
          retval = amxo_parser_include(parser_ctx, $2.txt);
          if (retval == 4) {
            // is an empty directory - not an error
            retval = 0;
          }
      } else {
          retval = amxo_parser_add_post_include(parser_ctx, $2.txt);
      }
      YY_CHECK(retval != 0 && !(retval == 2 && $1 == token_optional_include), $2.txt);
      parser_ctx->status = amxd_status_ok;
    }
  | INCLUDE TEXT ':' TEXT ';' {
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      int retval = amxo_parser_include(parser_ctx, $2.txt);
      if (retval == 2 || retval == 4) {
          // 2 = file/dir not found, 4 = empty directory
          parser_ctx->status = amxd_status_ok;
          retval = amxo_parser_include(parser_ctx, $4.txt);
          if (retval == 4) {
            retval = 0;
          }
      }
      YY_CHECK(retval != 0, $2.txt); 
    }
  ;

import
  : IMPORT TEXT ';' {
      $2.txt[$2.length] = 0;
      YY_CHECK(amxo_resolver_import_open(parser_ctx, $2.txt, $2.txt, RTLD_LAZY) != 0,
                $2.txt);
    }
  | IMPORT TEXT AS name ';' {
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      YY_CHECK(amxo_resolver_import_open(parser_ctx, $2.txt, $4.txt, RTLD_LAZY) != 0,
                $2.txt);
    }
  | IMPORT TEXT ldflags ';' {
      $2.txt[$2.length] = 0;
      YY_CHECK(amxo_resolver_import_open(parser_ctx, $2.txt, $2.txt, $3) != 0,
                $2.txt);
    }
  | IMPORT TEXT ldflags AS name ';' {
      $2.txt[$2.length] = 0;
      $5.txt[$5.length] = 0;
      YY_CHECK(amxo_resolver_import_open(parser_ctx, $2.txt, $5.txt, $3) != 0,
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
  | function_def
  | entry_point
  ; 

object_def
  : object_def_header ';' {
      int type = amxd_object_get_type(parser_ctx->object);
      YY_WARNING(type == amxd_object_mib, "Empty MIB object defined");
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed");
    }
  | object_def_header '{' '}' {
      int type = amxd_object_get_type(parser_ctx->object);
      YY_WARNING(type == amxd_object_mib, "Empty MIB object defined");
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed");
    }
  | object_def_header '{' object_body '}' {
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed");
    }
  ;

entry_point
  : ENTRY name '.' name ';' {
      $2.txt[$2.length] = 0;
      $4.txt[$4.length] = 0;
      YY_CHECK(amxo_parser_call_entry_point(parser_ctx, $2.txt, $4.txt) != 0,
                $2.txt);

    }
  ;

object_def_header
  : OBJECT name {
      amxd_object_type_t type = ($1 == token_mib)?amxd_object_mib:amxd_object_singleton;
      $2.txt[$2.length] = 0;
      YY_CHECK(amxo_parser_create_object(parser_ctx, $2.txt, 0, type) < 0, $2.txt);
    }
  | attributes OBJECT name {
      amxd_object_type_t type = ($2 == token_mib)?amxd_object_mib:amxd_object_singleton;
      $3.txt[$3.length] = 0;
      YY_WARNING(type == amxd_object_mib, "Attributes declared on mib object are ignored");
      if (type != amxd_object_mib) {
        YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      }
      YY_CHECK(amxo_parser_create_object(parser_ctx, $3.txt, $1, amxd_object_singleton) < 0, $3.txt);
    }
  | OBJECT name multi {
      $2.txt[$2.length] = 0;
      YY_CHECK($1 == token_mib, "Mib objects can not be multi-instance");
      YY_CHECK(amxo_parser_create_object(parser_ctx, $2.txt, 0, amxd_object_template) < 0, $2.txt);
      YY_WARNING(amxd_object_set_max_instances(parser_ctx->object, $3) != amxd_status_ok,
                 "Failed to set maximum instances");
    }
  | attributes OBJECT name multi {
      $3.txt[$3.length] = 0;
      YY_CHECK($2 == token_mib, "Mib objects can not be multi-instance");
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_CHECK(amxo_parser_create_object(parser_ctx, $3.txt, $1, amxd_object_template) < 0, $3.txt);
      YY_WARNING(amxd_object_set_max_instances(parser_ctx->object, $4) != amxd_status_ok,
                 "Failed to set maximum instances");
    }
  | SELECT path {
      $2.txt[$2.length] = 0;
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $2.txt), $2.txt);
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
  | add_mib
  | event_def
  | event_subscribe
  ;

parameter_def
  : param_header ';' { 
      YY_CHECK(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  | param_header '{' '}' {
      YY_CHECK(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  | param_header '{' param_body '}' {
      YY_CHECK(!amxo_parser_pop_param(parser_ctx), "Validate"); 
    }
  ; 

param_header
  : TYPE name {
      $2.txt[$2.length] = 0;
      YY_CHECK(!amxo_parser_push_param(parser_ctx, $2.txt, 0, $1), $2.txt);
    }
  | attributes TYPE name {
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs), $3.txt);
      YY_CHECK(!amxo_parser_push_param(parser_ctx, $3.txt, $1, $2), $3.txt);
    }
  | TYPE name '=' value {
      int retval = 0;
      $2.txt[$2.length] = 0;
      YY_CHECK_ACTION(!amxo_parser_push_param(parser_ctx, $2.txt, 0, $1),
                       $2.txt,
                       amxc_var_delete(&$4));
      retval = amxo_parser_set_param(parser_ctx, $2.txt, $4);
      YY_CHECK_ACTION(retval < 0, $2.txt, amxc_var_delete(&$4));
      amxc_var_delete(&$4);
    }
  | attributes TYPE name '=' value {
      bool key_attr_is_set = ($1 & (1 << attr_key));
      int retval = 0;
      $3.txt[$3.length] = 0;
      YY_CHECK_ACTION(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                       $3.txt,
                       amxc_var_delete(&$5));
      YY_CHECK_ACTION(key_attr_is_set,
                       "Key parameters can not have a default value",
                       parser_ctx->status = amxd_status_invalid_value;
                       amxc_var_delete(&$5));
      YY_CHECK_ACTION(!amxo_parser_push_param(parser_ctx, $3.txt, $1, $2),
                       $3.txt,
                       amxc_var_delete(&$5));
      retval = amxo_parser_set_param(parser_ctx, $3.txt, $5);
      YY_CHECK_ACTION(retval < 0, $3.txt, amxc_var_delete(&$5));
      amxc_var_delete(&$5);
    }
  ;

param_body
  : param_body param_content
  | param_content
  ;

param_content
  : action
  | DEFAULT value ';' {
      int retval = amxo_parser_set_param(parser_ctx, NULL, $2);
      YY_CHECK_ACTION(retval < 0,
                       amxd_param_get_name(parser_ctx->param),
                       amxc_var_delete(&$2));
      amxc_var_delete(&$2);
    }
  | FLAGS flags ';' {
      YY_CHECK(!amxo_parser_set_param_flags(parser_ctx, $2), "Parameter flags");
    }
  ;

action
  : action_header action_function ';' {
      int retval = amxo_parser_set_action(parser_ctx, $1, NULL);
      YY_CHECK(retval < 0, "Action");
      YY_WARNING(retval > 0, "Action");
    }
  | action_header action_function data ';' {
      int retval = amxo_parser_set_action(parser_ctx, $1, $3);
      YY_CHECK(retval < 0, "Action");
      YY_WARNING(retval > 0, "Action");
    }
  ;

action_header
  : ON ACTION_KW name {
      if ($3.length != 0) {
        $3.txt[$3.length] = 0;
      }
      $$ = amxo_parser_get_action_id(parser_ctx, $3.txt);
      YY_CHECK($$ < 0 , $3.txt);
    }
  | ON ACTION_KW TYPE {
      YY_CHECK($3 != AMXC_VAR_ID_LIST , "Invalid action");
      $$ = action_list;
    }
  ;

action_function
  : CALL name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, amxo_function_action, "auto");
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | CALL name RESOLVER {
      $2.txt[$2.length] = 0;
      $3.txt[$3.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, amxo_function_action, $3.txt);
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }

function_def
  : function_header func_args ';' {
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, amxo_function_rpc, "auto");
      YY_CHECK(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  | function_header func_args RESOLVER ';' {
      if ($3.length != 0) {
        $3.txt[$3.length] = 0;
      }
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, amxo_function_rpc, $3.txt);
      YY_CHECK(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  | function_header func_args '{' func_body '}'  {
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, amxo_function_rpc, "auto");
      YY_CHECK(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  | function_header func_args RESOLVER '{' func_body '}' {
      if ($3.length != 0) {
        $3.txt[$3.length] = 0;
      }
      const char *func_name = amxd_function_get_name(parser_ctx->func);
      int retval = amxo_parser_resolve_internal(parser_ctx, func_name, amxo_function_rpc, $3.txt);
      YY_CHECK(retval < 0, func_name);
      YY_WARNING(retval > 0, func_name);
      amxo_parser_pop_func(parser_ctx); 
    }
  ;

function_header
  : TYPE name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_push_func(parser_ctx, $2.txt, 0, $1);
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | attributes TYPE name {
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_func_attrs), $3.txt);
      int retval = amxo_parser_push_func(parser_ctx, $3.txt, $1, $2);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  ;

func_args
  : '(' ')'
  | '(' arguments ')'
  ;

arguments
  : argument_def ',' arguments
  | argument_def
  ;

func_body
  : FLAGS flags ';' {
      YY_CHECK(!amxo_parser_set_function_flags(parser_ctx, $2), "Function flags");
    }
  ;

argument_def
  : TYPE name { 
      $2.txt[$2.length] = 0;
      YY_CHECK(!amxo_parser_add_arg(parser_ctx, $2.txt, 0, $1, NULL), $2.txt);
    }
  | attributes TYPE name { 
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_arg_attrs), $3.txt);
      YY_CHECK(!amxo_parser_add_arg(parser_ctx, $3.txt, $1, $2, NULL), $3.txt);
    }
  | TYPE name '=' value {
      $2.txt[$2.length] = 0;
      YY_CHECK_ACTION(!amxo_parser_add_arg(parser_ctx, $2.txt, 0, $1, $4),
                       $2.txt,
                       amxc_var_delete(&$4));
      amxc_var_delete(&$4);
    }
  | attributes TYPE name '=' value {
      $3.txt[$3.length] = 0;
      YY_CHECK_ACTION(!amxo_parser_check_attr(parser_ctx, $1, amxo_arg_attrs),
                       $3.txt,
                       amxc_var_delete(&$5));
      YY_CHECK_ACTION(!amxo_parser_add_arg(parser_ctx, $3.txt, $1, $2, $5),
                       $3.txt,
                       amxc_var_delete(&$5));
      amxc_var_delete(&$5);
    }
  ;

counted
  : COUNTER WITH name ';' {
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_set_counter(parser_ctx, $3.txt), $3.txt);
    }
  ;

add_mib
  : EXTEND USING OBJECT name ';' {
      $4.txt[$4.length] = 0;
      YY_CHECK($3 != token_mib, $4.txt);
      YY_CHECK(!amxo_parser_add_mib(parser_ctx, $4.txt), $4.txt);
    } 
  | EXTEND WITH OBJECT name ';' {
      $4.txt[$4.length] = 0;
      YY_CHECK($3 != token_mib, $4.txt);
      YY_CHECK(!amxo_parser_add_mib(parser_ctx, $4.txt), $4.txt);
    } 
  ;

event_def
  : EVENT name ';' {
      $2.txt[$2.length] = 0;
      amxd_dm_t *dm = amxd_object_get_dm(parser_ctx->object);
      YY_CHECK(dm == NULL, "Can't add event");
      amxp_sigmngr_add_signal(&dm->sigmngr, $2.txt);
      amxd_object_add_event(parser_ctx->object, $2.txt);
      $$ = 0;
    }
  ;

event_subscribe
  : ON EVENT text_or_regexp event_func ';' {
      int retval = amxo_parser_subscribe_object(parser_ctx,
                                                $3.txt, $3.is_regexp,
                                                NULL);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  | ON EVENT text_or_regexp event_func filter ';' {
      $5.txt[$5.length] = 0;
      int retval = amxo_parser_subscribe_object(parser_ctx,
                                                $3.txt, $3.is_regexp,
                                                $5.txt);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  ;

populate
  : object_populate
  | event_populate
  ; 

event_populate
  : ON EVENT text_or_regexp event_func ';' {
      int retval = amxo_parser_subscribe(parser_ctx, 
                                         $3.txt, $3.is_regexp, 
                                         NULL);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  | ON EVENT text_or_regexp OF text_or_regexp event_func';' {
      int retval = amxo_parser_subscribe_path(parser_ctx,
                                              $3.txt, $3.is_regexp,
                                              $5.txt, $5.is_regexp);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  | ON EVENT text_or_regexp event_func filter';' {
      $5.txt[$5.length] = 0;
      int retval = amxo_parser_subscribe(parser_ctx, 
                                         $3.txt, $3.is_regexp,
                                         $5.txt);
      YY_CHECK(retval < 0, $3.txt);
      YY_WARNING(retval > 0, $3.txt);
    }
  ;

text_or_regexp
  : TEXT {
      $1.txt[$1.length] = 0;
      $$.txt = $1.txt;
      $$.is_regexp = false;  
    }
  | REGEXP '(' TEXT ')' {
      $3.txt[$3.length] = 0;
      $$.txt = $3.txt;
      $$.is_regexp = true;  
    }
  ;

event_func
  : CALL name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, amxo_function_event, "auto");
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  | CALL name RESOLVER {
      $2.txt[$2.length] = 0;
      $3.txt[$3.length] = 0;
      int retval = amxo_parser_resolve_internal(parser_ctx, $2.txt, amxo_function_event, $3.txt);
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  ;

filter
  : FILTER TEXT {
      $$ = $2;
    }
  ;

object_populate
  : object_pop_header ';' { 
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed");
    }
  | object_pop_header '{' '}' { 
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed"); 
    }
  | object_pop_header '{' object_pop_body '}' {
      YY_CHECK(!amxo_parser_pop_object(parser_ctx), "Validation failed");
    }
  ;

object_pop_header
  : OBJECT path { 
      $2.txt[$2.length] = 0;
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $2.txt), $2.txt);
    }
  | unset_attributes attributes OBJECT path { 
      $4.txt[$4.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $4.txt);
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $2, amxo_object_attrs), $4.txt);
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $4.txt), $4.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $1, false), $4.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $2, true), $4.txt);
    }
  | attributes unset_attributes OBJECT path { 
      $4.txt[$4.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $4.txt);
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $2, amxo_object_attrs), $4.txt);
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $4.txt), $4.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $1, true), $4.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $2, false), $4.txt);
    }
  | unset_attributes OBJECT path { 
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $3.txt), $3.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $1, false),  $3.txt);
    }
  | attributes OBJECT path { 
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_object_attrs), $3.txt);
      YY_CHECK(!amxo_parser_push_object(parser_ctx, $3.txt), $3.txt);
      YY_CHECK(!amxo_parser_set_object_attrs(parser_ctx, $1, true), $3.txt);
    }
  | INSTANCE OF '(' ')' {
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, 0, NULL, NULL), "");
    }
  | INSTANCE OF '(' instance_id ')'
  ;

instance_id
  : DIGIT {
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, $1, NULL, NULL), "");
    }
  | name {
      $1.txt[$1.length] = 0;
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, 0, $1.txt, NULL), "");
    }
  | DIGIT ',' name {
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, $1, $3.txt, NULL), "");
    }
  | data_options {
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, 0, NULL, $1), "");
    }
  | DIGIT ',' data_options {
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, $1, NULL, $3), "");
    }
  | name ',' data_options {
      $1.txt[$1.length] = 0;
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, 0, $1.txt, $3), "");
    }
  | DIGIT ',' name ',' data_options {
      $3.txt[$3.length] = 0;
      YY_CHECK(!amxo_parser_add_instance(parser_ctx, $1, $3.txt, $5), "");
    }
  ;

object_pop_body
  : object_pop_body object_pop_content 
  | object_pop_content
  ;

object_pop_content
  : param_pop_head ';' {
      amxo_parser_pop_param(parser_ctx); 
    }
  | param_pop_head '{' '}'  {
      amxo_parser_pop_param(parser_ctx); 
    }
  | param_pop_head '{' param_pop_body '}'  {
      amxo_parser_pop_param(parser_ctx); 
    }
  | object_populate
  | add_mib
  ;

param_pop_head
  : parameter
  | unset_attributes attributes parameter {
        if (parser_ctx->param != NULL) {
            YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                      amxd_param_get_name(parser_ctx->param));
            YY_CHECK(!amxo_parser_check_attr(parser_ctx, $2, amxo_param_attrs),
                      amxd_param_get_name(parser_ctx->param));
            YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $1, false),
                      amxd_param_get_name(parser_ctx->param));
            YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $2, true),
                      amxd_param_get_name(parser_ctx->param));
      }
    }
  | attributes unset_attributes parameter {
      if (parser_ctx->param != NULL) {
          YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                    amxd_param_get_name(parser_ctx->param));
          YY_CHECK(!amxo_parser_check_attr(parser_ctx, $2, amxo_param_attrs),
                    amxd_param_get_name(parser_ctx->param));
          YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $1, true), 
                    amxd_param_get_name(parser_ctx->param));
          YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $2, false),
                    amxd_param_get_name(parser_ctx->param));
      }
    }
  | unset_attributes parameter {
      if (parser_ctx->param != NULL) {
          YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                    amxd_param_get_name(parser_ctx->param));
          YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $1, false),
                    amxd_param_get_name(parser_ctx->param));
      }
    }
  | attributes parameter {
      if (parser_ctx->param != NULL) {
          YY_CHECK(!amxo_parser_check_attr(parser_ctx, $1, amxo_param_attrs),
                    amxd_param_get_name(parser_ctx->param));
          YY_CHECK(!amxo_parser_set_param_attrs(parser_ctx, $1, true),
                    amxd_param_get_name(parser_ctx->param));
      }
    }
  ;

parameter
  : PARAMETER name '=' value {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_set_param(parser_ctx, $2.txt, $4);
      YY_CHECK_ACTION(retval < 0, $2.txt, amxc_var_delete(&$4));
      YY_WARNING(retval > 0, $2.txt);
      amxc_var_delete(&$4);
    }
  | PARAMETER name {
      $2.txt[$2.length] = 0;
      int retval = amxo_parser_set_param(parser_ctx, $2.txt, NULL);
      YY_CHECK(retval < 0, $2.txt);
      YY_WARNING(retval > 0, $2.txt);
    }
  ;

param_pop_body
  : FLAGS flags ';' {
      YY_CHECK(!amxo_parser_set_param_flags(parser_ctx, $2), "Parameter flags");
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
      $$ = $1;
    }
  ;

data_options
  : data_option ',' data_options {
        amxc_var_for_each(var, $1) {
          amxc_var_set_path($3, amxc_var_key(var), var, AMXC_VAR_FLAG_UPDATE | AMXC_VAR_FLAG_COPY | AMXC_VAR_FLAG_AUTO_ADD);
        }
        amxc_var_delete(&$1);
        $$ = $3;
    }
  | data_option {
        $$ = $1;
        amxc_var_for_each(var, $1) {
          char* key = strdup(amxc_var_key(var));
          amxc_var_take_it(var);
          amxc_var_set_path($1, key, var, AMXC_VAR_FLAG_UPDATE | AMXC_VAR_FLAG_COPY | AMXC_VAR_FLAG_AUTO_ADD);
          free(key);
          amxc_var_delete(&var);
        }

    }
  ;

data_option
  : path '=' value {
      $1.txt[$1.length] = 0;
      amxc_var_new(&$$);
      amxc_var_set_type($$, AMXC_VAR_ID_HTABLE);
      amxc_var_set_key($$, $1.txt, $3, AMXC_VAR_FLAG_DEFAULT);
    }
  ;

values
  : values ',' value {
      if (amxc_var_type_of($1) != AMXC_VAR_ID_LIST) {
        amxc_var_new(&$$);
        amxc_var_set_type($$, AMXC_VAR_ID_LIST);
        amxc_var_set_index($$, -1, $1, AMXC_VAR_FLAG_DEFAULT);
      }
      amxc_var_set_index($$, -1, $3, AMXC_VAR_FLAG_DEFAULT);
    }
  | value {
      if (amxc_var_type_of($1) != AMXC_VAR_ID_LIST) {
        amxc_var_new(&$$);
        amxc_var_set_type($$, AMXC_VAR_ID_LIST);
        amxc_var_set_index($$, -1, $1, AMXC_VAR_FLAG_DEFAULT);
      } else {
        $$ = $1;
      }
    }
  ;

value
  : TEXT { 
      $1.txt[$1.length] = 0;
      amxc_string_t txt;
      amxc_string_init(&txt, $1.length + 1);
      amxc_string_append(&txt, $1.txt, $1.length);
      amxc_string_trim(&txt, NULL);
      amxc_string_resolve(&txt, &parser_ctx->config);
      amxc_var_new(&$$); 
      amxc_var_push(cstring_t, $$, amxc_string_take_buffer(&txt));
    }
  | STRING { 
      $1.txt[$1.length] = 0;
      amxc_var_new(&$$);
      amxc_var_set(cstring_t, $$, $1.txt);
    }
  | DIGIT { 
      amxc_var_new(&$$);
      amxc_var_set(int64_t, $$, $1);
    }
  | BOOL { 
      amxc_var_new(&$$);
      amxc_var_set(bool, $$, $1);
    }
  | '{' '}' {
      amxc_var_new(&$$);
      amxc_var_set_type($$, AMXC_VAR_ID_HTABLE);
    }
  | '[' ']' {
      amxc_var_new(&$$);
      amxc_var_set_type($$, AMXC_VAR_ID_LIST);
    }
  | '{' data_options '}' {
        $$ = $2;
    }
  | '[' values ']' {
        $$ = $2;
    }

  ;

flags
  : flags ',' flag {
      if (amxc_var_type_of($1) != AMXC_VAR_ID_HTABLE) {
        amxc_var_new(&$$);
        amxc_var_set_type($$, AMXC_VAR_ID_HTABLE);
        amxc_var_set_key($$, $1->hit.key, $1, AMXC_VAR_FLAG_DEFAULT | AMXC_VAR_FLAG_UPDATE);
      }
      amxc_var_set_key($$, $3->hit.key, $3, AMXC_VAR_FLAG_DEFAULT | AMXC_VAR_FLAG_UPDATE);
    }
  | flag {
      if (amxc_var_type_of($1) != AMXC_VAR_ID_HTABLE) {
        amxc_var_new(&$$);
        amxc_var_set_type($$, AMXC_VAR_ID_HTABLE);
        amxc_var_set_key($$, $1->hit.key, $1, AMXC_VAR_FLAG_DEFAULT | AMXC_VAR_FLAG_UPDATE);
      } else {
        $$ = $1;
      }
    }
  ;

flag
  : '%' STRING {
      $2.txt[$2.length] = 0;
      amxc_var_new(&$$);
      amxc_var_set(bool, $$, true);
      $$->hit.key = strdup($2.txt);
    }
  | '!' STRING {
      $2.txt[$2.length] = 0;
      amxc_var_new(&$$);
      amxc_var_set(bool, $$, false);
      $$->hit.key = strdup($2.txt);
  }
  ;

%%
