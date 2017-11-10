#ifndef CPPDATALIB_XML_PROPERTY_LIST_H
#define CPPDATALIB_XML_PROPERTY_LIST_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    // TODO: per stream, change single character writes to put() calls, and string writes to write() calls

    namespace xml_property_list
    {
        namespace impl
        {
            class stream_writer_base : public core::stream_handler, public core::stream_writer
            {
            public:
                stream_writer_base(std::ostream &stream) : core::stream_writer(stream) {}

            protected:
                std::ostream &write_string(std::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        switch (c)
                        {
                            case '"': stream.write("&quot;", 6); break;
                            case '&': stream.write("&amp;", 5); break;
                            case '\'': stream.write("&apos;", 6); break;
                            case '<': stream.write("&lt;", 4); break;
                            case '>': stream.write("&gt;", 4); break;
                            default:
                                if (iscntrl(c))
                                    stream.write("&#", 2).put(c).put(';');
                                else
                                    stream.put(str[i]);
                                break;
                        }
                    }

                    return stream;
                }
            };
        }

        class stream_writer : public impl::stream_writer_base
        {
        public:
            stream_writer(std::ostream &output) : impl::stream_writer_base(output) {}

        protected:
            void begin_() {output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_key_(const core::value &v)
            {
                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void null_(const core::value &) {throw core::error("XML Property List - 'null' value not allowed in output");}
            void bool_(const core::value &v) {output_stream.put('<') << (v.get_bool()? "true": "false") << "/>";}
            void integer_(const core::value &v) {output_stream << "<integer>" << v.get_int() << "</integer>";}
            void uinteger_(const core::value &v) {output_stream << "<integer>" << v.get_uint() << "</integer>";}
            void real_(const core::value &v) {output_stream << "<real>" << v.get_real() << "</real>";}
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "<date>"; break;
                        case core::blob: output_stream << "<data>"; break;
                        default: output_stream << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (v.get_subtype() == core::blob)
                    base64::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (is_key)
                    output_stream << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "</date>"; break;
                        case core::blob: output_stream << "</data>"; break;
                        default: output_stream << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << "<array>";}
            void end_array_(const core::value &, bool) {output_stream << "</array>";}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << "<dict>";}
            void end_object_(const core::value &, bool) {output_stream << "</dict>";}
        };

        class pretty_stream_writer : public impl::stream_writer_base
        {
            size_t indent_width;
            size_t current_indent;

            void output_padding(size_t padding)
            {
                while (padding > 0)
                {
                    char buffer[core::buffer_size];
                    size_t size = std::min(sizeof(buffer)-1, padding);

                    memset(buffer, ' ', size);
                    buffer[size] = 0;

                    output_stream.write(buffer, size);
                    padding -= size;
                }
            }

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width)
                : impl::stream_writer_base(output)
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0; output_stream << std::setprecision(CPPDATALIB_REAL_DIG);}

            void begin_item_(const core::value &)
            {
                if (current_container() != core::null)
                    output_stream << '\n', output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                output_stream << '\n', output_padding(current_indent);

                if (!v.is_string())
                    throw core::error("XML Property List - cannot write non-string key");
            }

            void bool_(const core::value &v) {output_stream << '<' << (v.get_bool()? "true": "false") << "/>";}
            void integer_(const core::value &v)
            {
                output_stream << "<integer>\n", output_padding(current_indent + indent_width);
                output_stream << v.get_int() << '\n'; output_padding(current_indent);
                output_stream << "</integer>";
            }
            void uinteger_(const core::value &v)
            {
                output_stream << "<integer>\n", output_padding(current_indent + indent_width);
                output_stream << v.get_uint() << '\n'; output_padding(current_indent);
                output_stream << "</integer>";
            }
            void real_(const core::value &v)
            {
                output_stream << "<real>\n", output_padding(current_indent + indent_width);
                output_stream << v.get_real() << '\n'; output_padding(current_indent);
                output_stream << "</real>";
            }
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                    output_stream << "<key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "<date>"; break;
                        case core::blob: output_stream << "<data>"; break;
                        default: output_stream << "<string>"; break;
                    }
            }
            void string_data_(const core::value &v, bool)
            {
                if (current_container_size() == 0)
                    output_stream << '\n', output_padding(current_indent + indent_width);

                if (v.get_subtype() == core::blob)
                    base64::write(output_stream, v.get_string());
                else
                    write_string(output_stream, v.get_string());
            }
            void end_string_(const core::value &v, bool is_key)
            {
                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                if (is_key)
                    output_stream << "</key>";
                else
                    switch (v.get_subtype())
                    {
                        case core::date:
                        case core::time:
                        case core::datetime: output_stream << "</date>"; break;
                        case core::blob: output_stream << "</data>"; break;
                        default: output_stream << "</string>"; break;
                    }
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << "<array>";
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                output_stream << "</array>";
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << "<dict>";
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                output_stream << "</dict>";
            }
        };

        inline std::string to_xml_property_list(const core::value &v)
        {
            std::ostringstream stream;
            stream_writer writer(stream);
            writer << v;
            return stream.str();
        }

        inline std::string to_pretty_xml_property_list(const core::value &v, size_t indent_width)
        {
            std::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            writer << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_XML_PROPERTY_LIST_H
