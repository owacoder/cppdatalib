/*
 * dump.h
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

#ifndef CPPDATALIB_DUMP_H
#define CPPDATALIB_DUMP_H

#include "stream_base.h"

namespace cppdatalib
{
    namespace core
    {
        namespace dump
        {
            namespace impl
            {
                class stream_writer_base : public core::stream_handler, public core::stream_writer
                {
                public:
                    stream_writer_base(core::ostream_handle &stream) : core::stream_writer(stream) {}

                protected:
                    core::ostream &write_string(core::ostream &stream, core::string_view_t str)
                    {
                        for (size_t i = 0; i < str.size(); ++i)
                        {
                            int c = str[i] & 0xff;

                            switch (c)
                            {
                                case '"':
                                case '\\':
                                    stream.put('\\');
                                    stream.put(c);
                                    break;
                                default:
                                    if (iscntrl(c))
                                    {
                                        switch (c)
                                        {
                                            case '\b': stream.write("\\b", 2); break;
                                            case '\f': stream.write("\\f", 2); break;
                                            case '\n': stream.write("\\n", 2); break;
                                            case '\r': stream.write("\\r", 2); break;
                                            case '\t': stream.write("\\t", 2); break;
                                            default: hex::write(stream.write("\\u00", 4), c); break;
                                        }
                                    }
                                    else
                                        stream.put(c);
                                    break;
                            }
                        }

                        return stream;
                    }

                    core::ostream &write_type(core::ostream &stream, core::type type)
                    {
                        switch (type)
                        {
                            case null: return stream << "null.";
                            case link: return stream << "link.";
                            case boolean: return stream << "boolean.";
                            case integer: return stream << "integer.";
                            case uinteger: return stream << "uinteger.";
                            case real: return stream << "real.";
#ifndef CPPDATALIB_DISABLE_TEMP_STRING
                            case temporary_string: return stream << "temporary_string.";
#endif
                            case string: return stream << "string.";
                            case array: return stream << "array.";
                            case object: return stream << "object.";
                        }

                        return stream << "unknown.";
                    }

                    core::ostream &write_subtype(core::ostream &stream, core::subtype_t subtype)
                    {
                        if (subtype == normal)
                            return stream;
                        if (subtype_is_reserved(subtype, &subtype))
                            return stream << "reserved " << subtype << ".";
                        else if (subtype_is_user_defined(subtype, &subtype))
                            return stream << "user " << subtype << ".";
                        else
                            return stream << subtype_to_string(subtype) << ".";
                    }
                };

                class attribute_stream_writer : public impl::stream_writer_base
                {
                    bool is_link_name;

                public:
                    attribute_stream_writer(core::ostream_handle output)
                        : stream_writer_base(output)
                    {}

                    std::string name() const {return "cppdatalib::dump::attribute_stream_writer";}

                protected:
                    void begin_() {stream().precision(CPPDATALIB_REAL_DIG); is_link_name = false;}

                    void begin_item_(const core::value &v)
                    {
                        if (container_key_was_just_parsed())
                            stream().put('=');
                        else if (current_container_size() > 0)
                            stream().put(',');

                        write_type(stream(), v.get_type());
                        write_subtype(stream(), v.get_subtype());

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        if (v.attributes_size())
                        {
                            impl::attribute_stream_writer writer(stream());
                            stream() << "[attributes=";
                            writer << core::value(v.get_attributes());
                            stream() << "].";
                        }
#endif
                    }
                    void begin_key_(const core::value &v)
                    {
                        if (current_container_size() > 0)
                            stream().put(',');

                        write_type(stream(), v.get_type());
                        write_subtype(stream(), v.get_subtype());

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        if (v.attributes_size())
                        {
                            impl::attribute_stream_writer writer(stream());
                            stream() << "[attributes=";
                            writer << core::value(v.get_attributes());
                            stream() << "].";
                        }
#endif
                    }

                    void null_(const core::value &) {stream() << "null";}
                    void link_(const core::value &v)
                    {
                        if (v.is_strong_link())
                        {
                            impl::attribute_stream_writer writer(stream());
                            writer << *v;
                        }
                        else
                        {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                            if (v.link_name_is_global())
                            {
                                stream() << "global(";
                                impl::attribute_stream_writer writer(stream());
                                writer << v.get_link_name();
                                stream() << ").";
                            }
#endif
                            stream() << reinterpret_cast<uint64_t>(v.get_link_unchecked());
                        }
                    }
                    void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
                    void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
                    void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
                    void real_(const core::value &v)
                    {
                        if (std::isinf(v.get_real_unchecked()))
                            stream() << (v.get_real_unchecked() < 0? "-": "") << "infinity";
                        else if (std::isnan(v.get_real_unchecked()))
                            stream() << "NaN";
                        else
                            stream() << v.get_real_unchecked();
                    }
                    void begin_string_(const core::value &v, core::int_t, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}
                    void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
                    void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}

                    void begin_array_(const core::value &, core::int_t, bool) {stream().put('[');}
                    void end_array_(const core::value &, bool) {stream().put(']');}

                    void begin_object_(const core::value &, core::int_t, bool) {stream().put('{');}
                    void end_object_(const core::value &, bool) {stream().put('}');}
                };
            }

            class stream_writer : public impl::stream_writer_base
            {
                std::unique_ptr<char []> buffer;
                size_t indent_width;
                size_t current_indent;
                bool nested;

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

                stream_writer(core::ostream_handle output, size_t indent_width, size_t current_indent)
                    : stream_writer_base(output)
                    , buffer(new char [core::buffer_size])
                    , indent_width(indent_width)
                    , current_indent(current_indent)
                    , nested(true)
                {}

            public:
                stream_writer(core::ostream_handle output, size_t indent_width)
                    : stream_writer_base(output)
                    , buffer(new char [core::buffer_size])
                    , indent_width(indent_width)
                    , current_indent(0)
                    , nested(false)
                {}

                size_t indent() {return indent_width;}

                std::string name() const {return "cppdatalib::dump::stream_writer";}

            protected:
                void begin_()
                {
                    if (!nested)
                    {
                        current_indent = 0;
                        stream().precision(CPPDATALIB_REAL_DIG);
                        stream() << "=== CPPDATALIB DUMP ===\n";
                    }
                }
                void end_()
                {
                    if (!nested)
                        stream() << "\n=== END CPPDATALIB DUMP ===\n";
                }

                void begin_item_(const core::value &v)
                {
                    if (container_key_was_just_parsed())
                        stream() << " = ";
                    else if (current_container_size() > 0)
                        stream().put(',');

                    if (current_container() == core::array)
                        stream().put('\n'), output_padding(current_indent);

                    write_type(stream(), v.get_type());
                    write_subtype(stream(), v.get_subtype());

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    if (v.attributes_size())
                    {
                        impl::attribute_stream_writer writer(stream());
                        stream() << "[attributes=";
                        writer << core::value(v.get_attributes());
                        stream() << "].";
                    }
#endif
                }
                void begin_key_(const core::value &v)
                {
                    if (current_container_size() > 0)
                        stream().put(',');

                    stream().put('\n'), output_padding(current_indent);

                    write_type(stream(), v.get_type());
                    write_subtype(stream(), v.get_subtype());

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    if (v.attributes_size())
                    {
                        impl::attribute_stream_writer writer(stream());
                        stream() << "[attributes=";
                        writer << core::value(v.get_attributes());
                        stream() << "].";
                    }
#endif
                }

                void null_(const core::value &) {stream() << "null";}
                void link_(const core::value &v)
                {
                    if (v.is_strong_link())
                    {
                        stream_writer writer(stream(), indent_width, current_indent);
                        writer << *v;
                    }
                    else
                    {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                        if (v.link_name_is_global())
                        {
                            stream() << "global(";
                            impl::attribute_stream_writer writer(stream());
                            writer << v.get_link_name();
                            stream() << ").";
                        }
#endif
                        stream() << reinterpret_cast<uint64_t>(v.get_link_unchecked());
                    }
                }
                void bool_(const core::value &v) {stream() << (v.get_bool_unchecked()? "true": "false");}
                void integer_(const core::value &v) {stream() << v.get_int_unchecked();}
                void uinteger_(const core::value &v) {stream() << v.get_uint_unchecked();}
                void real_(const core::value &v)
                {
                    if (std::isinf(v.get_real_unchecked()))
                        stream() << (v.get_real_unchecked() < 0? "-": "") << "infinity";
                    else if (std::isnan(v.get_real_unchecked()))
                        stream() << "NaN";
                    else
                        stream() << v.get_real_unchecked();
                }
                void begin_string_(const core::value &v, core::int_t, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}
                void string_data_(const core::value &v, bool) {write_string(stream(), v.get_string_unchecked());}
                void end_string_(const core::value &v, bool is_key) {if (v.get_subtype() != core::bignum || is_key) stream().put('"');}

                void begin_array_(const core::value &, core::int_t, bool)
                {
                    stream().put('[');
                    current_indent += indent_width;
                }
                void end_array_(const core::value &, bool)
                {
                    current_indent -= indent_width;

                    if (current_container_size() > 0)
                        stream().put('\n'), output_padding(current_indent);

                    stream().put(']');
                }

                void begin_object_(const core::value &, core::int_t, bool)
                {
                    stream().put('{');
                    current_indent += indent_width;
                }
                void end_object_(const core::value &, bool)
                {
                    current_indent -= indent_width;

                    if (current_container_size() > 0)
                        stream().put('\n'), output_padding(current_indent);

                    stream().put('}');
                }
            };
        }

        inline std::ostream &operator<<(std::ostream &o, const value &v)
        {
            core::ostream_handle wrap(o);
            dump::stream_writer writer(wrap, 2);
            writer << v;
            return o;
        }

        inline std::ostream &operator<<(std::ostream &o, core::stream_input &&parser)
        {
            core::ostream_handle wrap(o);
            dump::stream_writer writer(wrap, 2);
            writer << parser;
            return o;
        }

        inline const value &operator>>(const value &v, std::ostream &o)
        {
            core::ostream_handle wrap(o);
            dump::stream_writer writer(wrap, 2);
            return v >> writer;
        }

        inline void operator>>(core::stream_input &&parser, std::ostream &o)
        {
            core::ostream_handle wrap(o);
            dump::stream_writer writer(wrap, 2);
            parser >> writer;
        }
    }
}

#endif // CPPDATALIB_DUMP_H
