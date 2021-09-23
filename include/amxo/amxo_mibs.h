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

#if !defined(__AMXO_MIBS_H__)
#define __AMXO_MIBS_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef bool (* amxo_evaluate_expr_fn_t) (amxd_object_t* object,
                                          amxp_expr_t* expr);


/**
   @defgroup amxo_parser_mibs ODL Parser MIBS
   @ingroup amxo_parser

   Ambiorix ODL parser mibs API

   MIBS are data mode extention objects, A MIB can be applied to any
   object in the data model. The object get extended with the parameters,
   functions and objects defined in the MIB.

   MIBS can be defined in separate ODL files. Each MIB odl file contains
   exactly one MIB definition. The file must have the same name as the MIB
   definition.

   The MIB odl files can contain on the first line a expression definition.

   The MIBS are not loaded, until they are needed. Loading the MIBS and applying
   them on objects in the data model can be done using @ref amxo_parser_apply_mib
   or @ref amxo_parser_apply_mibs
 */

/**
   @ingroup amxo_parser_mibs
   @brief
   Scans a directory for MIB odl files

   Each odl file found in the given path must contain exactly one MIB
   definition. The name of the MIB and the name of the file must be the
   same.

   The MIB odl definition can contain at the first line an expression.
   The expression must be added as a comment and must start with "expr:"

   @param parser the odl parser instance
   @param path the path that needs to be scanned

   @return
   Returns 0 when success, any other value indicates failure.
 */
int amxo_parser_scan_mib_dir(amxo_parser_t* parser,
                             const char* path);

/**
   @ingroup amxo_parser_mibs
   @brief
   Scans multiple directories for MIB odl files

   The directories must be provided as a variant containing a list of
   variants, where each variant contains a path.

   @param parser the odl parser instance
   @param dirs a variant containing a list of variants, each containing a path.

   @return
   Returns 0 when success, any other value indicates failure.
 */
int amxo_parser_scan_mib_dirs(amxo_parser_t* parser,
                              amxc_var_t* dirs);

/**
   @ingroup amxo_parser_mibs
   @brief
   Get full path and file name of odl file describing a mib

   After scanning the mib directories using @ref amxo_parser_scan_mib_dir or
   @ref amxo_parser_scan_mib_dirs, this function returns the file name for
   a mib definition.

   The mib can be loaded using @ref amxo_parser_parse_file

   @param parser the odl parser instance
   @param mib_name name of the mib.

   @return
   Returns a string containing the full path and file name of the file containing
   the mib definition or NULL when no file found
 */
const char* amxo_parser_get_mib_file(amxo_parser_t* parser,
                                     const char* mib_name);

/**
   @ingroup amxo_parser_mibs
   @brief
   Loads the mib definition.

   Mibs can be loaded by mib name after @ref amxo_parser_scan_mib_dir or
   @ref amxo_parser_scan_mib_dirs was called.

   This function calls:
   - @ref amxo_parser_get_mib_file
   - @ref amxo_parser_parse_file

   If the mib was already loaded nothing is done.

   @param parser the odl parser instance
   @param dm data model for which the mib must be loaded
   @param mib_name name of the mib.

   @return
   Returns 0 when success, any other value indicates failure.
 */
int amxo_parser_load_mib(amxo_parser_t* parser,
                         amxd_dm_t* dm,
                         const char* mib_name);
/**
   @ingroup amxo_parser_mibs
   @brief
   Unconditionally applies a MIB to a data model object

   Loads the MIB odl file, if not already loaded and applies the MIB on
   the provided data model object. If an expression was set in the MIB odl
   file, the expression is ignored. The MIB is unconditionally applied on the
   data model object.

   @param parser the odl parser instance
   @param object the data model object
   @param mib_name the name of the mib.

   @return
   Returns 0 when success, any other value indicates failure.
 */
int amxo_parser_apply_mib(amxo_parser_t* parser,
                          amxd_object_t* object,
                          const char* mib_name);

/**
   @ingroup amxo_parser_mibs
   @brief
   Adds zero, one or more MIBs to a data model object.

   This function loops over all known MIB odl files, and will apply
   each MIB to the provided data model object if the object is
   matching the MIB expression.

   When an expression evaluation function is provided, the MIB expression
   will be passed to that function together with the data model object.
   When the function returns true the MIB will be applied on the data model
   object. If the MIB does not have an expression defined, the name of
   the MIB is used as an expression.

   No MIBs are applied to the data model object if no expression evaluation
   function is provided.

   To unconditionally apply a single MIB to a data model object use
   @ref amxo_parser_apply_mib

   @note
   Before calling this function one or more directories must be scanned for
   MIB odl files. Scanning a directory can be done using
   @ref amxo_parser_scan_mib_dir or @ref amxo_parser_scan_mib_dirs

   @param parser the odl parser instance
   @param object the data model object
   @param fn function that evaluates the MIB expression.

   @return
   Total number of MIBs added on the object
 */
int amxo_parser_add_mibs(amxo_parser_t* parser,
                         amxd_object_t* object,
                         amxo_evaluate_expr_fn_t fn);

/**
   @ingroup amxo_parser_mibs
   @brief
   Removes zero, one or more MIBs from a data model object.

   This function loops over all known MIB odl files, and will remove
   each MIB from the provided data model object if the MIB was added to the
   object and the object is not mathching the MIB expression any more.

   When an expression evaluation function is provided, the MIB expression
   will be passed to that function together with the data model object.
   When the function returns false the MIB will be removed from the data model
   object. If the MIB does not have an expression defined, the name of
   the MIB is used as an expression.

   No MIBs are removed from the data model object if no expression evaluation
   function is provided.

   To unconditionally remove a single MIB from a data model object use the
   data model function amxd_object_remove_mib.

   @note
   Before calling this function one or more directories must be scanned for
   MIB odl files. Scanning a directory can be done using
   @ref amxo_parser_scan_mib_dir or @ref amxo_parser_scan_mib_dirs

   @param parser the odl parser instance
   @param object the data model object
   @param fn function that evaluates the MIB expression.

   @return
   Total number of MIBs removed from the object
 */
int amxo_parser_remove_mibs(amxo_parser_t* parser,
                            amxd_object_t* object,
                            amxo_evaluate_expr_fn_t fn);
/**
   @ingroup amxo_parser_mibs
   @brief
   Applies zero, one or more MIBs to a data model object.

   This function loops over all known MIB odl files, and will add
   each MIB to the provided data model object if the object matches the MIB
   expression or removes the MIB from the data model object of the object
   is not matching the expression anymore.

   When an expression evaluation function is provided, the MIB expression
   will be passed to that function together with the data model object.
   When the function returns true the MIB will be added on the data model
   object otherwise removed. If the MIB ODL files does not have an expression
   defined, the name of the MIB is used as an expression.

   No MIBs are applied if no expression evaluation function is provided.

   @param parser the odl parser instance
   @param object the data model object
   @param fn function that evaluates the MIB expression.

   @return
   Total number of MIBs applied on the object
 */
int amxo_parser_apply_mibs(amxo_parser_t* parser,
                           amxd_object_t* object,
                           amxo_evaluate_expr_fn_t fn);

#ifdef __cplusplus
}
#endif

#endif // __AMXO_MIBS_H__

