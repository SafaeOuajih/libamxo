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

#ifndef __TEST_INVALID_ODL_H__
#define __TEST_INVALID_ODL_H__

void test_invalid_object_attrs(void **state);
void test_invalid_param_attrs(void **state);
void test_invalid_func_attrs(void **state);
void test_add_inst_on_singelton(void **state);
void test_duplicate_inst_index(void **state);
void test_duplicate_inst_name(void **state);
void test_invalid_inst_name(void **state);
void test_duplicate_obj_name(void **state);
void test_invalid_obj_name(void **state);
void test_duplicate_param_name(void **state);
void test_duplicate_param_name_with_counter(void **state);
void test_invalid_param_name(void **state);
void test_invalid_func_name(void **state);
void test_duplicate_func_arg_name(void **state);
void test_select_none_existing_obj(void **state);
void test_select_none_existing_param(void **state);
void test_invalid_param_value(void **state);
void test_invalid_param_value_validate(void **state);
void test_invalid_resolvers(void **state);
void test_not_existing_entry_point(void **state);
void test_invalid_parameter_actions(void **state);
void test_not_resolved_action(void **state);
void test_add_not_existing_mib(void **state);
void test_add_mib_with_duplicates(void **state);
void test_invalid_regexp_in_filter(void **state);
void test_invalid_key_params(void **state);

#endif // __TEST_INVALID_ODL_H__