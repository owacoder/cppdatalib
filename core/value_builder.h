/*
 * value_builder.h
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

#ifndef CPPDATALIB_VALUE_BUILDER_H
#define CPPDATALIB_VALUE_BUILDER_H

#include "stream_base.h"
#include <list>
#include <climits>
#include <cstddef>
#include <cstdio>

#include <cassert>

namespace cppdatalib
{
    namespace core
    {
        /* value_builder: This class is a built-in output format to build the input in a bound `value` variable.
         * This class is used internally to copy `value` instances. This class MUST NOT be copied while active()!!!
         */
        class value_builder : public core::stream_handler
        {
            core::value &v;

            // WARNING: Underlying container type of `keys` MUST be able to maintain element positions
            // so their addresses don't change (i.e. NOT VECTOR)
            typedef std::stack< core::value, std::list<core::value> > keys_t;
            typedef core::cache_vector_n<core::value *, core::cache_size> references_t;

            keys_t keys;
            core::value top_key;
            bool top_key_used;
            references_t references;

        public:
            value_builder(core::value &bind) : v(bind) {}
            value_builder(const value_builder &builder)
                : v(builder.v)
                , top_key_used(false)
            {
                assert(("cppdatalib::core::value_builder(const value_builder &) - attempted to copy a value_builder while active" && !builder.active()));
            }
#ifdef CPPDATALIB_CPP11
            value_builder(value_builder &&builder)
                : v(builder.v)
                , top_key_used(false)
            {
                assert(("cppdatalib::core::value_builder(value_builder &&) - attempted to move a value_builder while active" && !builder.active()));
            }
#endif

            const core::value &value() const {return v;}

            std::string name() const
            {
#ifdef CPPDATALIB_CPP11
                const size_t size = sizeof(ptrdiff_t) * CHAR_BIT;
                char n[size+1];

                snprintf(n, size, "%p", &v);
#else
                const uintmax_t ptr = reinterpret_cast<uintmax_t>(&v);

                std::string n = stdx::to_string(ptr);
#endif

                return std::string("cppdatalib::core::value_builder(") + n + ")";
            }

        protected:
            core::value *push_key()
            {
                if (top_key_used)
                {
                    keys.push(core::value());
                    return &keys.top();
                }

                top_key_used = true;
                return &top_key;
            }
            void pop_key()
            {
                if (keys.size())
                {
                    top_key.swap(keys.top());
                    keys.pop();
                }
                else
                    top_key_used = false;
            }

            // begin_() clears the bound value to null and pushes a reference to it
            void begin_()
            {
                keys = keys_t();
                references = references_t();

                top_key_used = false;

                v.set_null();
                references.push_back(&this->v);
            }

            // begin_key_() just queues a new object key in the stack
            void begin_key_(const core::value &)
            {
                references.push_back(push_key());
            }
            void end_key_(const core::value &)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                references.pop_back();
            }

            // begin_scalar_() pushes the item to the array if the object to be modified is an array,
            // adds a member with the specified key, or simply assigns if not in a container
            void begin_scalar_(const core::value &v, bool is_key)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                if (!is_key && current_container() == array)
                    references.back()->push_back(v);
                else if (!is_key && current_container() == object)
                {
                    references.back()->add_member(stdx::move(top_key), v);
                    pop_key();
                }
                else
                    *references.back() = v;
            }

            void string_data_(const core::value &v, bool)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                references.back()->get_owned_string_ref() += v.get_string_unchecked();
            }

#ifdef CPPDATALIB_CPP11
            void string_data_(core::value &&v, bool)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                if (v.is_owned_string() && references.back()->string_size() == 0)
                    references.back()->get_owned_string_ref() = std::move(v.get_owned_string_ref());
                else
                    references.back()->get_owned_string_ref() += v.get_string_unchecked();
            }
#endif

            // begin_container() operates similarly to begin_scalar_(), but pushes a reference to the container as well
            void begin_container(const core::value &v, optional_size size, bool is_key)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                if (!is_key && current_container() == array)
                {
                    references.back()->push_back(core::null_t());
                    references.push_back(&references.back()->get_array_ref().data().back());
                }
                else if (!is_key && current_container() == object)
                {
                    references.push_back(&references.back()->add_member(stdx::move(top_key)));
                    pop_key();
                }

                // WARNING: If one tries to perform the following assignment `*references.back() = v` here,
                // an infinite recursion will result, because the `core::value` assignment operator and the
                // `core::value` copy constructor use this class to build complex (array or object) types.
                if (v.is_array())
                {
                    references.back()->set_array(core::array_t(), v.get_subtype());
                    if (size.has_value() && size.value() < std::numeric_limits<size_t>::max())
                        references.back()->get_array_ref().data().reserve(size.value());
                }
                else if (v.is_object())
                    references.back()->set_object(core::object_t(), v.get_subtype());
                else if (v.is_string())
                    references.back()->set_string(core::string_t(), v.get_subtype());

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                if (v.attributes_size())
                    references.back()->set_attributes(v.get_attributes());
#endif
            }

            // end_container() just removes a container from the stack, because nothing more needs to be done
            void end_container(bool is_key)
            {
                if (references.empty())
                {
                    end();
                    begin();
                }

                if (!is_key)
                    references.pop_back();
            }

            void begin_string_(const core::value &v, core::optional_size size, bool is_key) {begin_container(v, size, is_key);}
            void end_string_(const core::value &, bool is_key) {end_container(is_key);}
            void begin_array_(const core::value &v, core::optional_size size, bool is_key) {begin_container(v, size, is_key);}
            void end_array_(const core::value &, bool is_key) {end_container(is_key);}
            void begin_object_(const core::value &v, core::optional_size size, bool is_key) {begin_container(v, size, is_key);}
            void end_object_(const core::value &, bool is_key) {end_container(is_key);}
        };

        inline value &value::assign(value &dst, const value &src)
        {
            switch (src.get_type())
            {
                case null: dst.set_null(src.get_subtype()); break;
                // Links are weakened in the destination when copying a link object (i.e. the destination is a weak link to the source, which is a strong link)
                case link: dst.set_link(src.get_link_unchecked(), src.get_subtype() == strong_link? normal: (subtype) src.get_subtype()); break;
                case boolean: dst.set_bool(src.get_bool_unchecked(), src.get_subtype()); break;
                case integer: dst.set_int(src.get_int_unchecked(), src.get_subtype()); break;
                case uinteger: dst.set_uint(src.get_uint_unchecked(), src.get_subtype()); break;
                case real: dst.set_real(src.get_real_unchecked(), src.get_subtype()); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: dst.set_temp_string(src.get_temp_string_unchecked(), src.get_subtype()); break;
#endif
                case string:
                {
                    if (src.is_nonnull_owned_string())
                        dst.set_string(static_cast<core::string_t>(src.get_string_unchecked()), src.get_subtype());
                    else
                        dst.set_string(core::string_t(), src.get_subtype());
                    break;
                }
                case array:
                {
                    if (src.is_nonnull_array())
                    {
                        value_builder builder(dst);
                        builder << src;
                    }
                    else
                        dst.set_array(core::array_t(), src.get_subtype());
                    break;
                }
                case object:
                {
                    if (src.is_nonnull_object())
                    {
                        value_builder builder(dst);
                        builder << src;
                    }
                    else
                        dst.set_object(core::object_t(), src.get_subtype());
                    break;
                }
            }

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            if (dst.attr_ && src.attr_)
                *dst.attr_ = *src.attr_;
            else if (dst.attr_)
                dst.attr_->data().clear();
            else if (src.attr_)
                dst.attr_ = new object_t(*src.attr_);
#endif

            return dst;
        }

#ifdef CPPDATALIB_CPP11
        inline value &value::assign(value &dst, value &&src)
        {
            using namespace std;

            dst.clear(src.get_type());
            switch (src.get_type())
            {
                case null: break;
                case boolean: dst.bool_ = std::move(src.bool_); break;
                case integer: dst.int_ = std::move(src.int_); break;
                case uinteger: dst.uint_ = std::move(src.uint_); break;
                case real: dst.real_ = std::move(src.real_); break;
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                case temporary_string: dst.tstr_ = src.tstr_; break;
#endif
#ifndef CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
                case string:
                    if (src.string_size())
                        dst.str_ref_() = std::move(src.str_ref_());
                    break;
#else
                case string:
#endif
                case array:
                case object:
                case link:
                    std::swap(dst.ptr_, src.ptr_);
                    break;
            }
            dst.subtype_ = src.get_subtype();
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
            std::swap(dst.attr_, src.attr_);
#endif
            return dst;
        }
#endif // CPPDATALIB_CPP11

        // Convert directly from value to serializer
        inline stream_handler &operator<<(stream_handler &output, const value &input)
        {
            const bool stream_ready = output.active();
            value::traverse_node_prefix_serialize prefix(&output);
            value::traverse_node_postfix_serialize postfix(&output);

            if (!stream_ready)
                output.begin();
            input.traverse(prefix, postfix);
            if (!stream_ready)
                output.end();

            return output;
        }
        inline stream_handler &convert(stream_handler &output, const value &input) {return output << input;}

#ifdef CPPDATALIB_CPP11
        // Convert directly from value to rvalue serializer
        inline void operator<<(stream_handler &&output, const value &input)
        {
            const bool stream_ready = output.active();
            value::traverse_node_prefix_serialize prefix(&output);
            value::traverse_node_postfix_serialize postfix(&output);

            if (!stream_ready)
                output.begin();
            input.traverse(prefix, postfix);
            if (!stream_ready)
                output.end();
        }
        inline void convert(stream_handler &&output, const value &input) {return std::move(output) << input;}
#endif

        // Convert directly from value to serializer
        inline const value &operator>>(const value &input, stream_handler &output)
        {
            output << input;
            return input;
        }
        inline const value &convert(const value &input, stream_handler &output) {return input >> output;}

#ifdef CPPDATALIB_CPP11
        // Convert directly from value to rvalue serializer
        inline const value &operator>>(const value &input, stream_handler &&output)
        {
            output << input;
            return input;
        }
        inline const value &convert(const value &input, stream_handler &&output) {return input >> std::move(output);}
#endif

        // Convert directly from parser to value
        inline stream_input &operator>>(stream_input &input, value &output)
        {
            value_builder builder(output);
            input.convert(builder);
            return input;
        }
        inline stream_input &convert(stream_input &input, value &output) {return input >> output;}

#ifdef CPPDATALIB_CPP11
        // Convert directly from parser to value
        inline void operator>>(stream_input &&input, value &output)
        {
            value_builder builder(output);
            input.convert(builder);
        }
        inline void convert(stream_input &&input, value &output) {return std::move(input) >> output;}
#endif

        // Convert directly from parser to value
        inline value &operator<<(value &output, stream_input &input)
        {
            input >> output;
            return output;
        }
        inline value &convert(value &output, stream_input &input) {return output << input;}

#ifdef CPPDATALIB_CPP11
        // Convert directly from parser to value
        inline value &operator<<(value &output, stream_input &&input)
        {
            input >> output;
            return output;
        }
        inline value &convert(value &output, stream_input &&input) {return output << std::move(input);}
#endif

        inline bool operator<(const value &lhs, const value &rhs)
        {
            value::traverse_less_than_compare_prefix prefix;

            if (lhs.is_nonnull_array() || lhs.is_nonnull_object() || rhs.is_nonnull_array() || rhs.is_nonnull_object())
            {
                value::traverse_compare_postfix postfix;

                lhs.parallel_traverse(rhs, prefix, postfix);
            }
            else
                prefix.run(&lhs, &rhs);

            return prefix.comparison() < 0;
        }

        inline bool operator<=(const value &lhs, const value &rhs)
        {
            value::traverse_compare_prefix prefix;

            if (lhs.is_nonnull_array() || lhs.is_nonnull_object() || rhs.is_nonnull_array() || rhs.is_nonnull_object())
            {
                value::traverse_compare_postfix postfix;

                lhs.parallel_traverse(rhs, prefix, postfix);
            }
            else
                prefix.run(&lhs, &rhs);

            return prefix.comparison() <= 0;
        }

        inline bool operator==(const value &lhs, const value &rhs)
        {
            value::traverse_equality_compare_prefix prefix;

            if (lhs.is_nonnull_array() || lhs.is_nonnull_object() || rhs.is_nonnull_array() || rhs.is_nonnull_object())
            {
                value::traverse_compare_postfix postfix;

                lhs.parallel_traverse(rhs, prefix, postfix);
            }
            else
                prefix.run(&lhs, &rhs);

            return prefix.comparison_equal();
        }

        inline bool operator!=(const value &lhs, const value &rhs)
        {
            return !(lhs == rhs);
        }

        inline bool operator>(const value &lhs, const value &rhs)
        {
            return !(lhs <= rhs);
        }

        inline bool operator>=(const value &lhs, const value &rhs)
        {
            return !(lhs < rhs);
        }
    }
}

#endif // CPPDATALIB_VALUE_BUILDER_H
