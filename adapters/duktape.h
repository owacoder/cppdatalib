#ifndef CPPDATALIB_DUKTAPE_H
#define CPPDATALIB_DUKTAPE_H

#ifdef CPPDATALIB_ENABLE_DUKTAPE
#include <duktape.h>

#include "../core/value_builder.h"

/*
 *  Since there is no good way to emulate buffer behavior in cppdatalib (TODO?? :)),
 *  buffers will be serialized and deserialized as blob strings with the following attributes object:
 *
 *    {
 *       "external": true/false, // Defaults to false for cppdatalib->Duktape. If true, then the returned blob will be empty for Duktape->cppdatalib. Always present for Duktape->cppdatalib
 *       "dynamic": true/false, // Defaults to true for cppdatalib->Duktape. Always present for Duktape->cppdatalib
 *       "size": uinteger size, // Defaults to size of blob for cppdatalib->Duktape. Always present for Duktape->cppdatalib
 *       "pointer": uinteger pointer to buffer // Not needed for cppdatalib->Duktape. Always present for Duktape->cppdatalib
 *    }
 */

template<>
class cast_existing_pointer_to_cppdatalib<duk_context *>
{
    duk_context *bind;
    duk_idx_t stack_index;
public:
    cast_existing_pointer_to_cppdatalib(duk_context *bind, duk_idx_t stack_index = -1) : bind(bind), stack_index(stack_index) {}
    operator cppdatalib::core::value() {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        switch (duk_get_type(bind, stack_index))
        {
            case DUK_TYPE_NONE:
            case DUK_TYPE_NULL: dest.set_null(); break;
            case DUK_TYPE_UNDEFINED: dest.set_null(cppdatalib::core::undefined); break;
            case DUK_TYPE_BOOLEAN: dest.set_bool(duk_get_boolean(bind, stack_index)); break;
            case DUK_TYPE_NUMBER: dest.set_real(duk_get_number(bind, stack_index)); break;
            case DUK_TYPE_POINTER: dest.set_uint(reinterpret_cast<uintmax_t>(duk_get_pointer(bind, stack_index)), cppdatalib::core::pointer); break;
            case DUK_TYPE_LIGHTFUNC: dest.set_uint(reinterpret_cast<uintmax_t>(duk_get_pointer(bind, stack_index)), cppdatalib::core::function_pointer); break;
            case DUK_TYPE_BUFFER:
            {
                duk_size_t size;
                void *p = duk_get_buffer_data(bind, stack_index, &size);

                dest.set_string({}, cppdatalib::core::blob);
                dest.attribute("external") = duk_is_external_buffer(bind, stack_index);
                dest.attribute("dynamic") = duk_is_dynamic_buffer(bind, stack_index);
                dest.attribute("pointer") = cppdatalib::core::value(reinterpret_cast<uintmax_t>(p), cppdatalib::core::pointer);
                dest.attribute("size") = size;

                if (!duk_is_external_buffer(bind, stack_index))
                    dest.get_owned_string_ref() = std::string(reinterpret_cast<char *>(p), size);

                break;
            }
            case DUK_TYPE_STRING:
            {
                duk_size_t strsize;
                const char *str = duk_get_lstring(bind, stack_index, &strsize);

                dest.set_string(std::string(str, strsize));
                break;
            }
            case DUK_TYPE_OBJECT:
            {
                if (duk_is_array(bind, stack_index))
                {
                    duk_enum(bind, stack_index, DUK_ENUM_OWN_PROPERTIES_ONLY |
                                                DUK_ENUM_ARRAY_INDICES_ONLY);
                    while (duk_next(bind, -1, true))
                    {
                        duk_size_t sz = duk_to_uint(bind, -2);

                        while (dest.size() < sz)
                            dest.push_back(cppdatalib::core::value(cppdatalib::core::null_t{}, cppdatalib::core::undefined));
                        dest.push_back(cppdatalib::core::value::from_existing_pointer(bind));

                        duk_pop_2(bind);
                    }

                    duk_pop(bind);
                }
                else
                {
                    duk_enum(bind, stack_index, DUK_ENUM_SORT_ARRAY_INDICES);
                    while (duk_next(bind, -1, true))
                    {
                        dest.add_member_at_end(cppdatalib::core::value::from_existing_pointer(bind, -2, cppdatalib::core::userdata_tag()),
                                               cppdatalib::core::value::from_existing_pointer(bind));
                        duk_pop_2(bind);
                    }

                    duk_pop(bind);
                }

                break;
            }
        }
    }
};

template<>
class cast_existing_pointer_from_cppdatalib<duk_context *>
{
    const cppdatalib::core::value &bind;
public:
    cast_existing_pointer_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    void convert(duk_context *dest) const {
        switch (bind.get_type())
        {
            case cppdatalib::core::link:
            case cppdatalib::core::null:
                if (bind.get_subtype() == cppdatalib::core::undefined)
                    duk_push_undefined(dest);
                else
                    duk_push_null(dest);
                break;
            case cppdatalib::core::boolean: duk_push_boolean(dest, bind.get_bool_unchecked()); break;
            case cppdatalib::core::integer: duk_push_int(dest, bind.get_int_unchecked()); break;
            case cppdatalib::core::uinteger: duk_push_uint(dest, bind.get_uint_unchecked()); break;
            case cppdatalib::core::real: duk_push_number(dest, bind.get_real_unchecked()); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
            case cppdatalib::core::temporary_string:
#endif
            case cppdatalib::core::string:
                if (bind.get_subtype() == cppdatalib::core::blob) // Must be a buffer
                {
                    if (bind.const_attribute("external").as_bool())
                    {
                        duk_push_external_buffer(dest);
                        duk_config_buffer(dest,
                                          -1,
                                          reinterpret_cast<void *>(bind.const_attribute("pointer").as_uint()),
                                          bind.const_attribute("size").as_uint());
                    }
                    else
                    {
                        size_t len = bind.const_attribute("size").as_uint(bind.string_size());

                        void *data = duk_push_buffer(dest, len, bind.const_attribute("dynamic").as_bool(true));

                        if (bind.string_size())
                            memcpy(data, bind.get_string_unchecked().data(), std::min(len, bind.string_size()));
                    }
                }
                else
                    duk_push_lstring(dest, bind.get_string_unchecked().data(), bind.get_string_unchecked().size());
                break;
            case cppdatalib::core::array:
            {
                duk_idx_t array_idx = duk_push_array(dest);
                size_t idx = 0;

                for (const cppdatalib::core::value &item: bind.get_array_unchecked())
                {
                    item.cast_to_existing_pointer(dest);
                    duk_put_prop_index(dest, array_idx, idx++);
                }

                break;
            }
            case cppdatalib::core::object:
            {
                duk_idx_t object_idx = duk_push_object(dest);

                for (auto it = bind.get_object_unchecked().begin(); it != bind.get_object_unchecked().end(); ++it)
                {
                    it->first.cast_to_existing_pointer(dest);
                    it->second.cast_to_existing_pointer(dest);
                    duk_put_prop(dest, object_idx);
                }

                break;
            }
        }
    }
};

#endif // CPPDATALIB_ENABLE_DUKTAPE
#endif // CPPDATALIB_DUKTAPE_H
