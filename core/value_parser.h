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

                            references.push(value::traversal_reference(p, p->get_array_unchecked().begin(), object_t::const_iterator(), false));
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
