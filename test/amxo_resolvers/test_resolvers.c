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
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <inttypes.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <cmocka.h>

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_parameter.h>
#include <amxo/amxo.h>
#include <amxo/amxo_hooks.h>

#include "test_resolvers.h"

#define UNUSED __attribute__((unused))

static const char* priv_data = "PRIVATE";

static void test_resolver_get(amxo_parser_t* parser,
                              void* priv) {
    amxc_var_t* config = amxo_parser_claim_config(parser, "test-data");
    assert_ptr_not_equal(config, NULL);
    amxc_var_set(int32_t, config, 666);
    assert_ptr_equal(priv, &priv_data);
}

static amxo_fn_ptr_t test_resolver_resolve(amxo_parser_t* parser,
                                           UNUSED const char* fn_name,
                                           UNUSED const char* data,
                                           void* priv) {
    amxc_var_t* config = amxo_parser_get_config(parser, "test-data");
    assert_ptr_not_equal(config, NULL);
    assert_int_equal(amxc_var_constcast(int32_t, config), 666);
    assert_ptr_equal(priv, &priv_data);

    return NULL;
}

static void test_resolver_clean(amxo_parser_t* parser,
                                void* priv) {
    amxc_var_t* config = amxo_parser_get_config(parser, "test-data");
    assert_ptr_not_equal(config, NULL);
    assert_int_equal(amxc_var_constcast(int32_t, config), 666);
    assert_ptr_equal(priv, &priv_data);
}

static amxo_resolver_t test_resolver = {
    .get = test_resolver_get,
    .resolve = test_resolver_resolve,
    .clean = test_resolver_clean,
    .priv = &priv_data
};

static amxo_resolver_t invalid_resolver = {
    .get = test_resolver_get,
    .resolve = NULL,
    .clean = test_resolver_clean,
    .priv = &priv_data
};

void test_register_resolver(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "%define { object Test { void Func(); } }",
        "%define { object Test { void Func()<!test:data!>; } }",
        NULL
    };

    amxd_dm_init(&dm);

    assert_int_not_equal(amxo_register_resolver("ftab", &test_resolver), 0);
    assert_int_not_equal(amxo_register_resolver("auto", &test_resolver), 0);
    assert_int_not_equal(amxo_register_resolver("import", &test_resolver), 0);
    assert_int_not_equal(amxo_register_resolver("import2", NULL), 0);
    assert_int_not_equal(amxo_register_resolver("My.Resolver", &test_resolver), 0);
    assert_int_not_equal(amxo_register_resolver("test", &invalid_resolver), 0);
    assert_int_equal(amxo_register_resolver("test", &test_resolver), 0);

    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
        amxd_dm_clean(&dm);
    }

    assert_int_equal(amxo_unregister_resolver("test"), 0);
    assert_int_not_equal(amxo_unregister_resolver("test"), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_invalid_resolver_names(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    const char* odls[] = {
        "%define { object Test { void Func()<!!>; } }",
        "%define { object Test { void Func()<!:data!>; } }",
        "%define { object Test { void Func()<! :data!>; } }",
        "%define { object Test { void Func()<!invalid-resolver:data!>; } }",
        NULL
    };

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_not_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_invalid_name);
        amxd_dm_clean(&dm);
    }

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}

void test_auto_resolver_order_no_any(UNUSED void** state) {
    amxd_dm_t dm;
    amxo_parser_t parser;
    amxc_var_t order;
    const char* odls[] = {
        "%define { object Test { void Func(); } }",
        "%define { object Test { void Func()<!test:data!>; } }",
        NULL
    };

    assert_int_equal(amxo_register_resolver("test", &test_resolver), 0);

    amxd_dm_init(&dm);
    amxo_parser_init(&parser);
    amxc_var_init(&order);
    amxc_var_set_type(&order, AMXC_VAR_ID_LIST);
    amxo_parser_set_config(&parser, "auto-resolver-order", &order);
    amxc_var_clean(&order);

    for(int i = 0; odls[i] != NULL; i++) {
        assert_int_equal(amxo_parser_parse_string(&parser, odls[i], amxd_dm_get_root(&dm)), 0);
        assert_int_equal(amxo_parser_get_status(&parser), amxd_status_ok);
        amxd_dm_clean(&dm);
    }

    assert_int_equal(amxo_unregister_resolver("test"), 0);

    amxo_parser_clean(&parser);
    amxd_dm_clean(&dm);
}