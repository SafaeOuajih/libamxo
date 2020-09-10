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

#include <amxc/amxc.h>
#include <amxp/amxp_signal.h>
#include <amxd/amxd_dm.h>
#include <amxo/amxo.h>
#include <amxo/amxo_hooks.h>

#include "amxo_assert.h"
#include "amxo_parser_priv.h"
#include "amxo_parser_hooks_priv.h"
#include "amxo_parser.tab.h"

void amxo_hooks_start(amxo_parser_t *parser) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->start != NULL) {
            hook->start(parser);
        }
    }
}

void amxo_hooks_end(amxo_parser_t *parser) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end != NULL) {
            hook->end(parser);
        }
    }
}

void amxo_hooks_start_include(amxo_parser_t *parser, const char *file) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->start_include != NULL) {
            hook->start_include(parser, file);
        }
    }
}

void amxo_hooks_end_include(amxo_parser_t *parser, const char *file) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end_include != NULL) {
            hook->end_include(parser, file);
        }
    }
}

void amxo_hooks_start_section(amxo_parser_t *parser, int section_id) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->start_section != NULL) {
            hook->start_section(parser, section_id);
        }
    }
}

void amxo_hooks_end_section(amxo_parser_t *parser, int section_id) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end_section != NULL) {
            hook->end_section(parser, section_id);
        }
    }
}

void amxo_hooks_set_config(amxo_parser_t *parser,
                           const char *name,
                           amxc_var_t *value) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->set_config != NULL) {
            hook->set_config(parser, name, value);
        }
    }
}

void amxo_hooks_create_object(amxo_parser_t *parser,
                              const char *name,
                              int64_t attr_bitmask,
                              amxd_object_type_t type) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->create_object != NULL) {
            hook->create_object(parser,
                                parser->object,
                                name,
                                attr_bitmask,
                                type);
        }
    }
}

void amxo_hooks_add_instance(amxo_parser_t *parser,
                             uint32_t index,
                             const char *name) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->add_instance != NULL) {
            hook->add_instance(parser,
                               parser->object,
                               index,
                               name);
        }
    }
}

void amxo_hooks_select_object(amxo_parser_t *parser,
                              const char *path) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->select_object != NULL) {
            hook->select_object(parser,
                                parser->object,
                                path);
        }
    }
}

void amxo_hooks_end_object(amxo_parser_t *parser) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end_object != NULL) {
            hook->end_object(parser,
                             parser->object);
        }
    }
}

void amxo_hooks_add_param(amxo_parser_t *parser,
                          const char *name,
                          int64_t attr_bitmask,
                          uint32_t type) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->add_param != NULL) {
            hook->add_param(parser,
                            parser->object,
                            name,
                            attr_bitmask,
                            type);
        }
    }
}

void amxo_hooks_set_param(amxo_parser_t *parser,
                          amxc_var_t *value) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->set_param != NULL) {
            hook->set_param(parser,
                            parser->object,
                            parser->param,
                            value);
        }
    }
}

void amxo_hooks_end_param(amxo_parser_t *parser) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end_param != NULL) {
            hook->end_param(parser,
                            parser->object,
                            parser->param);
        }
    }
}

void amxo_hooks_add_func(amxo_parser_t *parser,
                         const char *name,
                         int64_t attr_bitmask,
                         uint32_t type) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->add_func != NULL) {
            hook->add_func(parser,
                           parser->object,
                           name,
                           attr_bitmask,
                           type);
        }
    }
}

void amxo_hooks_end_func(amxo_parser_t *parser) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->end_func != NULL) {
            hook->end_func(parser,
                           parser->object,
                           parser->func);
        }
    }
}

void AMXO_PRIVATE amxo_hooks_add_func_arg(amxo_parser_t *parser,
                                          const char *name,
                                          int64_t attr_bitmask,
                                          uint32_t type,
                                          amxc_var_t *def_value) {
    amxc_llist_for_each(it, parser->hooks) {
        amxo_hooks_t *hook = amxc_llist_it_get_data(it, amxo_hooks_t, it);
        if(hook->add_func_arg != NULL) {
            hook->add_func_arg(parser,
                               parser->object,
                               parser->func,
                               name,
                               attr_bitmask,
                               type,
                               def_value);
        }
    }
}

int amxo_parser_set_hooks(amxo_parser_t *parser,
                          amxo_hooks_t *hooks) {
    int retval = -1;
    when_null(parser, exit);
    when_null(hooks, exit);
    when_not_null(hooks->it.llist, exit);

    if(parser->hooks == NULL) {
        amxc_llist_new(&parser->hooks);
    }
    retval = amxc_llist_append(parser->hooks, &hooks->it);

exit:
    return retval;
}

int amxo_parser_unset_hooks(amxo_parser_t *parser,
                            amxo_hooks_t *hooks) {
    int retval = -1;

    when_null(parser, exit);
    when_null(hooks, exit);

    when_true(parser->hooks != hooks->it.llist, exit);

    amxc_llist_it_take(&hooks->it);
    if(amxc_llist_is_empty(parser->hooks)) {
        amxc_llist_delete(&parser->hooks, NULL);
    }
    retval = 0;

exit:
    return retval;
}