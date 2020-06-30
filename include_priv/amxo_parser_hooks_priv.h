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

#if !defined(__AMXO_PARSER_HOOKS_PRIV_H__)
#define __AMXO_PARSER_HOOKS_PRIV_H__

#ifdef __cplusplus
extern "C"
{
#endif

void AMXO_PRIVATE amxo_hooks_start(amxo_parser_t *parser);

void AMXO_PRIVATE amxo_hooks_end(amxo_parser_t *parser);

void AMXO_PRIVATE amxo_hooks_start_include(amxo_parser_t *parser,
                                           const char *file);

void AMXO_PRIVATE amxo_hooks_end_include(amxo_parser_t *parser,
                                         const char *file);

void AMXO_PRIVATE amxo_hooks_start_section(amxo_parser_t *parser,
                                           int section_id);

void AMXO_PRIVATE amxo_hooks_end_section(amxo_parser_t *parser,
                                         int section_id);

void AMXO_PRIVATE amxo_hooks_set_config(amxo_parser_t *parser,
                                        const char *name,
                                        amxc_var_t *value);

void AMXO_PRIVATE amxo_hooks_create_object(amxo_parser_t *parser,
                                           const char *name,
                                           int64_t attr_bitmask,
                                           amxd_object_type_t type);

void AMXO_PRIVATE amxo_hooks_add_instance(amxo_parser_t *parser,
                                          uint32_t index,
                                          const char *name);

void AMXO_PRIVATE amxo_hooks_select_object(amxo_parser_t *parser,
                                           const char *path);

void AMXO_PRIVATE amxo_hooks_end_object(amxo_parser_t *parser);

void AMXO_PRIVATE amxo_hooks_add_param(amxo_parser_t *parser,
                                       const char *name,
                                       int64_t attr_bitmask,
                                       uint32_t type);

void AMXO_PRIVATE amxo_hooks_set_param(amxo_parser_t *parser,
                                       amxc_var_t *value);

void AMXO_PRIVATE amxo_hooks_end_param(amxo_parser_t *parser);

void AMXO_PRIVATE amxo_hooks_add_func(amxo_parser_t *parser,
                                      const char *name,
                                      int64_t attr_bitmask,
                                      uint32_t type);

void AMXO_PRIVATE amxo_hooks_end_func(amxo_parser_t *parser);

void AMXO_PRIVATE amxo_hooks_add_func_arg(amxo_parser_t *parser,
                                          const char *name,
                                          int64_t attr_bitmask,
                                          uint32_t type,
                                          amxc_var_t *def_value);

#ifdef __cplusplus
}
#endif

#endif // __AMXO_PARSER_PRIV_H__
