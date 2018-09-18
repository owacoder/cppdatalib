/*
 * json_pointer.h
 *
 * Copyright © 2017 Oliver Adams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef CPPDATALIB_JSON_POINTER_H
#define CPPDATALIB_JSON_POINTER_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace json
    {
        namespace pointer
        {
            namespace impl
            {
                inline bool normalize_node_path(std::string &path_node)
                {
                    if (path_node.empty())
                        return true;

                    if (path_node[0] != '/')
                        return false;

                    for (size_t i = 1; i < path_node.size(); ++i)
                    {
                        if (path_node[i] == '~')
                        {
                            if (i + 1 == path_node.size())
                                return false;
                            switch (path_node[i+1])
                            {
                                case '0': path_node.replace(i, 2, "~"); break;
                                case '1': path_node.replace(i, 2, "/"); break;
                                default: return false;
                            }
                        }
                    }

                    return true;
                }

                inline const core::value *evaluate(const core::value &value, const std::string &pointer, bool throw_on_errors = true)
                {
                    size_t idx = 1, end_idx = 0;
                    std::string path_node;
                    const core::value *reference = &value;

                    if (pointer.empty())
                        return reference;
                    else if (pointer.find('/') != 0)
                    {
                        if (throw_on_errors)
                            throw core::error("JSON Pointer - Expected empty path or '/' beginning path");
                        else
                            return NULL;
                    }

                    end_idx = pointer.find('/', idx);
                    while (idx <= pointer.size())
                    {
                        // Extract the current node from pointer
                        if (end_idx == std::string::npos)
                            path_node = pointer.substr(idx, end_idx);
                        else
                            path_node = pointer.substr(idx, end_idx - idx);
                        idx += path_node.size() + 1;

                        // Escape special characters in strings
                        if (!normalize_node_path(path_node))
                        {
                            if (throw_on_errors)
                                throw core::error("JSON Pointer - Expected identifier following '~'");
                            else
                                return NULL;
                        }

                        // Dereference
                        if (reference->is_object())
                        {
                            reference = reference->member_ptr(core::value(path_node));
                            if (!reference)
                            {
                                if (throw_on_errors)
                                    throw core::error("JSON Pointer - Attempted to dereference non-existent member in object");
                                else
                                    return NULL;
                            }
                        }
                        else if (reference->is_array())
                        {
                            for (size_t i = 0; i < path_node.size(); ++i)
                            {
                                if (!isdigit(path_node[i] & 0xff))
                                {
                                    if (throw_on_errors)
                                        throw core::error("JSON Pointer - Attempted to dereference invalid array index");
                                    else
                                        return NULL;
                                }
                            }

                            core::uint_t array_index;
                            core::istring_wrapper_stream stream(path_node);
                            stream >> array_index;
                            if (stream.fail() || stream.get() != EOF || (path_node[0] == '0' && path_node != "0") || array_index > reference->array_size()) // Check whether there are leading zeroes
                            {
                                if (throw_on_errors)
                                    throw core::error("JSON Pointer - Attempted to dereference invalid array index");
                                else
                                    return NULL;
                            }

                            reference = &reference->get_array_unchecked()[static_cast<size_t>(array_index)];
                        }
                        else
                        {
                            if (throw_on_errors)
                                throw core::error("JSON Pointer - Attempted to dereference a scalar value");
                            else
                                return NULL;
                        }

                        // Look for the next node
                        end_idx = pointer.find('/', idx);
                    }

                    return reference;
                }

                /* Evaluates the pointer, with special behaviors
                 *
                 * `value` is the root value
                 * `pointer` is the string representation of the JSON Pointer
                 * `parent` is a reference to a variable outside the function, in which the parent of the returned value is placed
                 *   `parent` may contain NULL even if the return value is non-zero if the return value is the root, without a known parent
                 *   If the return value is NULL (only possible when `throw_on_errors == false`), `parent` is meaningless
                 *   If `destroy_element` is specified, the return value is the parent of the destroyed value, and `parent` is the grandparent of the destroyed value
                 * `throw_on_errors == true` throws an error if the element does not exist (unless `allow_add_element == true`, in which case the last node path may not exist)
                 *   If `throw_on_errors == true`, the return value will never be NULL
                 * `throw_on_errors == false` returns NULL if the element does not exist (unless `allow_add_element == true`, in which case the last node path may not exist)
                 * `allow_add_element == true` allows the last node element to not exist, or be "-" to append to the end of an existing array
                 *   A null value will always be added if the element does not exist
                 * `destroy_element == true` destroys the node (if it exists) and returns the parent as the return value, and the grandparent in `parent`
                 *   The exception is when destroying the root, the root will be set to null and a pointer to the root will be returned (otherwise there's no way to tell, when `throw_on_errors == false`, whether the element was destroyed if NULL was returned)
                 */
                inline core::value *evaluate(core::value &value, const std::string &pointer, core::value *&parent, bool throw_on_errors = true, bool allow_add_element = false, bool destroy_element = false)
                {
                    size_t idx = 1, end_idx = 0;
                    std::string path_node;
                    core::value *reference = &value;

                    parent = NULL;

                    if (pointer.empty())
                    {
                        if (destroy_element)
                            reference->set_null();

                        return reference;
                    }
                    else if (pointer.find('/') != 0)
                    {
                        if (throw_on_errors)
                            throw core::error("JSON Pointer - Expected empty path or '/' beginning path");
                        else
                            return NULL;
                    }

                    end_idx = pointer.find('/', idx);
                    while (idx <= pointer.size())
                    {
                        // Extract the current node from pointer
                        if (end_idx == std::string::npos)
                            path_node = pointer.substr(idx, end_idx);
                        else
                            path_node = pointer.substr(idx, end_idx - idx);
                        idx += path_node.size() + 1;

                        // Escape special characters in strings
                        if (!normalize_node_path(path_node))
                        {
                            if (throw_on_errors)
                                throw core::error("JSON Pointer - Expected identifier following '~'");
                            else
                                return NULL;
                        }

                        // Dereference
                        if (reference->is_object())
                        {
                            core::value member_name(path_node);

                            if (destroy_element && idx > pointer.size())
                            {
                                reference->erase_member(member_name);
                                return reference;
                            }
                            else if (reference->is_member(member_name))
                            {
                                parent = reference;
                                reference = &reference->member(member_name);
                            }
                            else if (allow_add_element && idx > pointer.size())
                            {
                                parent = reference;
                                return &reference->member(member_name);
                            }
                            else if (throw_on_errors)
                                throw core::error("JSON Pointer - Attempted to dereference non-existent member in object");
                            else
                                return NULL;
                        }
                        else if (reference->is_array())
                        {
                            if (allow_add_element && idx > pointer.size() && path_node == "-")
                            {
                                parent = reference;
                                reference->push_back(core::null_t());
                                return &reference->get_array_ref().data().back();
                            }

                            for (size_t i = 0; i < path_node.size(); ++i)
                            {
                                if (!isdigit(path_node[i] & 0xff))
                                {
                                    if (throw_on_errors)
                                        throw core::error("JSON Pointer - Attempted to dereference invalid array index");
                                    else
                                        return NULL;
                                }
                            }

                            core::uint_t array_index;
                            core::istring_wrapper_stream stream(path_node);
                            stream >> array_index;
                            if (stream.fail() || stream.get() != EOF || (path_node[0] == '0' && path_node != "0") || array_index >= reference->array_size()) // Check whether there are leading zeroes
                            {
                                if (throw_on_errors)
                                    throw core::error("JSON Pointer - Attempted to dereference invalid array index");
                                else
                                    return NULL;
                            }

                            if (destroy_element && idx > pointer.size())
                            {
                                reference->erase_element(static_cast<size_t>(array_index));
                                return reference;
                            }

                            parent = reference;
                            reference = &reference->get_array_ref()[static_cast<size_t>(array_index)];
                        }
                        else
                        {
                            if (throw_on_errors)
                                throw core::error("JSON Pointer - Attempted to dereference a scalar value");
                            else
                                return NULL;
                        }

                        // Look for the next node
                        end_idx = pointer.find('/', idx);
                    }

                    return reference;
                }
            }

            // Returns true if the object pointed to by pointer exists
            inline bool exists(const core::value &value, const std::string &pointer)
            {
                return impl::evaluate(value, pointer, false) != NULL;
            }

            // Returns the object pointed to by pointer
            inline const core::value &deref(const core::value &value, const std::string &pointer)
            {
                return *impl::evaluate(value, pointer);
            }

            // Returns the object pointed to by pointer
            inline core::value &deref(core::value &value, const std::string &pointer)
            {
                core::value *parent;
                return *impl::evaluate(value, pointer, parent, true, false, false);
            }

            // Returns newly inserted element
            inline core::value &add(core::value &value, const std::string &pointer, const core::value &src)
            {
                core::value *parent;
                return *impl::evaluate(value, pointer, parent, true, true, false) = src;
            }

            inline void remove(core::value &value, const std::string &pointer)
            {
                core::value *parent;
                impl::evaluate(value, pointer, parent, true, false, true);
            }

            // Returns newly replaced element
            inline core::value &replace(core::value &value, const std::string &pointer, const core::value &src)
            {
                return deref(value, pointer) = src;
            }

            // Returns newly inserted destination element
            inline core::value &move(core::value &value, const std::string &dst_pointer, const std::string &src_pointer)
            {
                core::value src = deref(value, src_pointer);
                remove(value, src_pointer);
                return add(value, dst_pointer, src);
            }

            // Returns newly inserted destination element
            inline core::value &copy(core::value &value, const std::string &dst_pointer, const std::string &src_pointer)
            {
                core::value src = deref(value, src_pointer);
                return add(value, dst_pointer, src);
            }

            // Returns true if the value pointed to by `pointer` equals the value of `src`
            inline bool test(core::value &value, const std::string &pointer, const core::value &src)
            {
                return deref(value, pointer) == src;
            }

            inline bool is_valid_path(std::string path)
            {
                return impl::normalize_node_path(path);
            }

            // Returns array with specified path
            // Elements are unsigned integers for array indexes, null values for insertion indexes ("-" elements)
            // and strings for object keys
            inline core::value path_to_array(core::string_view_t path)
            {
                core::value result = core::value(core::array_t());

                if (path.empty() || !is_valid_path(static_cast<std::string>(path)))
                    return core::value();

                size_t idx = 1, end_idx = 0;
                end_idx = path.find('/', idx);
                while (idx <= path.size())
                {
                    std::string path_node;

                    // Extract the current node from pointer
                    if (end_idx == std::string::npos)
                        path_node = static_cast<std::string>(path.substr(idx, end_idx));
                    else
                        path_node = static_cast<std::string>(path.substr(idx, end_idx - idx));
                    idx += path_node.size() + 1;

                    // Escape special characters in strings
                    impl::normalize_node_path(path_node);

                    // Is it an array entry?
                    bool all_digits = true;
                    for (size_t i = 0; i < path_node.size(); ++i)
                        if (!isdigit(path_node[i] & 0xff))
                        {
                            all_digits = false;
                            break;
                        }

                    // Add to result
                    if (all_digits)
                    {
                        core::uint_t aIndex;
                        core::istring_wrapper_stream wrapper(path_node);
                        wrapper >> aIndex;
                        result.push_back(aIndex);
                    }
                    else if (path_node == "-")
                        result.push_back(core::value());
                    else
                        result.push_back(core::value(path_node));

                    // Look for the next node
                    end_idx = path.find('/', idx);
                }

                return result;
            }

            // Returns JSON Pointer from specified array
            // Array elements that are null are assumed to be insertion indexes ("-" elements)
            inline std::string array_to_path(const core::value &path)
            {
                std::string result_path;

                if (path.is_array())
                {
                    if (path.array_size() == 0)
                        result_path.push_back('/');

                    for (core::array_const_iterator_t item = path.get_array_unchecked().begin();
                         item.differs(path.get_array_unchecked().end());
                         ++item)
                    {
                        result_path.push_back('/');

                        switch (item->get_type())
                        {
                            default: break;
                            case core::null: result_path.push_back('-'); break;
                            case core::boolean: result_path += item->get_bool_unchecked()? "true": "false"; break;
                            case core::integer: result_path += stdx::to_string(item->get_int_unchecked()); break;
                            case core::uinteger: result_path += stdx::to_string(item->get_uint_unchecked()); break;
                            case core::real: result_path += stdx::to_string(item->get_real_unchecked()); break;
                            case core::string:
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                            case core::temporary_string:
#endif
                            {
                                core::string_t s = static_cast<core::string_t>(item->get_string());
                                for (size_t i = 0; i < s.size(); ++i)
                                {
                                    switch (s[i])
                                    {
                                        case '~': result_path += "~0"; break;
                                        case '/': result_path += "~1"; break;
                                        default: result_path.push_back(s[i]); break;
                                    }
                                }
                                break;
                            }
                        }
                    }
                }

                return result_path;
            }
        }
    }
}

#endif // CPPDATALIB_JSON_POINTER_H
