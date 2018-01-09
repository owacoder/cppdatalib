/*
 * value_parser.h
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

#ifndef CPPDATALIB_VALUE_PARSER_H
#define CPPDATALIB_VALUE_PARSER_H

#include "value_builder.h"

namespace cppdatalib
{
    namespace core
    {
        class value_parser : public core::stream_input
        {
            std::stack<value::traversal_reference, std::vector<value::traversal_reference>> references;
            const value *bind;
            const value *p;

        public:
            value_parser(const value &bind)
                : bind(&bind)
            {
                reset();
            }

            unsigned int features() const {return provides_buffered_arrays |
                                                  provides_buffered_objects |
                                                  provides_buffered_strings |
                                                  provides_prefix_array_size |
                                                  provides_prefix_object_size |
                                                  provides_prefix_string_size;}

            void reset()
            {
                references = decltype(references)();
                p = bind;
            }

        protected:
            void write_one_()
            {
                if (!references.empty() || p != NULL)
                {
                    if (p != NULL)
                    {
                        if (p->is_array())
                        {
                            get_output()->begin_array(*p, p->array_size());

                            references.push(value::traversal_reference(p, p->get_array_unchecked().begin(), object_const_iterator_t(), false));
                            if (!p->get_array_unchecked().empty())
                                p = std::addressof(*references.top().array++);
                            else
                                p = NULL;
                        }
                        else if (p->is_object())
                        {
                            get_output()->begin_object(*p, p->object_size());

                            references.push(value::traversal_reference(p, array_t::const_iterator(), p->get_object_unchecked().begin(), true));
                            if (!p->get_object_unchecked().empty())
                                p = std::addressof(references.top().object->first);
                            else
                                p = NULL;
                        }
                        else
                        {
                            get_output()->write(*p);
                            p = NULL;
                        }
                    }
                    else
                    {
                        const value *peek = references.top().p;
                        if (peek->is_array() && references.top().array != peek->get_array_unchecked().end())
                            p = std::addressof(*references.top().array++);
                        else if (peek->is_object() && references.top().object != peek->get_object_unchecked().end())
                        {
                            if (!references.top().traversed_key_already)
                                p = std::addressof(references.top().object->first);
                            else
                                p = std::addressof((references.top().object++)->second);

                            references.top().traversed_key_already = !references.top().traversed_key_already;
                        }
                        else
                        {
                            references.pop();

                            if (peek->is_array())
                                get_output()->end_array(*peek);
                            else /* Object (scalars are not pushed to references stack) */
                                get_output()->end_object(*peek);
                        }
                    }
                }
            }
        };
    }
}

#endif // CPPDATALIB_VALUE_PARSER_H
