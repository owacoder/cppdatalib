/*
 * xml.h
 *
 * Copyright © 2018 Oliver Adams
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

#ifndef CPPDATALIB_XML_XML_H
#define CPPDATALIB_XML_XML_H

#ifdef CPPDATALIB_ENABLE_XML
#include "../core/core.h"

#ifndef CPPDATALIB_ENABLE_ATTRIBUTES
#error The XML library requires CPPDATALIB_ENABLE_ATTRIBUTES to be defined
#endif

namespace cppdatalib
{
    namespace xml
    {
        class parser : public core::xml_impl::stream_parser
        {
            std::unique_ptr<char []> buffer;
            bool require_full_document;

            core::string_t content;

            enum {
                ready_to_read_prolog,
                ready_to_read_elements,
                reading_elements
            } state;

        public:
            parser(core::istream_handle input, bool require_full_document = true)
                : core::xml_impl::stream_parser(input)
                , buffer(new char [core::buffer_size + core::max_utf8_code_sequence_size + 1])
                , require_full_document(require_full_document)
            {
                reset();
            }

            bool busy() const {return get_output() && state == reading_elements;}

        protected:
            void reset_()
            {
                state = require_full_document? ready_to_read_prolog: ready_to_read_elements;
                content.clear();
                core::xml_impl::stream_parser::reset_();
            }

            void begin_tag()
            {
                if (nesting_depth() > 0 && get_output()->current_container() != core::array)
                    // We have to start an array for mixed content
                    get_output()->begin_array(core::value(core::array_t()), core::stream_handler::unknown_size);

                if (content.empty())
                    return;

                get_output()->write(core::value(std::move(content)));
                content = core::string_t();
            }

            void end_tag()
            {
                if (!content.empty())
                {
                    get_output()->write(core::value(std::move(content)));
                    content = core::string_t();
                }

                if (nesting_depth() > 0 &&
                        get_output()->current_container() == core::array)
                    get_output()->end_array(core::value(core::array_t()));

                if (nesting_depth() > 0 &&
                        get_output()->current_container() == core::object)
                    get_output()->end_object(core::value(core::object_t()));
            }

            void write_one_()
            {
                switch (state)
                {
                    case ready_to_read_prolog:
                    {
                        core::value attributes;
                        if (!read_prolog(attributes))
                            throw core::custom_error("XML - " + last_error());

                        if (attributes["version"].get_string().substr(0, 2) != "1.")
                            throw core::error("XML - unsupported version");

                        if (attributes["encoding"].get_string() != "UTF-8")
                            throw core::error("XML - unsupported encoding");

                        state = reading_elements;
                        break;
                    }
                    case ready_to_read_elements:
                        state = reading_elements;
                        // Fallthrough
                    case reading_elements:
                    {
                        what_was_read read;
                        core::string_t string;
                        core::value value;

                        if (!read_next(current_element_stack().size(), read, string, value))
                            throw core::custom_error("XML - " + last_error());

                        switch (read)
                        {
                            case eof_was_reached:
                                state = require_full_document? ready_to_read_prolog: ready_to_read_elements;
                                break;
                            case start_tag_was_read:
                                begin_tag();
                                get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                                get_output()->write(value);
                                break;
                            case end_tag_was_read:
                                end_tag();
                                break;
                            case complete_tag_was_read:
                                begin_tag();
                                get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                                get_output()->write(core::value(value));
                                get_output()->write(core::value());
                                end_tag();
                                break;
                            case content_was_read:
                                content += string;
                                break;
                            case nothing_was_read:
                                break;
                            case comment_was_read:
                                std::cout << "<!--" << string << "-->\n";
                                break;
                            case processing_instruction_was_read:
                            default:
                                break; // Discard, we don't need them right now
                        }
                    }
                    default: break;
                }
            }
        };

        // Use an array to counteract the lack of ordering when using objects
        // Array elements that are not objects are concatenated together, which may or may not be the desired operation
        class stream_writer : public core::xml_impl::stream_writer_base
        {
            core::cache_vector_n<core::string_t, core::cache_size> current_keys;

        public:
            stream_writer(core::ostream_handle output) : core::xml_impl::stream_writer_base(output) {}

            std::string name() const {return "cppdatalib::xml::stream_writer";}

        protected:
            void begin_() {stream().precision(CPPDATALIB_REAL_DIG); current_keys.clear();}

            void begin_item_(const core::value &v)
            {
                if (container_key_was_just_parsed())
                {
                    write_attributes(v);
                    stream().put('>');
                }
            }

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML - cannot write non-string key");

                if (current_container_size() > 0)
                {
                    write_name(stream().write("</", 2), current_keys.back()).put('>');
                    current_keys.pop_back();
                }

                stream().put('<');
                current_keys.push_back("");
            }
            void end_key_(const core::value &v)
            {
                write_attributes(v);
            }

            // Implementation of null_() does nothing
            // void null_(const core::value &) {}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("XML - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    current_keys.back() += v.get_string_unchecked();
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    write_name(stream(), current_keys.back());
            }

            void end_object_(const core::value &, bool)
            {
                write_name(stream().write("</", 2), current_keys.back()).put('>');
                current_keys.pop_back();
            }
        };

        class pretty_stream_writer : public core::xml_impl::stream_writer_base
        {
            std::unique_ptr<char []> buffer;
            size_t indent_width;
            size_t current_indent;
            core::cache_vector_n<core::string_t, core::cache_size> current_keys;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    size_t size = std::min(size_t(core::buffer_size-1), padding);

                    memset(buffer.get(), ' ', size);

                    stream().write(buffer.get(), size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(core::ostream_handle output, size_t indent_width)
                : core::xml_impl::stream_writer_base(output)
                , buffer(new char [core::buffer_size])
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

            std::string name() const {return "cppdatalib::xml::pretty_stream_writer";}

        protected:
            void begin_() {current_indent = 0; stream().precision(CPPDATALIB_REAL_DIG); current_keys.clear();}

            void begin_item_(const core::value &v)
            {
                if (container_key_was_just_parsed())
                {
                    write_attributes(v);
                    stream().put('>');
                    stream().put('\n'), output_padding(current_indent);
                }
                else if (current_container() == core::array) // Must be using an array to counteract the lack of ordering when using objects
                {
                    // Array elements that are not objects are concatenated together, which may or may not be the desired operation

                    if (current_container_size() > 0)
                        stream().put('\n'), output_padding(current_indent);
                }
            }

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML - cannot write non-string key");

                if (current_container_size() > 0)
                {
                    stream().put('\n'), output_padding(current_indent);
                    write_name(stream().write("</", 2), current_keys.back()).put('>');
                    current_keys.pop_back();
                }

                stream().put('<');
                current_keys.push_back("");
            }
            void end_key_(const core::value &v)
            {
                write_attributes(v);
            }

            // Implementation of null_() does nothing
            // void null_(const core::value &) {}
            void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
            void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("XML - cannot write 'NaN' or 'Infinity' values");
                stream() << v.get_real_unchecked();
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    current_keys.back() += v.get_string_unchecked();
                else
                    write_element_content(stream(), v.get_string_unchecked());
            }
            void end_string_(const core::value &, bool is_key)
            {
                if (is_key)
                    write_name(stream(), current_keys.back());
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                stream().put('\n'), output_padding(current_indent);
                write_name(stream().write("</", 2), current_keys.back()).put('>');
                current_keys.pop_back();
            }
        };

        class document_writer : public stream_writer
        {
        public:
            document_writer(core::ostream_handle output) : stream_writer(output) {}

        protected:
            void begin_()
            {
                stream() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

                stream_writer::begin_();
            }
        };

        class pretty_document_writer : public pretty_stream_writer
        {
        public:
            pretty_document_writer(core::ostream_handle output, size_t indent_width) : pretty_stream_writer(output, indent_width) {}

        protected:
            void begin_()
            {
                stream() << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

                pretty_stream_writer::begin_();
            }
        };

        inline core::value from_xml(core::istream_handle stream)
        {
            parser reader(stream);
            core::value v;
            reader >> v;
            return v;
        }

        inline core::value operator "" _xml(const char *stream, size_t size)
        {
            core::istringstream wrap(std::string(stream, size));
            return from_xml(wrap);
        }

        inline std::string to_xml_elements(const core::value &v)
        {
            core::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_xml_elements(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }

        inline std::string to_xml_document(const core::value &v)
        {
            core::ostringstream stream;
            document_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_xml_document(const core::value &v, size_t indent_width)
        {
            core::ostringstream stream;
            pretty_document_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_ENABLE_XML
#endif // CPPDATALIB_XML_XML_H
