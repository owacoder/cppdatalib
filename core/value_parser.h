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

        protected:
            void reset_()
            {
                references = decltype(references)();
                p = bind;
            }

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

        // Forward-declare generic_parser
        class generic_parser;

        // Base class for all generic parser helper classes
        struct generic_stream_input : public core::stream_input
        {
        private:
            generic_parser * const master_parser;

        public:
            generic_stream_input(generic_parser &parser);

        protected:
            template<typename ForItem>
            void compose_parser(const ForItem &item);
        };

        // Parser for a generic scalar type. Can be specialized for different scalar types
        template<typename T>
        class type_parser : public generic_stream_input
        {
        protected:
            const T &bind;

        public:
            type_parser(const T &bind, generic_parser &parser)
                : generic_stream_input(parser)
                , bind(bind)
            {
                reset();
            }

            unsigned int features() const {return provides_buffered_arrays |
                                                  provides_buffered_objects |
                                                  provides_buffered_strings |
                                                  provides_prefix_array_size |
                                                  provides_prefix_object_size |
                                                  provides_prefix_string_size;}

        protected:
            void reset_() {}

            void write_one_()
            {
                get_output()->write(core::value(bind));
            }
        };

        // Parser for a generic template type. Assumes template type is an array type
        // supporting begin(), end(), and size().
        // Can be specialized for different functionality
        template<template<size_t, typename...> class Template, size_t N, typename... Ts>
        class sized_template_parser : public generic_stream_input
        {
        protected:
            const Template<N, Ts...> *bind;
            decltype(bind->begin()) iterator;

        public:
            sized_template_parser(const Template<N, Ts...> &bind, generic_parser &parser)
                : generic_stream_input(parser)
                , bind(&bind)
            {
                reset();
            }

        protected:
            void reset_() {iterator = bind->begin();}

            void write_one_();
        };

        // Parser for a generic template type. Assumes template type is an array type
        // supporting begin(), end(), and size().
        // Can be specialized for different functionality
        template<template<typename...> class Template, typename... Ts>
        class template_parser : public generic_stream_input
        {
        protected:
            const Template<Ts...> *bind;
            decltype(bind->begin()) iterator;

        public:
            template_parser(const Template<Ts...> &bind, generic_parser &parser)
                : generic_stream_input(parser)
                , bind(&bind)
            {
                reset();
            }

        protected:
            void reset_() {iterator = bind->begin();}

            void write_one_();
        };

        // Parser for a generic template type. Assumes template type is an array type
        // supporting begin(), end(), and size().
        // Can be specialized for different functionality
        template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
        class array_template_parser : public generic_stream_input
        {
        protected:
            const Template<T, N, Ts...> *bind;
            decltype(bind->begin()) iterator;

        public:
            array_template_parser(const Template<T, N, Ts...> &bind, generic_parser &parser)
                : generic_stream_input(parser)
                , bind(&bind)
            {
                reset();
            }

        protected:
            void reset_() {iterator = bind->begin();}

            void write_one_();
        };

        namespace impl
        {
            class impl_generic_parser
            {
                std::unique_ptr<core::stream_input> input;

            public:
                template<typename T>
                impl_generic_parser(const T &bind, core::stream_handler *output, generic_parser &parser)
                {
                    input = std::unique_ptr<type_parser<T>>(new type_parser<T>(bind, parser));
                    if (output != nullptr)
                        input->set_output(*output);
                }

                template<template<size_t, typename...> class Template, size_t N, typename... Ts>
                impl_generic_parser(const Template<N, Ts...> &bind, core::stream_handler *output, generic_parser &parser)
                {
                    input = std::unique_ptr<sized_template_parser<Template, N, Ts...>>(new sized_template_parser<Template, N, Ts...>(bind, parser));
                    if (output != nullptr)
                        input->set_output(*output);
                }

                template<template<typename...> class Template, typename... Ts>
                impl_generic_parser(const Template<Ts...> &bind, core::stream_handler *output, generic_parser &parser)
                {
                    input = std::unique_ptr<template_parser<Template, Ts...>>(new template_parser<Template, Ts...>(bind, parser));
                    if (output != nullptr)
                        input->set_output(*output);
                }

                template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
                impl_generic_parser(const Template<T, N, Ts...> &bind, core::stream_handler *output, generic_parser &parser)
                {
                    input = std::unique_ptr<array_template_parser<Template, T, N, Ts...>>(new array_template_parser<Template, T, N, Ts...>(bind, parser));
                    if (output != nullptr)
                        input->set_output(*output);
                }

                core::stream_input &parser() {return *input;}
                const core::stream_input &parser() const {return *input;}
            };
        }

        // Generic parser type, pass it a value of any type and it will be parsed to the specified output handler
        class generic_parser : public core::stream_input
        {
            std::vector<impl::impl_generic_parser> stack;

        public:
            template<typename ForItem>
            generic_parser(const ForItem &bind)
            {
                stack.push_back(impl::impl_generic_parser(bind, nullptr, *this));
                reset();
            }

            template<typename ForItem>
            generic_parser(const ForItem &bind, core::stream_handler &output) : core::stream_input(output)
            {
                stack.push_back(impl::impl_generic_parser(bind, &output, *this));
                reset();
            }

            template<typename ForItem>
            void compose_parser(const ForItem &bind)
            {
                stack.push_back(impl::impl_generic_parser(bind, get_output(), *this));
            }

            bool busy() const {return core::stream_input::busy() || stack.size() > 1;}

        protected:
            void output_changed_()
            {
                for (auto &parser: stack)
                    parser.parser().set_output(*get_output());
            }

            void reset_()
            {
                while (stack.size() > 1) stack.pop_back();
                stack.back().parser().reset();
            }

            void write_one_()
            {
                while (!stack.back().parser().was_just_reset() && !stack.back().parser().busy() && stack.size() > 1)
                    stack.pop_back();
                stack.back().parser().write_one();
            }
        };

        template<template<size_t, typename...> class Template, size_t N, typename... Ts>
        void sized_template_parser<Template, N, Ts...>::write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_array(core::array_t(), bind->size());
                if (iterator != bind->end())
                    compose_parser(*iterator++);
            }
            else if (iterator != bind->end())
                compose_parser(*iterator++);
            else
                get_output()->end_array(core::array_t());
        }

        template<template<typename...> class Template, typename... Ts>
        void template_parser<Template, Ts...>::write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_array(core::array_t(), bind->size());
                if (iterator != bind->end())
                    compose_parser(*iterator++);
            }
            else if (iterator != bind->end())
                compose_parser(*iterator++);
            else
                get_output()->end_array(core::array_t());
        }

        template<template<typename, size_t, typename...> class Template, typename T, size_t N, typename... Ts>
        void array_template_parser<Template, T, N, Ts...>::write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_array(core::array_t(), bind->size());
                if (iterator != bind->end())
                    compose_parser(*iterator++);
            }
            else if (iterator != bind->end())
                compose_parser(*iterator++);
            else
                get_output()->end_array(core::array_t());
        }

        inline generic_stream_input::generic_stream_input(generic_parser &parser)
            : master_parser(&parser)
        {
            if (parser.get_output())
                set_output(*parser.get_output());
        }

        template<typename ForItem>
        void generic_stream_input::compose_parser(const ForItem &item)
        {
            master_parser->compose_parser(item);
        }
    }
}

#endif // CPPDATALIB_VALUE_PARSER_H
