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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/resource.h>

#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <libgen.h>
#include <ctype.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxo/amxo.h>

#include "amxo_parser_priv.h"

static void amxo_resolver_auto_defaults(amxo_parser_t* parser,
                                        UNUSED void* priv) {
    amxc_var_t* config = amxo_parser_claim_config(parser, "auto-resolver-order");
    amxc_var_set_type(config, AMXC_VAR_ID_LIST);

    amxc_var_add(cstring_t, config, "ftab");
    amxc_var_add(cstring_t, config, "import");
    amxc_var_add(cstring_t, config, "*");

    return;
}

static amxo_fn_ptr_t amxo_resolver_auto(amxo_parser_t* parser,
                                        const char* fn_name,
                                        UNUSED const char* data,
                                        UNUSED void* priv) {
    int retval = -1;
    amxc_var_t* config = amxo_parser_claim_config(parser, "auto-resolver-order");
    const amxc_llist_t* order = amxc_var_constcast(amxc_llist_t, config);
    amxc_htable_t* resolvers = amxo_parser_get_resolvers();
    amxc_var_t used_resolvers;
    const char* name = NULL;

    amxc_var_init(&used_resolvers);
    amxc_var_set_type(&used_resolvers, AMXC_VAR_ID_HTABLE);
    amxc_var_add_key(bool, &used_resolvers, "auto", true);
    amxc_llist_for_each(it, order) {
        name = amxc_var_constcast(cstring_t, amxc_var_from_llist_it(it));
        if(name[0] == '*') {
            break;
        }
        retval = amxo_parser_resolve(parser, name, fn_name, NULL);
        when_true(retval == 0, exit);
        amxc_var_add_key(bool, &used_resolvers, name, true);
    }

    when_true((name != NULL && name[0] != '*'), exit);

    amxc_htable_for_each(hit, resolvers) {
        name = amxc_htable_it_get_key(hit);
        if(amxc_var_get_key(&used_resolvers, name, AMXC_VAR_FLAG_DEFAULT)) {
            continue;
        }
        retval = amxo_parser_resolve(parser, name, fn_name, NULL);
        when_true(retval == 0, exit);
    }

exit:
    amxc_var_clean(&used_resolvers);
    if(retval == 0) {
        return parser->resolved_fn;
    } else {
        return NULL;
    }
}

static amxo_resolver_t res_auto = {
    .hit = { .ait = NULL, .key = NULL, .next = NULL },
    .get = amxo_resolver_auto_defaults,
    .resolve = amxo_resolver_auto,
    .clean = NULL,
    .priv = NULL
};

CONSTRUCTOR_LVL(110) static void amxo_auto_init(void) {
    amxo_register_resolver("auto", &res_auto);
}

DESTRUCTOR_LVL(110) static void amxo_auto_cleanup(void) {
    amxo_unregister_resolver("auto");
}
