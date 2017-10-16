#ifndef CPPDATALIB_JSON_H
#define CPPDATALIB_JSON_H

#include "../core/value_builder.h"
#include "../core/hex.h"

namespace cppdatalib
{
    namespace json
    {
        inline std::istream &read_string(std::istream &stream, core::stream_handler &writer)
        {
            static const std::string hex = "0123456789ABCDEF";
            int c;
            std::string buffer;

            {char chr; stream >> chr; c = chr;} // Skip initial whitespace
            if (c != '"') throw core::error("JSON - expected string");

            writer.begin_string(core::string_t(), core::stream_handler::unknown_size);
            while (c = stream.get(), c != '"')
            {
                if (c == EOF) throw core::error("JSON - unexpected end of string");

                if (c == '\\')
                {
                    c = stream.get();
                    if (c == EOF) throw core::error("JSON - unexpected end of string");

                    switch (c)
                    {
                        case 'b': buffer.push_back('\b'); break;
                        case 'f': buffer.push_back('\f'); break;
                        case 'n': buffer.push_back('\n'); break;
                        case 'r': buffer.push_back('\r'); break;
                        case 't': buffer.push_back('\t'); break;
                        case 'u':
                        {
                            uint32_t code = 0;
                            for (int i = 0; i < 4; ++i)
                            {
                                c = stream.get();
                                if (c == EOF) throw core::error("JSON - unexpected end of string");
                                size_t pos = hex.find(toupper(c));
                                if (pos == std::string::npos) throw core::error("JSON - invalid character escape sequence");
                                code = (code << 4) | pos;
                            }

                            std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf8;
                            buffer += utf8.to_bytes(code);

                            break;
                        }
                        default:
                            buffer.push_back(c);
                            break;
                    }
                }
                else
                    buffer.push_back(c);

                if (buffer.size() >= 65536)
                    writer.append_to_string(buffer), buffer.clear();
            }

            writer.append_to_string(buffer);
            writer.end_string(core::string_t());
            return stream;
        }

        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                if (c == '"' || c == '\\')
                    stream << '\\' << static_cast<char>(c);
                else
                {
                    switch (c)
                    {
                        case '"':
                        case '\\': stream << '\\' << static_cast<char>(c); break;
                        case '\b': stream << "\\b"; break;
                        case '\f': stream << "\\f"; break;
                        case '\n': stream << "\\n"; break;
                        case '\r': stream << "\\r"; break;
                        case '\t': stream << "\\t"; break;
                        default:
                            if (iscntrl(c))
                                hex::write(stream << "\\u00", c);
                            else
                                stream << str[i];
                            break;
                    }
                }
            }

            return stream;
        }

        inline std::istream &convert(std::istream &stream, core::stream_handler &writer)
        {
            bool delimiter_required = false;
            char chr;

            writer.begin();

            while (stream >> std::skipws >> chr, stream.unget(), stream.good() && !stream.eof())
            {
                if (writer.nesting_depth() == 0 && delimiter_required)
                    break;

                if (delimiter_required && !strchr(",:]}", chr))
                    throw core::error("JSON - expected ',' separating array or object entries");

                switch (chr)
                {
                    case 'n':
                        if (!core::stream_starts_with(stream, "null")) throw core::error("JSON - expected 'null' value");
                        writer.write(core::null_t());
                        delimiter_required = true;
                        break;
                    case 't':
                        if (!core::stream_starts_with(stream, "true")) throw core::error("JSON - expected 'true' value");
                        writer.write(true);
                        delimiter_required = true;
                        break;
                    case 'f':
                        if (!core::stream_starts_with(stream, "false")) throw core::error("JSON - expected 'false' value");
                        writer.write(false);
                        delimiter_required = true;
                        break;
                    case '"':
                        read_string(stream, writer);
                        delimiter_required = true;
                        break;
                    case ',':
                        stream.get(); // Eat ','
                        if (writer.current_container_size() == 0 || writer.container_key_was_just_parsed())
                            throw core::error("JSON - invalid ',' does not separate array or object entries");
                        stream >> chr; stream.unget(); // Peek ahead
                        if (!stream || chr == ',' || chr == ']' || chr == '}')
                            throw core::error("JSON - invalid ',' does not separate array or object entries");
                        delimiter_required = false;
                        break;
                    case ':':
                        stream.get(); // Eat ':'
                        if (!writer.container_key_was_just_parsed())
                            throw core::error("JSON - invalid ':' does not separate a key and value pair");
                        delimiter_required = false;
                        break;
                    case '[':
                        stream.get(); // Eat '['
                        writer.begin_array(core::array_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case ']':
                        stream.get(); // Eat ']'
                        writer.end_array(core::array_t());
                        delimiter_required = true;
                        break;
                    case '{':
                        stream.get(); // Eat '{'
                        writer.begin_object(core::object_t(), core::stream_handler::unknown_size);
                        delimiter_required = false;
                        break;
                    case '}':
                        stream.get(); // Eat '}'
                        writer.end_object(core::object_t());
                        delimiter_required = true;
                        break;
                    default:
                        if (isdigit(chr) || chr == '-')
                        {
                            core::real_t r;
                            stream >> r;
                            if (!stream) throw core::error("JSON - invalid number");

                            if (r == trunc(r) && r >= INT64_MIN && r <= INT64_MAX)
                                writer.write(static_cast<core::int_t>(r));
                            else
                                writer.write(r);

                            delimiter_required = true;
                        }
                        else
                            throw core::error("JSON - expected value");
                        break;
                }
            }

            if (!delimiter_required)
                throw core::error("JSON - expected value");

            writer.end();
            return stream;
        }

        class stream_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            stream_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << ':';
                else if (current_container_size() > 0)
                    output_stream << ',';
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                output_stream << v.get_real();
            }
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool) {output_stream << '[';}
            void end_array_(const core::value &, bool) {output_stream << ']';}

            void begin_object_(const core::value &, core::int_t, bool) {output_stream << '{';}
            void end_object_(const core::value &, bool) {output_stream << '}';}
        };

        class pretty_stream_writer : public core::stream_handler, public core::stream_writer
        {
            size_t indent_width;
            size_t current_indent;

            void output_padding(size_t padding)
            {
                while (padding-- > 0)
                    output_stream << ' ';
            }

        public:
            pretty_stream_writer(std::ostream &output, size_t indent_width)
                : core::stream_writer(output)
                , indent_width(indent_width)
                , current_indent(0)
            {}

            size_t indent() {return indent_width;}

        protected:
            void begin_() {current_indent = 0;}

            void begin_item_(const core::value &)
            {
                if (container_key_was_just_parsed())
                    output_stream << ": ";
                else if (current_container_size() > 0)
                    output_stream << ',';

                if (current_container() == core::array)
                    output_stream << '\n', output_padding(current_indent);
            }
            void begin_key_(const core::value &v)
            {
                if (current_container_size() > 0)
                    output_stream << ',';

                output_stream << '\n', output_padding(current_indent);

                if (!v.is_string())
                    throw core::error("JSON - cannot write non-string key");
            }

            void null_(const core::value &) {output_stream << "null";}
            void bool_(const core::value &v) {output_stream << (v.get_bool()? "true": "false");}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real()))
                    throw core::error("JSON - cannot write 'NaN' or 'Infinity' values");
                output_stream << v.get_real();
            }
            void begin_string_(const core::value &, core::int_t, bool) {output_stream << '"';}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}
            void end_string_(const core::value &, bool) {output_stream << '"';}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                output_stream << '[';
                current_indent += indent_width;
            }
            void end_array_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                output_stream << ']';
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                output_stream << '{';
                current_indent += indent_width;
            }
            void end_object_(const core::value &, bool)
            {
                current_indent -= indent_width;

                if (current_container_size() > 0)
                    output_stream << '\n', output_padding(current_indent);

                output_stream << '}';
            }
        };

        inline std::istream &operator>>(std::istream &stream, core::value &v)
        {
            core::value_builder builder(v);
            convert(stream, builder);
            return stream;
        }

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            stream_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::istream &input(std::istream &stream, core::value &v) {return stream >> v;}
        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::ostream &pretty_print(std::ostream &stream, const core::value &v, size_t indent_width)
        {
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream;
        }

        inline core::value from_json(const std::string &json)
        {
            std::istringstream stream(json);
            core::value v;
            stream >> v;
            return v;
        }

        inline std::string to_json(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }

        inline std::string to_pretty_json(const core::value &v, size_t indent_width)
        {
            std::ostringstream stream;
            pretty_stream_writer writer(stream, indent_width);
            core::convert(v, writer);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_JSON_H
