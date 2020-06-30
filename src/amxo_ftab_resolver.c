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

#define _GNU_SOURCE
#include <sys/resource.h>

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_common.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_action.h>
#include <amxo/amxo.h>

#include "amxo_assert.h"
#include "amxo_parser_priv.h"

typedef struct _amxo_ftab_fn {
    amxc_htable_it_t hit;
    amxo_fn_ptr_t fn;
} amxo_ftab_fn_t;

static void amxo_resolver_ftab_defaults(amxo_parser_t *parser,
                                        AMXO_UNUSED void *priv) {
    amxo_resolver_ftab_add(parser,
                           "check_minimum",
                           AMXO_FUNC(amxd_action_param_check_minimum));
    amxo_resolver_ftab_add(parser,
                           "check_maximum",
                           AMXO_FUNC(amxd_action_param_check_maximum));
    amxo_resolver_ftab_add(parser,
                           "check_range",
                           AMXO_FUNC(amxd_action_param_check_range));
    amxo_resolver_ftab_add(parser,
                           "check_enum",
                           AMXO_FUNC(amxd_action_param_check_enum));
}

static amxo_fn_ptr_t amxo_resolver_ftab(amxo_parser_t *parser,
                                        const char *fn_name,
                                        const char *data,
                                        AMXO_UNUSED void *priv) {
    amxo_fn_ptr_t fn = NULL;
    amxo_ftab_fn_t *ftab_fn = NULL;
    amxc_htable_t *ftab_data = NULL;
    amxc_htable_it_t *it = NULL;

    ftab_data = amxo_parser_get_resolver_data(parser, "ftab");
    when_null(ftab_data, exit);

    if((data != NULL) && (data[0] != 0)) {
        it = amxc_htable_get(ftab_data, data);
    } else {
        it = amxc_htable_get(ftab_data, fn_name);
    }
    when_null(it, exit);

    ftab_fn = amxc_htable_it_get_data(it, amxo_ftab_fn_t, hit);
    fn = ftab_fn->fn;

exit:
    return fn;
}

static void amxo_resolver_ftab_clean(amxo_parser_t *parser,
                                     AMXO_UNUSED void *priv) {
    amxc_htable_t *ftab_data = NULL;
    ftab_data = amxo_parser_get_resolver_data(parser, "ftab");
    amxc_htable_clean(ftab_data, amxo_ftab_fn_free);
}

static bool amxo_ftab_func_name_is_valid(const char *name) {
    bool retval = false;
    bool is_allnum = true;
    when_str_empty(name, exit);
    when_true(isalpha(name[0]) == 0, exit);

    for(int i = 0; name[i] != 0; i++) {
        if(isalpha(name[i]) != 0) {
            is_allnum = false;
        } else {
            if(isdigit(name[i]) == 0) {
                is_allnum = false;
                if((name[i] != '_') &&
                   (name[i] != '-')) {
                    goto exit;
                }
            }
        }
    }

    retval = is_allnum ? false : true;

exit:
    return retval;
}


void AMXO_PRIVATE amxo_ftab_fn_free(AMXO_UNUSED const char *key,
                                    amxc_htable_it_t *it) {
    amxo_ftab_fn_t *ftab_fn = amxc_htable_it_get_data(it, amxo_ftab_fn_t, hit);
    free(ftab_fn);
}

int amxo_resolver_ftab_add(amxo_parser_t *parser,
                           const char *fn_name,
                           amxo_fn_ptr_t fn) {
    int retval = -1;
    amxo_ftab_fn_t *ftab_fn = NULL;
    amxc_htable_t *ftab_data = NULL;
    when_null(parser, exit);
    when_null(fn, exit);
    when_true(!amxo_ftab_func_name_is_valid(fn_name), exit);

    ftab_data = amxo_parser_claim_resolver_data(parser, "ftab");
    when_null(ftab_data, exit);
    when_true(amxc_htable_contains(ftab_data, fn_name), exit);

    ftab_fn = calloc(1, sizeof(amxo_ftab_fn_t));
    when_null(ftab_fn, exit);

    ftab_fn->fn = fn;
    amxc_htable_insert(ftab_data, fn_name, &ftab_fn->hit);

    retval = 0;

exit:
    return retval;
}

int amxo_resolver_ftab_remove(amxo_parser_t *parser,
                              const char *fn_name) {
    int retval = -1;
    amxc_htable_t *ftab_data = NULL;
    amxc_htable_it_t *it = NULL;
    when_null(parser, exit);
    when_str_empty(fn_name, exit);
    ftab_data = amxo_parser_claim_resolver_data(parser, "ftab");
    when_null(ftab_data, exit);

    it = amxc_htable_get(ftab_data, fn_name);
    if(it != NULL) {
        amxc_htable_it_clean(it, amxo_ftab_fn_free);
    }
    retval = 0;

exit:
    return retval;
}

void amxo_resolver_ftab_clear(amxo_parser_t *parser) {
    when_null(parser, exit);
    amxo_resolver_ftab_clean(parser, NULL);

exit:
    return;
}

static amxo_resolver_t ftab = {
    .get = amxo_resolver_ftab_defaults,
    .resolve = amxo_resolver_ftab,
    .clean = amxo_resolver_ftab_clean
};

AMXO_CONSTRUCTOR(110) static void amxo_ftab_init(void) {
    amxo_register_resolver("ftab", &ftab);
}

AMXO_DESTRUCTOR(110) static void amxo_ftab_cleanup(void) {
    amxo_unregister_resolver("ftab");
}
