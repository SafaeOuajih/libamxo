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
%option reentrant bison-bridge
%option never-interactive
%option nounput
%option noinput
%option noyywrap
%option bison-locations

%{
    #define GNU_SOURCE
    #include <string.h>
    #include <sys/types.h>
    #include <dlfcn.h>
    #include <stdint.h>
    #include <amxc/amxc.h>
    #include <amxp/amxp_signal.h>
    #include <amxd/amxd_dm.h>
    #include <amxd/amxd_parameter.h>
    #include <amxo/amxo.h>
    #include "amxo_parser_priv.h"
    #include "amxo_parser_hooks_priv.h"
    #include "amxo_parser.tab.h"

    #if __SIZE_WIDTH__ == 64
    #define ssize_t_abs(x) labs(x)
    #else
    #define ssize_t_abs(x) abs(x)
    #endif
   
    #define YY_EXTRA_TYPE amxo_parser_t*
    #define YY_INPUT(buf,result,max_size) { \
      ssize_t read_result = 0; \
      read_result = yyextra->reader(yyextra, buf, max_size); \
      if (read_result == -1) { \
        YY_FATAL_ERROR( "input in flex scanner failed" ); \
      } \
      result = read_result > 0? ssize_t_abs(read_result):0; \
    }

    extern int yylex \
               (YYSTYPE * yylval_param ,  YYLTYPE* yylloc_param, yyscan_t yyscanner);

    #define UNUSED __attribute__((unused))
    #define YY_DECL int yylex \
               (YYSTYPE * yylval_param , UNUSED YYLTYPE* yylloc_param, yyscan_t yyscanner)
%}

%option yylineno

%{
    #define YY_USER_ACTION { \
        yylloc->first_line = yylineno; \
        yyextra->line = yylineno; \
    }
%}

%x MULTI_LINE_COMMENT SINGLE_LINE_COMMENT LONG_TEXT LONG_TEXT_SQ FUNC_RESOLVER

NEWLINE  \r?\n

%%

"/*"                                   { BEGIN( MULTI_LINE_COMMENT ); }
<MULTI_LINE_COMMENT>{
    "*"                { yymore(); }
    [^*\n]+            { yymore(); }
    [^*\n]*{NEWLINE}   { yymore(); }
    "*/"               { yylval->cptr.txt = yytext;
                         yylval->cptr.length = yyleng - 2;
                         // yylval->cptr = calloc(1,yyleng+1);
                         // memcpy(yylval->cptr ,yytext,yyleng-2);
                         BEGIN(INITIAL);
                       }
}
                                       
"#"                                    { BEGIN( SINGLE_LINE_COMMENT ); }
"//"                                   { BEGIN( SINGLE_LINE_COMMENT ); }
<SINGLE_LINE_COMMENT>{NEWLINE}         { BEGIN(INITIAL); }
<SINGLE_LINE_COMMENT>"*"               { }
<SINGLE_LINE_COMMENT>[^*\n]+           { }

"'"                                   { BEGIN(LONG_TEXT_SQ); }
<LONG_TEXT_SQ>{
    "*"              { yymore(); }
    {NEWLINE}        { yymore(); }
    [^\\']+          { yymore(); }
    [\\].            { yymore(); }
    "'"              { yylval->cptr.txt = yytext;
                       yylval->cptr.length = yyleng - 1;
                       BEGIN(INITIAL);
                       return TEXT;
                     }
    .                { yymore(); }
}


"\""                                   { BEGIN(LONG_TEXT); }
<LONG_TEXT>{
    "*"              { yymore(); }
    {NEWLINE}        { yymore(); }
    [^\\"]+          { yymore(); }
    [\\].            { yymore(); }
    "\""             { yylval->cptr.txt = yytext;
                       yylval->cptr.length = yyleng - 1;
                       BEGIN(INITIAL);
                       return TEXT;
                     }
    .                { yymore(); }
}

"<!"                 { BEGIN( FUNC_RESOLVER ); }
<FUNC_RESOLVER>{
    {NEWLINE}        { yymore(); }
    "!>"             { if (yyleng - 2 == 0) {
                           yylval->cptr.txt = NULL;
                           yylval->cptr.length = 0;
                       } else {
                           yylval->cptr.txt = yytext;
                           yylval->cptr.length = yyleng - 2;
                       }
                       BEGIN(INITIAL);
                       return RESOLVER;
                     }
    .                { yymore(); }
}

[ \t]                     { }
[\n]                      { }

<<EOF>>                   { yylval->integer = token_eof;
                            yyterminate();
                            return EOF_TOKEN; 
                          }

%config                   { yylval->integer = token_config; 
                            amxo_hooks_start_section(yyextra, 0);
                            return CONFIG;
                          }
%define                   { yylval->integer = token_define; 
                            amxo_hooks_start_section(yyextra, 1);
                            return DEFINE;
                          }
%populate                 { yylval->integer = token_populate; 
                            amxo_hooks_start_section(yyextra, 2);
                            return POPULATE;
                          }
datamodel                 { yylval->integer = token_populate; 
                            amxo_hooks_start_section(yyextra, 2);
                            return POPULATE;
                          }

object                    { yylval->integer = token_object;  
                            return OBJECT; 
                          }
select                    { yylval->integer = token_object;  
                            return SELECT; 
                          }
mib                       { yylval->integer = token_mib;  
                            return OBJECT; 
                          }
parameter                 { yylval->integer = token_keyword;  
                            return PARAMETER; 
                          }
&include                  { yylval->integer = token_post_include;
                            return INCLUDE;
                          }
#include                  { yylval->integer = token_optional_include;
                            return INCLUDE;
                          }
\?include                 { yylval->integer = token_conditional_include;
                            return INCLUDE;
                          }
include                   { yylval->integer = token_include;
                            return INCLUDE;
                          }
import                    { yylval->integer = token_import;
                            return IMPORT;
                          }
using                     { yylval->integer = token_import;
                            return IMPORT;
                          }
as                        { yylval->integer = token_keyword;
                            return AS;
                          }
instance                  { yylval->integer = token_keyword;
                            return INSTANCE;
                          }
of                        { yylval->integer = token_keyword;
                            return OF;
                          }
add                       { yylval->integer = token_keyword;
                            return OF;
                          }
extend                    {  yylval->integer = token_keyword;
                             return EXTEND;
                          }

%read-only                { yylval->integer = 1 << attr_readonly; 
                            return SET_ATTRIBUTE; 
                          }
read-only                 { yylval->integer = 1 << attr_readonly; 
                            return SET_ATTRIBUTE; 
                          }
%persistent               { yylval->integer = 1 << attr_persistent; 
                            return SET_ATTRIBUTE; 
                          }
persistent                { yylval->integer = 1 << attr_persistent; 
                            return SET_ATTRIBUTE; 
                          }
%private                  { yylval->integer = 1 << attr_private; 
                            return SET_ATTRIBUTE; 
                          }
%protected                { yylval->integer = 1 << attr_protected; 
                            return SET_ATTRIBUTE; 
                          }
%template                 { yylval->integer = 1 << attr_template; 
                            return SET_ATTRIBUTE; 
                          }
template-only             { yylval->integer = 1 << attr_template; 
                            return SET_ATTRIBUTE; 
                          }
%instance                 { yylval->integer = 1 << attr_instance; 
                            return SET_ATTRIBUTE; 
                          }
%volatile                 { yylval->integer = 1 << attr_variable; 
                            return SET_ATTRIBUTE; 
                          }
volatile                  { yylval->integer = 1 << attr_variable; 
                            return SET_ATTRIBUTE; 
                          }
%in                       { yylval->integer = 1 << attr_in; 
                            return SET_ATTRIBUTE; 
                          }
%out                      { yylval->integer = 1 << attr_out; 
                            return SET_ATTRIBUTE; 
                          }
in                        { yylval->integer = 1 << attr_in; 
                            return SET_ATTRIBUTE; 
                          }
out                       { yylval->integer = 1 << attr_out; 
                            return SET_ATTRIBUTE; 
                          }
%mandatory                { yylval->integer = 1 << attr_mandatory; 
                            return SET_ATTRIBUTE; 
                          }
mandatory                 { yylval->integer = 1 << attr_mandatory; 
                            return SET_ATTRIBUTE; 
                          }
%strict                   { yylval->integer = 1 << attr_strict; 
                            return SET_ATTRIBUTE; 
                          }
%key                      { yylval->integer = 1 << attr_key; 
                            return SET_ATTRIBUTE; 
                          }
%unique                   { yylval->integer = 1 << attr_unique; 
                            return SET_ATTRIBUTE; 
                          }
%global                   { yylval->integer = token_keyword;
                            return GLOBAL;
                          }
!read-only                { yylval->integer = 1 << attr_readonly; 
                            return UNSET_ATTRIBUTE; 
                          }
!persistent               { yylval->integer = 1 << attr_persistent; 
                            return UNSET_ATTRIBUTE; 
                          }
!private                  { yylval->integer = 1 << attr_private; 
                            return UNSET_ATTRIBUTE; 
                          }
!protected                { yylval->integer = 1 << attr_protected; 
                            return UNSET_ATTRIBUTE; 
                          }
!volatile                 { yylval->integer = 1 << attr_variable; 
                            return UNSET_ATTRIBUTE; 
                          }
void                      { yylval->integer = AMXC_VAR_ID_NULL;
                            return TYPE;   
                          }
string                    { yylval->integer = AMXC_VAR_ID_CSTRING;
                            return TYPE;
                          }
csv_string                { yylval->integer = AMXC_VAR_ID_CSV_STRING;
                            return TYPE;
                          }
ssv_string                { yylval->integer = AMXC_VAR_ID_SSV_STRING;
                            return TYPE;
                          }
int8                      { yylval->integer = AMXC_VAR_ID_INT8;
                            return TYPE;
                          }
int16                     { yylval->integer = AMXC_VAR_ID_INT16;
                            return TYPE;
                          }
int32                     { yylval->integer = AMXC_VAR_ID_INT32;
                            return TYPE;
                          }
int64                     { yylval->integer = AMXC_VAR_ID_INT64;
                            return TYPE;
                          }
uint8                     { yylval->integer = AMXC_VAR_ID_UINT8;
                            return TYPE;
                          }
uint16                    { yylval->integer = AMXC_VAR_ID_UINT16;
                            return TYPE;
                          }
uint32                    { yylval->integer = AMXC_VAR_ID_UINT32;
                            return TYPE;
                          }
uint64                    { yylval->integer = AMXC_VAR_ID_UINT64;
                            return TYPE;
                          }
bool                      { yylval->integer = AMXC_VAR_ID_BOOL;
                            return TYPE;
                          }
list                      { yylval->integer = AMXC_VAR_ID_LIST;
                            return TYPE;
                          }
htable                    { yylval->integer = AMXC_VAR_ID_HTABLE;
                            return TYPE;
                          }
fd                        { yylval->integer = AMXC_VAR_ID_FD;
                            return TYPE;
                          }
variant                   { yylval->integer = AMXC_VAR_ID_ANY;
                            return TYPE;
                          }
double                    { yylval->integer = AMXC_VAR_ID_DOUBLE;
                            return TYPE;
                          }
datetime                  { yylval->integer = AMXC_VAR_ID_TIMESTAMP;
                            return TYPE;
                          }

true                      { yylval->boolean = true;
                            return BOOL;
                          }
false                     { yylval->boolean = false;
                            return BOOL;
                          }

RTLD_NOW                  { yylval->integer = RTLD_NOW;
                            return LDFLAG;
                          }

RTLD_GLOBAL               { yylval->integer = RTLD_GLOBAL;
                            return LDFLAG;
                          }

entry-point               { yylval->integer = token_keyword;
                            return ENTRY;
                          }

with                      { yylval->integer = token_keyword;
                            return WITH;
                          }

counted                   { yylval->integer = token_keyword;
                            return COUNTER;
                          }

on                        { yylval->integer = token_keyword;
                            return ON;
                          }
default                   { yylval->integer = token_keyword;
                            return DEFAULT;
                          }

action                    { yylval->integer = token_keyword;
                            return ACTION_KW;
                          }
event                     { yylval->integer = token_keyword;
                            return EVENT;
                          }
filter                    { yylval->integer = token_keyword;
                            return FILTER;
                          }
call                      { yylval->integer = token_keyword;
                            return CALL;
                          }

constraint                { yylval->integer = token_keyword;
                            return CONSTRAINT;
                          }

minvalue                  { yylval->integer = token_keyword;
                            return MIN;
                          }
maxvalue                  { yylval->integer = token_keyword;
                            return MAX;
                          }
range                     { yylval->integer = token_keyword;
                            return RANGE;
                          }
enum                      { yylval->integer = token_keyword;
                            return ENUM;
                          }
custom                    { yylval->integer = token_keyword;
                            return CUSTOM;
                          }
read                      { yylval->integer = action_read;
                            return ACTION;
                          }
prewrite                  { yylval->integer = action_write;
                            return DEP_ACTION;
                          }
delete                    { yylval->integer = action_del_inst;
                            return DEP_ACTION;
                          }
write                     { yylval->integer = action_write;
                            return ACTION;
                          }
regexp                    { yylval->integer = token_keyword;
                            return REGEXP;
                          }
userflags                 { yylval->integer = token_keyword;
                            return FLAGS;
                          }

[[:digit:]]+              { char *endptr = NULL;
                            yylval->integer = strtoll(yytext, &endptr, 0);; 
                            return DIGIT; 
                          }
-[[:digit:]]+             { char *endptr = NULL;
                            yylval->integer = strtoll(yytext, &endptr, 0);; 
                            return DIGIT; 
                          }

[a-zA-Z0-9\-_]+           { yylval->cptr.txt = yytext;
                            yylval->cptr.length = yyleng;
                            return STRING;
                          }

\n|.                      { return yytext[0]; }

%%

void amxo_parser_create_lex(amxo_parser_t *parser) {
	yylex_init(&parser->scanner);
    yyset_extra(parser, parser->scanner);
}

void amxo_parser_destroy_lex(amxo_parser_t *parser) {
	yylex_destroy(parser->scanner);
}
