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

#if !defined(__AMXO_TYPES_H__)
#define __AMXO_TYPES_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _amxo_parser amxo_parser_t;

typedef ssize_t (* amxo_reader_t) (amxo_parser_t* parser, void* buf, size_t max_size);

typedef void (* amxo_fn_ptr_t)(void);

/**
   @ingroup amxo_resolver
   @brief
   Function ponter caster macro.

   @param x C function pointer
 */
#define AMXO_FUNC(x) ((amxo_fn_ptr_t) x)

typedef void (* amxo_res_get_default_t) (amxo_parser_t* parser, void* priv);

typedef amxo_fn_ptr_t (* amxo_res_resolve_fn_t) (amxo_parser_t* parser,
                                                 const char* fn_name,
                                                 const char* data,
                                                 void* priv);

typedef void (* amxo_res_clean_fn_t) (amxo_parser_t* parser,
                                      void* priv);

typedef struct _amxo_resolver {
    amxc_htable_it_t hit;
    amxo_res_get_default_t get;
    amxo_res_resolve_fn_t resolve;
    amxo_res_clean_fn_t clean;
    void* priv;
} amxo_resolver_t;

typedef void (* amxo_start_end_t) (amxo_parser_t* parser);

typedef void (* amxo_include_t) (amxo_parser_t* parser,
                                 const char* file);

typedef void (* amxo_section_t) (amxo_parser_t* parser,
                                 int section_id);

typedef void (* amxo_set_config_t) (amxo_parser_t* parser,
                                    const char* option,
                                    amxc_var_t* value);

typedef void (* amxo_create_object_t) (amxo_parser_t* parser,
                                       amxd_object_t* parent,
                                       const char* name,
                                       int64_t attr_bitmask,
                                       amxd_object_type_t type);

typedef void (* amxo_add_instance_t) (amxo_parser_t* parser,
                                      amxd_object_t* parent,
                                      uint32_t index,
                                      const char* name);

typedef void (* amxo_select_object_t) (amxo_parser_t* parser,
                                       amxd_object_t* parent,
                                       const char* path);

typedef void (* amxo_end_object_t) (amxo_parser_t* parser,
                                    amxd_object_t* object);

typedef void (* amxo_add_param_func_t) (amxo_parser_t* parser,
                                        amxd_object_t* object,
                                        const char* name,
                                        int64_t attr_bitmask,
                                        uint32_t type);

typedef void (* amxo_set_param_t) (amxo_parser_t* parser,
                                   amxd_object_t* object,
                                   amxd_param_t* param,
                                   amxc_var_t* value);

typedef void (* amxo_end_param_t) (amxo_parser_t* parser,
                                   amxd_object_t* object,
                                   amxd_param_t* param);
typedef void (* amxo_end_func_t) (amxo_parser_t* parser,
                                  amxd_object_t* object,
                                  amxd_function_t* function);

typedef void (* amxo_add_func_arg_t) (amxo_parser_t* parser,
                                      amxd_object_t* object,
                                      amxd_function_t* func,
                                      const char* name,
                                      int64_t attr_bitmask,
                                      uint32_t type,
                                      amxc_var_t* def_value);

typedef struct _amxo_hooks {
    amxc_llist_it_t it;
    amxo_start_end_t start;
    amxo_start_end_t end;
    amxo_include_t start_include;
    amxo_include_t end_include;
    amxo_set_config_t set_config;
    amxo_section_t start_section;
    amxo_section_t end_section;
    amxo_create_object_t create_object;
    amxo_add_instance_t add_instance;
    amxo_select_object_t select_object;
    amxo_end_object_t end_object;
    amxo_add_param_func_t add_param;
    amxo_set_param_t set_param;
    amxo_end_param_t end_param;
    amxo_add_param_func_t add_func;
    amxo_add_func_arg_t add_func_arg;
    amxo_end_func_t end_func;
} amxo_hooks_t;

typedef enum _amxo_reason {
    AMXO_START,
    AMXO_STOP,
    AMXO_ODL_LOADED
} amxo_reason_t;

typedef enum _amxo_con_type {
    AMXO_BUS,
    AMXO_LISTEN,
    AMXO_CUSTOM,
} amxo_con_type_t;

typedef int (* amxo_entry_point_t) (int reason,
                                    amxd_dm_t* dm,
                                    amxo_parser_t* parser);

typedef struct _amxo_entry {
    amxc_llist_it_t it;
    amxo_entry_point_t entry_point;
} amxo_entry_t;

typedef void (* amxo_fd_read_t) (int fd, void* priv);

typedef struct _amxo_connection {
    amxc_llist_it_t it;
    char* uri;
    int fd;
    amxo_fd_read_t reader;
    void* el_data;
    void* priv;
    amxo_con_type_t type;
} amxo_connection_t;

/**
   @ingroup amxo_parser
   @brief
   The ODL parser structure

   The structure can be allocated and initialize on the heap using
   @ref amxo_parser_new or initialized on the stack using @ref amxo_parser_init

   It is not the intention to access the members of this structure directly,
   an amxo_parser_t pointer should be handled as an opaque handle.

   Use the API functions to work with the amxo_parser_t instance.
 */
struct _amxo_parser {
    void* scanner;                /**< Flex scanner */
    int fd;                       /**< The file descriptor, used to read from the odl file
                                       @ref amxo_parser_parse_file or @ref amxo_parser_parse_fd
                                   */
    amxc_var_t config;            /** An variant containing a htable with configuration options
                                      Can be used by resolvers or the parser itself.
                                   */
    amxc_llist_t global_config;   /**< List of global config settings, contains the names of the
                                       global config options
                                   */
    amxc_rbuffer_t rbuffer;       /**< Ring buffer, used to parse text */
    amxo_reader_t reader;         /**< Reader function */
    amxd_status_t status;         /**< Parser status, @ref amxo_parser_get_status */
    amxc_string_t msg;            /**< Parser message, @ref amxo_parser_get_message */

    amxc_astack_t object_stack;   /**< Data model object stack, internally used by odl parser */
    amxd_object_t* object;        /**< Data model current object, internally used by odl parser */
    amxd_param_t* param;          /**< Data model current parameter, internally used by odl parser */
    amxd_function_t* func;        /**< Data model current function, internally used by odl parser */
    amxc_var_t* data;             /**< Some data, internally used by odl parser */

    amxo_fn_ptr_t resolved_fn;    /**< Filled with function pointer, internally used by function resolver */
    amxc_htable_t* resolvers;     /**< Resolvers data, internally used by function resolver */

    amxc_llist_t* connections;    /**< List of connections */
    amxc_llist_t* listeners;      /**< List of listen sockets */
    amxc_llist_t* entry_points;   /**< List of entry points that needs to be called */
    amxc_llist_t* hooks;          /**< List of parser hooks */
    amxc_var_t* post_includes;    /**< List of include files that needs to be loaded after entry-points are called*/

    amxc_var_t* include_stack;    /**< Used for recursive include tracking */

    const char* file;
    uint32_t line;

    amxc_htable_t mibs;
    amxc_lstack_t event_stack;    /**< */
};

#ifdef __cplusplus
}
#endif

#endif // __AMXO_TYPES_H__

