#ifndef CPPDATALIB_XML_XLS_H
#define CPPDATALIB_XML_XLS_H

#include "../core/value_builder.h"

namespace cppdatalib
{
    namespace xml_xls
    {
        inline std::ostream &write_string(std::ostream &stream, const std::string &str)
        {
            for (size_t i = 0; i < str.size(); ++i)
            {
                int c = str[i] & 0xff;

                switch (c)
                {
                    case '"': stream << "&quot;"; break;
                    case '&': stream << "&amp;"; break;
                    case '\'': stream << "&apos;"; break;
                    case '<': stream << "&lt;"; break;
                    case '>': stream << "&gt;"; break;
                    default:
                        if (iscntrl(c))
                            stream << "&#" << c << ';';
                        else
                            stream << str[i];
                        break;
                }
            }

            return stream;
        }

        class table_writer : public core::stream_handler, public core::stream_writer
        {
        public:
            table_writer(std::ostream &output) : core::stream_writer(output) {}

        protected:
            void begin_() {output_stream << "<Table>";}
            void end_() {output_stream << "</Table>";}

            void begin_item_(const core::value &v)
            {
                const char *type = NULL;

                switch (v.get_type())
                {
                    case core::null: type = "String"; break;
                    case core::boolean: type = "Boolean"; break;
                    case core::integer:
                    case core::real: type = "Number"; break;
                    case core::string:
                        if (v.get_subtype() == core::date ||
                            v.get_subtype() == core::time ||
                            v.get_subtype() == core::datetime)
                            type = "DateTime";
                        else
                            type = "String";
                        break;
                    case core::array:
                        if (nesting_depth() == 0)
                            break; // No need to do anything for the top-level array
                        output_stream << "<Row>";
                        break;
                    default: break;
                }

                if (type)
                    output_stream << "<Cell><Data ss:Type=\"" << type << "\">";
            }
            void end_item_(const core::value &v)
            {
                if (v.is_array())
                {
                    if (nesting_depth() > 1)
                        output_stream << "</Row>";
                }
                else
                    output_stream << "</Data></Cell>";
            }

            void bool_(const core::value &v) {output_stream << v.as_int();}
            void integer_(const core::value &v) {output_stream << v.get_int();}
            void real_(const core::value &v) {output_stream << v.get_real();}
            void string_data_(const core::value &v) {write_string(output_stream, v.get_string());}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() == 2)
                    throw core::error("XML XLS - 'array' value not allowed in row output");
            }
            void begin_object_(const core::value &, core::int_t, bool) {throw core::error("XML XLS - 'object' value not allowed in output");}
        };

        class worksheet_writer : public table_writer
        {
            std::string worksheet_name;

        public:
            worksheet_writer(std::ostream &output, const std::string &worksheet_name) : table_writer(output), worksheet_name(worksheet_name) {}

        protected:
            void begin_()
            {
                if (worksheet_name.find_first_of("\\/?*[]") != std::string::npos)
                    throw core::error("XML XLS - Invalid worksheet name cannot contain any of '\\/?*[]'");

                output_stream << "<Worksheet ss:Name=\"";
                write_string(output_stream, worksheet_name);
                output_stream << "\">";

                table_writer::begin_();
            }
            void end_()
            {
                table_writer::end_();

                output_stream << "</Worksheet>";
            }
        };

        class workbook_writer : public worksheet_writer
        {
        public:
            workbook_writer(std::ostream &output, const std::string &worksheet_name) : worksheet_writer(output, worksheet_name) {}

        protected:
            void begin_()
            {
                output_stream << "<Workbook xmlns=\"urn:schemas-microsoft-com:office:spreadsheet\""
                              << " xmlns:c=\"urn:schemas-microsoft-com:office:component:spreadsheet\""
                              << " xmlns:html=\"http://www.w3.org/TR/REC-html40\""
                              << " xmlns:o=\"urn:schemas-microsoft-com:office:office\""
                              << " xmlns:ss=\"urn:schemas-microsoft-com:office:spreadsheet\""
                              << " xmlns:x2=\"http://schemas.microsoft.com/office/excel/2003/xml\""
                              << " xmlns:x=\"urn:schemas-microsoft-com:office:excel\""
                              << " xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">";

                worksheet_writer::begin_();
            }
            void end_()
            {
                worksheet_writer::end_();

                output_stream << "</Workbook>";
            }
        };

        class document_writer : public workbook_writer
        {
        public:
            document_writer(std::ostream &output, const std::string &worksheet_name) : workbook_writer(output, worksheet_name) {}

        protected:
            void begin_()
            {
                output_stream << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                              << "<?mso-application progid=\"Excel.Sheet\"?>";

                workbook_writer::begin_();
            }
        };

        inline std::ostream &operator<<(std::ostream &stream, const core::value &v)
        {
            table_writer writer(stream);
            core::convert(v, writer);
            return stream;
        }

        inline std::ostream &print(std::ostream &stream, const core::value &v) {return stream << v;}

        inline std::string to_xml_xls(const core::value &v)
        {
            std::ostringstream stream;
            stream << v;
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_XML_XLS_H
