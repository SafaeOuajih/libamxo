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

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>

#include <amxc/amxc.h>
#include <amxp/amxp.h>

#include <amxd/amxd_dm.h>
#include <amxd/amxd_object.h>
#include <amxd/amxd_object_expression.h>

#include <amxo/amxo.h>
#include <amxo/amxo_save.h>

#include "amxo_assert.h"
#include "amxo_parser_priv.h"

#define PARAM_ATTR(param, attr_name) \
    amxc_var_dyncast(bool,                                        \
                     amxc_var_get_path(param,                     \
                                       attr_name,                 \
                                       AMXC_VAR_FLAG_DEFAULT))

#define PARAM_NAME(param) \
    amxc_var_constcast(cstring_t,                                 \
                       amxc_var_get_path(param,                   \
                                         "name",                  \
                                         AMXC_VAR_FLAG_DEFAULT))

#define PARAM_FLAGS(param) \
    amxc_var_constcast(amxc_llist_t,                              \
                       amxc_var_get_path(param,                   \
                                         "flags",                 \
                                         AMXC_VAR_FLAG_DEFAULT))

#define PARAM_VALUE(param) \
    amxc_var_get_path(param,                                      \
                      "value",                                    \
                      AMXC_VAR_FLAG_DEFAULT)


static int indentation = 0;

static int amxo_parser_save_object_tree(int fd,
                                        amxd_object_t* object,
                                        uint32_t depth,
                                        amxc_string_t* buffer);

static int amxo_parser_write(int fd, const char* buf, size_t bytes) {
    int retval = 0;
    size_t length = 0;
    ssize_t written = 0;
    while(length != bytes) {
        written = write(fd, buf + length, bytes - length);
        if(written < 0) {
            retval = errno;
            break;
        }
        length += written;
    }

    return retval;
}

static int amxo_parser_flush_buffer(int fd, amxc_string_t* buffer) {
    static const char* spaces = "                                        ";
    int retval = 0;
    const char* buf = amxc_string_get(buffer, 0);
    size_t length = amxc_string_text_length(buffer);

    if(indentation > 0) {
        for(int i = indentation; i > 0; i -= 40) {
            int size = (i > 40) ? 40 : i;
            retval = amxo_parser_write(fd, spaces, size);
            when_failed(retval, exit);
        }
    }

    retval = amxo_parser_write(fd, buf, length);

exit:
    amxc_string_reset(buffer);
    return retval;
}

static void amxo_parser_writef(amxc_string_t* buffer,
                               const char* fmt,
                               ...) {
    va_list args;

    va_start(args, fmt);
    amxc_string_vappendf(buffer, fmt, args);
    va_end(args);
}

static char* amxo_parser_build_filename(amxo_parser_t* pctx,
                                        const char* filename,
                                        bool temp) {
    char* full_name = NULL;
    amxc_string_t full_path;

    amxc_string_init(&full_path, 128);
    amxc_string_appendf(&full_path, "%s%s", filename, temp ? ".tmp" : "");
    amxc_string_resolve(&full_path, &pctx->config);
    full_name = amxc_string_take_buffer(&full_path);
    amxc_string_clean(&full_path);

    return full_name;
}

static int amxo_parser_copy(int dest_fd, int source_fd) {
    int retval = 0;

    ssize_t length = 0;
    char buffer[1024];

    length = read(source_fd, buffer, 1024);
    while(length > 0) {
        retval = amxo_parser_write(dest_fd, buffer, (size_t) length);
        when_failed(retval, exit);
        length = read(source_fd, buffer, 1024);
    }

exit:
    return retval;
}

static int amxo_parser_open_file(amxo_parser_t* pctx,
                                 const char* filename,
                                 bool append) {
    int fd = -1;
    char* full_path_tmp = amxo_parser_build_filename(pctx, filename, true);
    char* full_path = amxo_parser_build_filename(pctx, filename, false);
    char* real_path = NULL;
    int open_flags = O_CREAT | O_WRONLY | O_TRUNC;
    int mode_flags = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

    if(append) {
        real_path = realpath(full_path, NULL);
        when_null(real_path, exit);
    }
    fd = open(full_path_tmp, open_flags, mode_flags);
    when_true(fd == -1, exit);

    if(append) {
        int orig_fd = open(real_path, O_RDONLY);
        if(orig_fd == -1) {
            close(fd);
            fd = -1;
            goto exit;
        }
        if(amxo_parser_copy(fd, orig_fd) != 0) {
            close(orig_fd);
            close(fd);
            fd = -1;
            goto exit;
        }
        close(orig_fd);
    }

exit:
    free(full_path_tmp);
    free(full_path);
    free(real_path);
    return fd;
}

static int amxo_parser_save_value(amxc_var_t* value,
                                  amxc_string_t* buffer) {
    int type = amxc_var_type_of(value);
    int retval = 0;
    const char* const_txt = NULL;
    char* txt = NULL;
    switch(type) {
    case AMXC_VAR_ID_INT8:
    case AMXC_VAR_ID_INT16:
    case AMXC_VAR_ID_INT32:
    case AMXC_VAR_ID_INT64:
    case AMXC_VAR_ID_UINT8:
    case AMXC_VAR_ID_UINT16:
    case AMXC_VAR_ID_UINT32:
    case AMXC_VAR_ID_UINT64:
    case AMXC_VAR_ID_BOOL:
        txt = amxc_var_dyncast(cstring_t, value);
        amxo_parser_writef(buffer, "%s", txt);
        free(txt);
        break;
    case AMXC_VAR_ID_TIMESTAMP:
        txt = amxc_var_dyncast(cstring_t, value);
        amxo_parser_writef(buffer, "\"%s\"", txt);
        free(txt);
        break;
    case AMXC_VAR_ID_CSTRING:
    case AMXC_VAR_ID_SSV_STRING:
    case AMXC_VAR_ID_CSV_STRING:
        const_txt = amxc_var_constcast(cstring_t, value);
        amxo_parser_writef(buffer, "\"%s\"", const_txt);
        break;
    default:
        retval = -1;
        break;
    }

    return retval;
}

static int amxo_parser_save_table_config(const char* name,
                                         amxc_var_t* value,
                                         amxc_string_t* buffer) {
    int retval = -1;
    const amxc_htable_t* hvalue = amxc_var_constcast(amxc_htable_t, value);
    const char* sep = "";
    amxo_parser_writef(buffer, "%s = {", name);
    amxc_htable_for_each(it, hvalue) {
        amxc_var_t* val = amxc_var_from_htable_it(it);
        const char* key = amxc_htable_it_get_key(it);
        when_true(!amxd_name_is_valid(key), exit);

        amxo_parser_writef(buffer, "%s%s = ", sep, key);
        retval = amxo_parser_save_value(val, buffer);
        when_true(retval < 0, exit);
        sep = ",";
    }
    amxo_parser_writef(buffer, "};\n");
    retval = 0;

exit:
    return retval;
}

static int amxo_parser_save_list_config(const char* name,
                                        amxc_var_t* value,
                                        amxc_string_t* buffer) {
    int retval = -1;
    const amxc_llist_t* lvalue = amxc_var_constcast(amxc_llist_t, value);
    const char* sep = "";
    amxo_parser_writef(buffer, "%s = [", name);
    amxc_llist_for_each(it, lvalue) {
        amxc_var_t* val = amxc_var_from_llist_it(it);
        amxo_parser_writef(buffer, "%s", sep);
        retval = amxo_parser_save_value(val, buffer);
        when_true(retval < 0, exit);
        sep = ", ";
    }
    amxo_parser_writef(buffer, "];\n");
    retval = 0;

exit:
    return retval;
}

static int amxo_parser_save_config_options(int fd,
                                           amxc_var_t* config,
                                           amxc_string_t* buffer) {
    int retval = 0;
    const amxc_htable_t* hconfig = amxc_var_constcast(amxc_htable_t, config);
    amxo_parser_writef(buffer, "%%config {\n");
    retval = amxo_parser_flush_buffer(fd, buffer);
    when_true(retval < 0, exit);
    indentation += 4;

    amxc_htable_for_each(it, hconfig) {
        amxc_var_t* option = amxc_var_from_htable_it(it);
        const char* key = amxc_htable_it_get_key(it);
        retval = -1;
        when_true(!amxd_name_is_valid(key), exit);

        if(amxc_var_type_of(option) == AMXC_VAR_ID_HTABLE) {
            retval = amxo_parser_save_table_config(key, option, buffer);
            when_true(retval < 0, exit);
        } else if(amxc_var_type_of(option) == AMXC_VAR_ID_LIST) {
            retval = amxo_parser_save_list_config(key, option, buffer);
            when_true(retval < 0, exit);
        } else {
            amxo_parser_writef(buffer, "%s = ", key);
            amxo_parser_save_value(option, buffer);
            amxo_parser_writef(buffer, ";\n", key);
        }
        retval = amxo_parser_flush_buffer(fd, buffer);
        when_true(retval < 0, exit);
    }

    indentation -= 4;
    amxo_parser_writef(buffer, "}\n");
    retval = amxo_parser_flush_buffer(fd, buffer);

exit:
    return retval;
}

static bool amxo_parser_has_key_params(const amxc_htable_t* params) {
    bool retval = false;

    amxc_htable_for_each(it, params) {
        amxc_var_t* param = amxc_var_from_htable_it(it);

        if(PARAM_ATTR(param, "attributes.key")) {
            retval = true;
            break;
        }
    }

    return retval;
}

static int amxo_parser_instance_header(amxd_object_t* object,
                                       amxc_string_t* buffer) {
    int retval = 0;
    const amxc_htable_t* ht_params = NULL;
    amxc_var_t params;
    const char* inst_name = amxd_object_get_name(object, AMXD_OBJECT_NAMED);

    amxo_parser_writef(buffer, "instance add(%d", amxd_object_get_index(object));
    if(amxd_name_is_valid(inst_name)) {
        amxo_parser_writef(buffer, ", %s", inst_name);
    }

    amxc_var_init(&params);
    amxd_object_describe_params(object, &params, amxd_dm_access_public);
    ht_params = amxc_var_constcast(amxc_htable_t, &params);
    if(amxo_parser_has_key_params(ht_params)) {
        amxc_htable_for_each(it, ht_params) {
            amxc_var_t* param = amxc_var_from_htable_it(it);
            const char* name = amxc_htable_it_get_key(it);
            amxc_var_t* value = PARAM_VALUE(param);
            bool is_key_param = PARAM_ATTR(param, "attributes.key");
            if(!is_key_param) {
                continue;
            }
            amxo_parser_writef(buffer, ", %s = ", name);
            retval = amxo_parser_save_value(value, buffer);
            when_true(retval < 0, exit);
        }
    }
    amxo_parser_writef(buffer, ") {\n");

exit:
    amxc_var_clean(&params);
    return retval;
}

static int amxo_parser_open_parent_tree(int fd,
                                        amxd_object_t* object,
                                        amxc_string_t* buffer) {
    int retval = 0;
    if(amxd_object_get_type(object) != amxd_object_root) {
        retval = amxo_parser_open_parent_tree(fd,
                                              amxd_object_get_parent(object),
                                              buffer);
        when_true(retval < 0, exit);
    }

    if(amxd_object_get_type(object) == amxd_object_root) {
        goto exit;
    }

    if(amxd_object_get_type(object) == amxd_object_instance) {
        amxo_parser_instance_header(object, buffer);
    } else {
        amxo_parser_writef(buffer,
                           "object %s {\n",
                           amxd_object_get_name(object, AMXD_OBJECT_NAMED));
    }
    retval = amxo_parser_flush_buffer(fd, buffer);
    indentation += 4;

exit:
    return retval;
}

static int amxo_parser_save_mibs(int fd,
                                 amxd_object_t* object,
                                 amxc_string_t* buffer) {
    int retval = 0;
    amxc_array_it_t* it = amxc_array_get_first(&object->mib_names);
    while(it) {
        const char* name = (const char*) amxc_array_it_get_data(it);
        amxo_parser_writef(buffer, "extend using mib %s;\n", name);
        retval = amxo_parser_flush_buffer(fd, buffer);
        when_true(retval < 0, exit);
        it = amxc_array_it_get_next(it);
    }

exit:
    return retval;
}

static int amxo_parser_save_param_flags(amxc_var_t* param,
                                        amxc_string_t* buffer) {
    int retval = 0;
    const amxc_llist_t* flags = PARAM_FLAGS(param);
    const char* sep = "";
    amxo_parser_writef(buffer, "{\n");
    indentation += 4;
    amxo_parser_writef(buffer, "flags ");
    amxc_llist_iterate(it, flags) {
        amxc_var_t* var_flag = amxc_var_from_llist_it(it);
        const char* flag = amxc_var_constcast(cstring_t, var_flag);
        amxo_parser_writef(buffer, "%s %%%s", sep, flag);
        sep = ",";
    }
    amxo_parser_writef(buffer, ";\n");
    indentation -= 4;
    amxo_parser_writef(buffer, "}\n");

    return retval;
}

static int amxo_parser_save_params(int fd,
                                   amxd_object_t* object,
                                   amxc_string_t* buffer) {
    int retval = 0;
    const amxc_htable_t* ht_params = NULL;
    amxc_var_t params;
    amxc_var_init(&params);

    amxd_object_describe_params(object, &params, amxd_dm_access_public);
    ht_params = amxc_var_constcast(amxc_htable_t, &params);

    amxc_htable_for_each(it, ht_params) {
        amxc_var_t* param = amxc_var_from_htable_it(it);
        const char* name = PARAM_NAME(param);
        amxc_var_t* value = PARAM_VALUE(param);
        const amxc_llist_t* flags = PARAM_FLAGS(param);
        bool is_templ_param = PARAM_ATTR(param, "attributes.template");
        bool is_inst_param = PARAM_ATTR(param, "attributes.instance");
        bool is_key_param = PARAM_ATTR(param, "attributes.key");
        bool is_persist_param = PARAM_ATTR(param, "attributes.persistent");

        if(amxd_object_get_type(object) == amxd_object_template) {
            if(is_key_param || !is_persist_param || !is_templ_param) {
                continue;
            }
        } else if(amxd_object_get_type(object) == amxd_object_instance) {
            if(is_key_param || !is_persist_param || !is_inst_param) {
                continue;
            }
        } else {
            if(!is_persist_param) {
                continue;
            }
        }

        amxo_parser_writef(buffer, "parameter %s = ", name);
        retval = amxo_parser_save_value(value, buffer);
        when_true(retval < 0, exit);
        if(!amxc_llist_is_empty(flags)) {
            amxo_parser_save_param_flags(param, buffer);
        } else {
            amxo_parser_writef(buffer, ";\n");
        }
        retval = amxo_parser_flush_buffer(fd, buffer);
        when_true(retval < 0, exit);
    }

exit:
    amxc_var_clean(&params);
    return retval;
}

static int amxo_parser_save_leave(int fd,
                                  amxc_llist_it_t* it,
                                  uint32_t depth,
                                  amxc_string_t* buffer) {
    int retval = 0;
    amxd_object_t* obj = amxc_container_of(it, amxd_object_t, it);
    if(!amxd_object_is_attr_set(obj, amxd_oattr_persistent) &&
       ( amxd_object_get_type(obj) != amxd_object_template)) {
        goto exit;
    }
    if(amxd_object_get_type(obj) == amxd_object_instance) {
        amxo_parser_instance_header(obj, buffer);
    } else {
        amxo_parser_writef(buffer,
                           "object %s {\n",
                           amxd_object_get_name(obj, AMXD_OBJECT_NAMED));

    }
    retval = amxo_parser_flush_buffer(fd, buffer);
    when_true(retval < 0, exit);
    indentation += 4;
    retval = amxo_parser_save_object_tree(fd, obj, depth, buffer);
    when_true(retval < 0, exit);
    indentation -= 4;
    amxo_parser_writef(buffer, "}\n");
    retval = amxo_parser_flush_buffer(fd, buffer);
    when_true(retval < 0, exit);

exit:
    return retval;
}

static int amxo_parser_save_object_tree(int fd,
                                        amxd_object_t* object,
                                        uint32_t depth,
                                        amxc_string_t* buffer) {
    int retval = 0;

    if(amxd_object_get_type(object) != amxd_object_root) {
        retval = amxo_parser_save_mibs(fd, object, buffer);
        when_failed(retval, exit);
        retval = amxo_parser_save_params(fd, object, buffer);
        when_failed(retval, exit);
        if(depth != UINT32_MAX) {
            depth--;

        }
        when_true(depth == 0, exit);
    }

    if(amxd_object_get_type(object) == amxd_object_template) {
        amxd_object_for_each(instance, it, object) {
            retval = amxo_parser_save_leave(fd, it, depth, buffer);
            when_failed(retval, exit);
        }
    } else {
        amxd_object_for_each(child, it, object) {
            retval = amxo_parser_save_leave(fd, it, depth, buffer);
            when_failed(retval, exit);
        }
    }

exit:
    return retval;
}

static int amxo_parser_close_parent_tree(int fd,
                                         amxd_object_t* object,
                                         amxc_string_t* buffer) {
    int retval = 0;
    if(amxd_object_get_type(object) == amxd_object_root) {
        goto exit;
    }

    indentation -= 4;
    amxo_parser_writef(buffer, "}\n");
    retval = amxo_parser_flush_buffer(fd, buffer);
    when_true(retval < 0, exit);

    retval = amxo_parser_close_parent_tree(fd,
                                           amxd_object_get_parent(object),
                                           buffer);

exit:
    return retval;
}

static int amxo_parser_save_tree(int fd,
                                 amxd_object_t* object,
                                 uint32_t depth,
                                 amxc_string_t* buffer) {
    int retval = 0;
    amxo_parser_writef(buffer, "%%populate {\n");
    retval = amxo_parser_flush_buffer(fd, buffer);
    when_true(retval < 0, exit);
    indentation += 4;

    retval = amxo_parser_open_parent_tree(fd, object, buffer);
    when_failed(retval, exit);
    retval = amxo_parser_save_object_tree(fd, object, depth, buffer);
    when_failed(retval, exit);
    retval = amxo_parser_close_parent_tree(fd, object, buffer);
    when_failed(retval, exit);

    indentation -= 4;
    amxo_parser_writef(buffer, "}\n");
    retval = amxo_parser_flush_buffer(fd, buffer);

exit:
    return retval;
}

static void amxo_parser_remove_file(amxo_parser_t* pctx,
                                    const char* filename,
                                    int fd) {
    char* full_path_tmp = amxo_parser_build_filename(pctx, filename, true);

    close(fd);
    unlink(full_path_tmp);
    free(full_path_tmp);
}

static int amxo_parser_close_file(amxo_parser_t* pctx,
                                  const char* filename,
                                  int fd) {
    int retval = 0;
    char* full_path_tmp = amxo_parser_build_filename(pctx, filename, true);
    char* full_path = amxo_parser_build_filename(pctx, filename, false);

    retval = fsync(fd);
    when_failed(retval, exit);
    retval = close(fd);
    when_failed(retval, exit);
    retval = rename(full_path_tmp, full_path);
    when_failed(retval, exit);

exit:
    free(full_path_tmp);
    free(full_path);
    return retval;
}

int amxo_parser_save(amxo_parser_t* pctx,
                     const char* filename,
                     amxd_object_t* object,
                     uint32_t depth,
                     amxc_var_t* config,
                     bool append) {
    int retval = -1;
    int fd = -1;
    amxc_string_t buffer;
    amxc_string_init(&buffer, 128);

    when_null(pctx, exit);
    when_str_empty(filename, exit);

    indentation = 0;
    pctx->status = amxd_status_ok;
    fd = amxo_parser_open_file(pctx, filename, append);
    when_true(fd < 0, exit);
    if((config != NULL) && (amxc_var_type_of(config) == AMXC_VAR_ID_HTABLE)) {
        retval = amxo_parser_save_config_options(fd, config, &buffer);
        when_true(retval < 0, exit);
    }

    if(object != NULL) {
        retval = amxo_parser_save_tree(fd, object, depth, &buffer);
        when_true(retval < 0, exit);
    }

exit:
    if(fd >= 0) {
        if(retval < 0) {
            amxo_parser_remove_file(pctx, filename, fd);
        } else {
            amxo_parser_close_file(pctx, filename, fd);
        }
    } else {
        retval = -1;
    }
    amxc_string_clean(&buffer);
    return retval;
}

int amxo_parser_save_config(amxo_parser_t* pctx,
                            const char* filename,
                            amxc_var_t* config,
                            bool append) {
    return amxo_parser_save(pctx, filename, NULL, 0, config, append);
}

int amxo_parser_save_object(amxo_parser_t* pctx,
                            const char* filename,
                            amxd_object_t* object,
                            bool append) {
    return amxo_parser_save(pctx, filename, object, UINT32_MAX, NULL, append);
}
