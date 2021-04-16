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

#if !defined(__AMXO_RESOLVERS_H__)
#define __AMXO_RESOLVERS_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
   @defgroup amxo_resolver Function resolvers

   @section amxo_resolvers Default Function Resolvers
   The odl parser can resolve functions for a data model functions.
   By default the odl parser supports the following function resolvers:
   - Function table (ftab)
   - Library Import
   - Automatic - the default resolver if no resolver is specified

   In the odl file, the odl parrser can be instructed to use a specific
   function resolver. After each function definition a resolver directive can
   be placed. Such a resolver directive starts with "<!" and ends with "!>".

   In bewteen the name of the resolver and optionally extra resolver data can
   be placed. The resolver data is specific for each resolver, please read
   the documetation of the resolver itself.

   Default resolver names:
   - Function table resolver - "ftab"
   - Library Import - "import"
   - Automatic - "auto"

   If no resolver directive is specified, the automatic resolver is used.

   If a resolver directuve is specified, the resolver is used, no other resolvers
   are used.

   If a data model can not be resolved, the odl parser continues and the data
   model method exists, but will not have an implementation.

   @code
     %define {
         object Greeter {
             uint32 say(%in %mandatory string message) <!ftab:dump!>;
         }
     }
   @endcode

   @subsection amxo_resolver_ftab Function Table Resolver
   To make it possible for the odl parser to use the function table resolver,
   the function table must be supplied with function pointers, prior to start
   parsing an odl file.

   To supply the function pointers use @ref amxo_resolver_ftab_add

   @subsubsection amxo_resolver_ftab_directive Function Table Reslover Directive

   @paragraph ftab_directive_format

   @code
   <!ftab:FUNCTION NAME>
   @endcode

   The function name is optional, if not given the name of the function specified
   in the odl file is used.

   @paragraph ftab_directive_examples
     Search a function implatation for the data model method "say" in the function
     table resolver, with name "say.
   @code
     %define {
         object Greeter {
             uint32 say(%in %mandatory string message) <!ftab!>;
         }
     }
   @endcode

     Search a function implatation for the data model method "say" in the function
     table resolver, with name "dump".
   @code
     %define {
         object Greeter {
             uint32 say(%in %mandatory string message) <!ftab:dump!>;
         }
     }
   @endcode

   @subsubsection amxo_resolver_ftab_example Example
   In this example, a "say" data model method is defined in the odl file
   under the object "Greeter". The odl parser can automatically bind a C
   implementation to this data model method by using the function table
   resolver.

   @paragraph ftab_example_odl_file Example odl file

   @code
     %define {
         object Greeter {
             uint32 say(%in %mandatory string message);
         }
     }
   @endcode

   @paragraph ftab_example_c_file Example C implementation

   @code{.c}
     static amxd_status_t _greeter_say(amxd_object_t *object,
                                     AMXB_UNUSED amxd_function_t *func,
                                     amxc_var_t *args,
                                     amxc_var_t *ret) {
         amxd_status_t status = amxd_status_unknown_error;
         amxc_var_t *msg = amxc_var_get_key(args, "message", AMXC_VAR_FLAG_DEFAULT);
         char *msg_txt = amxc_var_dyncast(cstring_t, msg);

         printf("==> %s says '%s'\n", from_txt, msg_txt);
         fflush(stdout);

         status = amxd_status_ok;
     exit:
         free(msg_txt);
         return status;
     }

     int create_data_model(amxd_dm_t *dm) {
         int retval = 0;
         amxo_parser_t parser;
         amxd_object_t *root = amxd_dm_get_root(dm);

         amxo_parser_init(&parser);
         amxo_parser_ftab_add(&parser, "say", AMXO_FUNC(_greeter_say));
         retval = amxo_parser_parse_file(&parser, "greeter_main.odl", root);
         amxo_parser_clean(&parser);

         return retval;
      }
   @endcode

 */

/**
   @ingroup amxo_resolver
   @brief
   Registers a function resolver.

   The odl parser provides already two function resolvers. In most cases this
   is sufficient. Extra resolvers can be registered using this function.

   The resolver must at least provide the "resolve" method, the other two
   are optional ("get" and "clean").

   The "resolve" method is called during the parsing of odl files.
   The "get" method is called when a parser is initializing to fetch all
   default configuration options for the function resolver.
   The "clean" method is called when parsing is done. In this method the
   resolver can clean-up stuff that was allocated during function resolving.

   A function resolver is typically implemented in a library and can provide
   extra API functions. The default "ftab" and "import" resolvers are providing
   extra API functions see @ref amxo_resolver_ftab_add,
   @ref amxo_resolver_ftab_remove, @ref amxo_resolver_ftab_clear,
   @ref amxo_resolver_import_open and @ref amxo_resolver_import_close_all.
   A custom function resolver can do the same, so it is easier for an
   application developer to use your function resolver.

   The name of the resolver must be unique.

   @param name of the resolver, must match the name used in the odl files
   @param resolver the resolver implementation function table

   @return
   Retruns 0 when the function is added to the function table.
 */
int amxo_register_resolver(const char* name,
                           amxo_resolver_t* resolver);

/**
   @ingroup amxo_resolver
   @brief
   Unregisters a function resolver.

   Removes the function resolver. After removing the resolver can not be used
   in any parsing anymore until it is re-added.

   It is not recommended to unregister a function resolver during parsing, for
   example in parser hooks.

   @param name of the resolver

   @return
   Retruns 0 when the function is added to the function table.
 */
int amxo_unregister_resolver(const char* name);

/**
   @ingroup amxo_resolver
   @brief
   Fetches resolver specific data for a parser instance.

   Allocates or returns the resolver specific data for the parser instance.
   Each parser instance can keep resolver specific data. This function
   is typically called in the API functions provided by the resolver.

   When there already was a data table for the function resolver available, due
   to a previous call to this function, that pointer is returned.

   A hash table is allocated, the data stored in this table is up to the
   function resolver implementation.

   @warning
   Do not delete the table. Deleting the table (or clear all data) can be done
   in the "clean" method, see @ref amxo_register_resolver.

   @param parser the odl parser instance
   @param resolver_name name of the resolver

   @return
   pointer to a hash table or NULL if allocating fails.
 */
amxc_htable_t* amxo_parser_claim_resolver_data(amxo_parser_t* parser,
                                               const char* resolver_name);

/**
   @ingroup amxo_resolver
   @brief
   Gets the resolver specific parser data.

   Returns the function resolver specific data for the given parser. If no such
   data is available it will return a NULL pointer

   This function will not allocate a table if there is no available.
   Use @ref amxo_parser_claim_resolver_data to allocate a resolver data table.

   @warning
   Do not delete the table. Deleting the table (or clear all data) can be done
   in the "clean" method, see @ref amxo_register_resolver.

   @param parser the odl parser instance
   @param resolver_name name of the resolver

   @return
   pointer to a hash table or NULL if allocating fails.
 */
amxc_htable_t* amxo_parser_get_resolver_data(amxo_parser_t* parser,
                                             const char* resolver_name);

/**
   @ingroup amxo_resolver
   @brief
   Removes the resolver specific parser data.

   Removes the resolver specific data. If the resolver allocated memory for the
   resolver specific data it is the responsibility of the resolver to free
   the allocated memory for the data before removing it from the parser context

   @param parser the odl parser instance
   @param resolver_name name of the resolver
 */
void amxo_parser_remove_resolver_data(amxo_parser_t* parser,
                                      const char* resolver_name);
/**
   @ingroup amxo_resolver
   @defgroup amxo_resolver_ftab Function Table Resolver API
 */

/**
   @ingroup amxo_resolver_ftab
   @brief
   Adds a C function to the function table.

   Adds a C function to the function table resolver. The same function can be
   added multiple times using a different name. This makes it possible
   to use the same inmplenentation for different data model methods.

   Function names can be prepended with the full object path.

   For an example see @ref amxo_resolver_ftab

   @note
   Supply function pointers to the function table resolver prior to parsing
   an odl file. By default the function table contains some parameter validation
   functions:
     - check_minimum
     - check_minimum_length
     - check_maximum
     - check_maximum_length
     - check_range
     - check_enum
   When using the same parser instance for parsing multiple odl files, there is
   no need to add the functions again, the function table is not cleared after
   parsing. The function @ref amxo_parser_clean will remove all functions from
   the table, including the default parameter validation functions.

   @param parser the odl parser instance
   @param fn_name the data model method name
   @param fn the function pointer (C implementaton), use macro @ref AMXO_FUNC

   @return
   Retruns 0 when the function is added to the function table.
 */
int amxo_resolver_ftab_add(amxo_parser_t* parser,
                           const char* fn_name,
                           amxo_fn_ptr_t fn);

/**
   @ingroup amxo_resolver_ftab
   @brief
   Removes a function from the function table

   Removes a single entry from the function table.

   @param parser the odl parser instance
   @param fn_name the data model method name

   @return
   Retruns 0 when the function is removed from the function table.
 */
int amxo_resolver_ftab_remove(amxo_parser_t* parser,
                              const char* fn_name);

/**
   @ingroup amxo_resolver_ftab
   @brief
   Removes all functions from the function table

   Removes all entries from the function table.

   @param parser the odl parser instance
 */
void amxo_resolver_ftab_clear(amxo_parser_t* parser);

/**
   @ingroup amxo_resolver
   @defgroup amxo_resolver_import Import Resolver API
 */

/**
   @ingroup amxo_resolver_import
   @brief
   Opens a shared object file (.so file)

   The import function resolver can load shared objects. During parsing
   of the odl file all "import" mentiones will call this method to open and
   load the shared object.

   Using this function it is possible to load shared objects prior to start
   parsing an odl file.

   After parsing the referenced shared objects are kept in memory. When they are
   not needed anymore they can be unloaded by using
   @ref amxo_resolver_import_close_all

   @note
   The alias must be unique, if another shared object is loaded with same alias
   the loading will fail.

   @warning
   After parsing of an odl file, all shared objects not referenced are unloaded.
   A shared object is considered referenced when at least one function is resolved
   from that shared object.

   @param parser the odl parser instance
   @param so_name file name of the shared object (can include relative or
                   absolute path).
   @param alias alias for the shared object, makes it easier to use in odl files
   @param flags dlopen flags

   @return

   Retruns 0 when the function is removed from the function table.
 */
int amxo_resolver_import_open(amxo_parser_t* parser,
                              const char* so_name,
                              const char* alias,
                              int flags);

/**
   @ingroup amxo_resolver_import
   @brief
   Unloads all loaded shared objects

   Removes all loaded shared objects from memory, unregardless they are referenced.
   This function is typically called just before exiting the application,

   @warning
   Call this method before exiting the application.
   Do not call this method when resolved functions are still being used.
 */
void amxo_resolver_import_close_all(void);


#ifdef __cplusplus
}
#endif

#endif // __AMXO_RESOLVERS_H__
