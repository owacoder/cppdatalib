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

#include <cassert>

namespace cppdatalib
{
    namespace core
    {
        class value_builder : public core::stream_handler
        {
            core::value &v;

            // WARNING: Underlying container type of `keys` MUST be able to maintain element positions
            // so their addresses don't change (i.e. NOT VECTOR)
            std::stack<core::value, std::list<core::value>> keys;
            std::stack<core::value *, std::vector<core::value *>> references;

        public:
            value_builder(core::value &bind) : v(bind) {}
            value_builder(const value_builder &builder)
                : v(builder.v)
            {
                assert(("cppdatalib::core::value_builder(const value_builder &) - attempted to copy a value_builder while active" && !builder.active()));
            }
            value_builder(value_builder &&builder)
                : v(builder.v)
            {
                assert(("cppdatalib::core::value_builder(value_builder &&) - attempted to move a value_builder while active" && !builder.active()));
            }

            const core::value &value() const {return v;}

        protected:
            // begin_() clears the bound value to null and pushes a reference to it
            void begin_()
            {
                keys = decltype(keys)();
                references = decltype(references)();

                v.set_null();
                references.push(&this->v);
            }

            // begin_key_() just queues a new object key in the stack
            void begin_key_(const core::value &v)
            {
                keys.push(v);
                references.push(&keys.top());
            }
            void end_key_(const core::value &)
            {
                if (references.empty())
                    end(), begin();

                references.pop();
            }

            // begin_scalar_() pushes the item to the array if the object to be modified is an array,
            // adds a member with the specified key, or simply assigns if not in a container
            void begin_scalar_(const core::value &v, bool is_key)
            {
                if (references.empty())
                    end(), begin();

                if (!is_key && current_container() == array)
                    references.top()->push_back(v);
                else if (!is_key && current_container() == object)
                {
                    references.top()->add_member(keys.top(), v);
                    keys.pop();
                }
                else
                    *references.top() = v;
            }

            void string_data_(const core::value &v, bool)
            {
                if (references.empty())
                    end(), begin();

                references.top()->get_string_ref() += v.get_string_unchecked();
            }

            // begin_container() operates similarly to begin_scalar_(), but pushes a reference to the container as well
            void begin_container(const core::value &v, core::int_t, bool is_key)
            {
                if (references.empty())
                    end(), begin();

                if (!is_key && current_container() == array)
                {
                    references.top()->push_back(core::null_t());
                    references.push(&references.top()->get_array_ref().data().back());
                }
                else if (!is_key && current_container() == object)
                {
                    references.push(&references.top()->add_member(keys.top()));
                    keys.pop();
                }

                // WARNING: If one tries to perform the following assignment `*references.top() = v` here,
                // an infinite recursion will result, because the `core::value` assignment operator and the
                // `core::value` copy constructor use this class to build complex (array or object) types.
                if (v.is_array())
                    references.top()->set_array(core::array_t(), v.get_subtype());
                else if (v.is_object())
                    references.top()->set_object(core::object_t(), v.get_subtype());
                else if (v.is_string())
                    references.top()->set_string(core::string_t(), v.get_subtype());
            }

            // end_container_() just removes a container from the stack, because nothing more needs to be done
            void end_container(bool is_key)
            {
                if (references.empty())
                    end(), begin();

                if (!is_key)
                    references.pop();
            }

            void begin_string_(const core::value &v, int_t size, bool is_key) {begin_container(v, size, is_key);}
            void end_string_(const core::value &, bool is_key) {end_container(is_key);}
            void begin_array_(const core::value &v, core::int_t size, bool is_key) {begin_container(v, size, is_key);}
            void end_array_(const core::value &, bool is_key) {end_container(is_key);}
            void begin_object_(const core::value &v, core::int_t size, bool is_key) {begin_container(v, size, is_key);}
            void end_object_(const core::value &, bool is_key) {end_container(is_key);}
        };

        inline value &value::assign(value &dst, const value &src)
        {
            switch (src.get_type())
            {
                case null: dst.set_null(src.get_subtype()); return dst;
                case boolean: dst.set_bool(src.get_bool_unchecked(), src.get_subtype()); return dst;
                case integer: dst.set_int(src.get_int_unchecked(), src.get_subtype()); return dst;
                case uinteger: dst.set_uint(src.get_uint_unchecked(), src.get_subtype()); return dst;
                case real: dst.set_real(src.get_real_unchecked(), src.get_subtype()); return dst;
                case string: dst.set_string(src.get_string_unchecked(), src.get_subtype()); return dst;
                case array:
                case object:
                {
                    value_builder builder(dst);
                    builder << src;
                    return dst;
                }
                default: dst.set_null(src.get_subtype()); return dst;
            }
        }

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

        // Convert directly from value to serializer
        inline const value &operator>>(const value &input, stream_handler &output)
        {
            output << input;
            return input;
        }

        // Convert directly from value to rvalue serializer
        inline const value &operator>>(const value &input, stream_handler &&output)
        {
            output << input;
            return input;
        }

        // Convert directly from parser to value
        inline stream_input &operator>>(stream_input &input, value &output)
        {
            value_builder builder(output);
            input.convert(builder);
            return input;
        }

        // Convert directly from parser to value
        inline void operator>>(stream_input &&input, value &output)
        {
            value_builder builder(output);
            input.convert(builder);
        }

        // Convert directly from parser to value
        inline value &operator<<(value &output, stream_input &input)
        {
            input >> output;
            return output;
        }

        // Convert directly from parser to value
        inline value &operator<<(value &output, stream_input &&input)
        {
            input >> output;
            return output;
        }

        inline bool operator<(const value &lhs, const value &rhs)
        {
            value::traverse_less_than_compare_prefix prefix;

            if (lhs.is_array() || lhs.is_object() || rhs.is_array() || rhs.is_object())
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

            if (lhs.is_array() || lhs.is_object() || rhs.is_array() || rhs.is_object())
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

            if (lhs.is_array() || lhs.is_object() || rhs.is_array() || rhs.is_object())
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
