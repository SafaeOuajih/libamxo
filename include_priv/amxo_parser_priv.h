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

#if !defined(__AMXO_PARSER_PRIV_H__)
#define __AMXO_PARSER_PRIV_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <amxc/amxc_macros.h>

#define when_true_status(x, l, c) if(x) { c; goto l; }

enum amxo_parser_tokens_t
{
    token_include,
    token_optional_include,
    token_conditional_include,
    token_post_include,
    token_import,
    token_config,
    token_define,
    token_populate,
    token_object,
    token_mib,
    token_keyword,
    token_eof,
    token_requires,
};

typedef enum _amxo_parser_attr {
    attr_readonly,
    attr_persistent,
    attr_private,
    attr_template,
    attr_instance,
    attr_variable,
    attr_in,
    attr_out,
    attr_mandatory,
    attr_strict,
    attr_key,
    attr_unique,
    attr_protected
} amxo_parser_attr_t;

typedef enum _amxo_action {
    amxo_action_invalid = -1,
    action_read = 0,
    action_write,
    action_validate,
    action_list,
    action_describe,
    action_add_inst,
    action_del_inst,
    action_destroy,
    action_max = action_destroy
} amxo_action_t;

typedef enum _event_id {
    event_none,
    event_instance_add,
    event_object_change
} event_id_t;

typedef struct _event {
    event_id_t id;
    amxc_var_t data;
    char* path;
    amxc_llist_it_t it;
} event_t;

typedef struct _amxo_txt {
    char* txt;
    int length;
} amxo_txt_t;

typedef struct _amxo_txt_regexp {
    char* txt;
    bool is_regexp;
} amxo_txt_regexp_t;

typedef struct _amxo_res_data {
    amxc_htable_it_t hit;
    amxc_htable_t data;
} amxo_res_data_t;

void PRIVATE amxo_parser_free_event(amxc_llist_it_t* it);

void PRIVATE amxo_ftab_fn_free(const char* key,
                               amxc_htable_it_t* it);

void PRIVATE amxo_parser_del_mib_info(const char* key,
                                      amxc_htable_it_t* it);

ssize_t PRIVATE amxo_parser_fd_reader(amxo_parser_t* parser,
                                      void* buf,
                                      size_t max_size);

int PRIVATE amxo_parser_parse_file_impl(amxo_parser_t* parser,
                                        const char* file_path,
                                        amxd_object_t* object);

void PRIVATE amxo_parser_child_init(amxo_parser_t* parser);

void PRIVATE amxo_parser_create_lex(amxo_parser_t* parser);
void PRIVATE amxo_parser_destroy_lex(amxo_parser_t* parser);

void PRIVATE amxo_parser_msg(amxo_parser_t* parser, const char* format, ...) \
    __attribute__ ((format(printf, 2, 3)));

int PRIVATE amxo_parser_printf(amxo_parser_t* parser, const char* format, ...) \
    __attribute__ ((format(printf, 2, 3)));

int PRIVATE amxo_parser_set_config_internal(amxo_parser_t* parser,
                                            const char* name,
                                            amxc_var_t* value);

bool PRIVATE amxo_parser_find(amxo_parser_t* parser,
                              const amxc_llist_t* dirs,
                              const char* file_path,
                              char** full_path);

bool PRIVATE amxo_parser_check_attr(amxo_parser_t* pctx,
                                    int64_t attributes,
                                    int64_t bitmask);

bool PRIVATE amxo_parser_set_param_attrs(amxo_parser_t* pctx,
                                         uint64_t attr,
                                         bool enable);
bool PRIVATE amxo_parser_set_param_flags(amxo_parser_t* pctx);

bool PRIVATE amxo_parser_set_object_attrs(amxo_parser_t* pctx,
                                          uint64_t attr,
                                          bool enable);

int PRIVATE amxo_parser_create_object(amxo_parser_t* pctx,
                                      const char* name,
                                      int64_t attr_bitmask,
                                      amxd_object_type_t type);

bool PRIVATE amxo_parser_add_instance(amxo_parser_t* pctx,
                                      const char* parent,
                                      uint32_t index,
                                      const char* name);

bool PRIVATE amxo_parser_push_object(amxo_parser_t* pctx,
                                     const char* name);

bool PRIVATE amxo_parser_pop_object(amxo_parser_t* pctx);

bool PRIVATE amxo_parser_push_param(amxo_parser_t* pctx,
                                    const char* name,
                                    int64_t attr_bitmask,
                                    uint32_t type);

int PRIVATE amxo_parser_set_param(amxo_parser_t* pctx,
                                  const char* name,
                                  amxc_var_t* value);

bool PRIVATE amxo_parser_pop_param(amxo_parser_t* pctx);

int PRIVATE amxo_parser_push_func(amxo_parser_t* pctx,
                                  const char* name,
                                  int64_t attr_bitmask,
                                  uint32_t type);

bool PRIVATE amxo_parser_set_function_flags(amxo_parser_t* pctx);

void PRIVATE amxo_parser_pop_func(amxo_parser_t* pctx);

bool PRIVATE amxo_parser_add_arg(amxo_parser_t* pctx,
                                 const char* name,
                                 int64_t attr_bitmask,
                                 uint32_t type,
                                 amxc_var_t* def_value);

bool PRIVATE amxo_parser_set_counter(amxo_parser_t* pctx,
                                     const char* param_name);

int PRIVATE amxo_parser_subscribe_path(amxo_parser_t* pctx,
                                       const char* event,
                                       bool event_is_regexp,
                                       const char* path,
                                       bool path_is_regexp);

int PRIVATE amxo_parser_subscribe(amxo_parser_t* pctx,
                                  const char* event,
                                  bool event_is_regexp,
                                  const char* full_expr);

bool PRIVATE amxo_parser_subscribe_item(amxo_parser_t* pctx);

int PRIVATE amxo_parser_add_post_include(amxo_parser_t* pctx,
                                         const char* file_path);
int PRIVATE amxo_parser_include(amxo_parser_t* pctx, const char* file_path);

amxc_htable_t* PRIVATE amxo_parser_get_resolvers(void);

int PRIVATE amxo_parser_resolve_internal(amxo_parser_t* parser,
                                         const char* fn_name,
                                         amxo_fn_type_t type,
                                         const char* data);

void PRIVATE amxo_resolver_import_clean(amxo_parser_t* parser,
                                        void* priv);

int PRIVATE amxo_parser_resolve(amxo_parser_t* parser,
                                const char* resolver_name,
                                const char* func_name,
                                amxo_fn_type_t type,
                                const char* data);

void PRIVATE amxo_parser_clean_resolvers(amxo_parser_t* parser);
void PRIVATE amxo_parser_init_resolvers(amxo_parser_t* parser);

int PRIVATE amxo_parser_call_entry_point(amxo_parser_t* pctx,
                                         const char* lib_name,
                                         const char* fn_name);

bool PRIVATE amxo_parser_set_data_option(amxo_parser_t* pctx,
                                         const char* key,
                                         amxc_var_t* value);

int PRIVATE amxo_parser_set_action(amxo_parser_t* pctx,
                                   amxo_action_t action);

amxo_action_t PRIVATE amxo_parser_get_action_id(amxo_parser_t* pctx,
                                                const char* action_name);

char* PRIVATE amxo_parser_build_import_resolver_data(const char* function,
                                                     const char* library);

bool PRIVATE amxo_parser_add_mib(amxo_parser_t* pctx,
                                 const char* mib_name);

void PRIVATE amxo_parser_connection_free(amxc_llist_it_t* it);

bool PRIVATE amxo_parser_no_resolve(amxo_parser_t* parser);

bool PRIVATE amxo_parser_check_config(amxo_parser_t* pctx,
                                      const char* path,
                                      const char* check);
#ifdef __cplusplus
}
#endif

#endif // __AMXO_PARSER_PRIV_H__

