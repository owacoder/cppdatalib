/*
 * mariadb.h
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

#ifndef CPPDATALIB_MARIADB_H
#define CPPDATALIB_MARIADB_H

#ifndef CPPDATALIB_DISABLE_MARIADB

#include "../core/core.h"
#include <mysql/mysql.h>

namespace cppdatalib
{
    namespace mariadb
    {
        namespace impl
        {
            std::string escape(MYSQL *mysql, const std::string &str)
            {
                std::string temp;
                temp.resize(str.length() * 2 + 1);
                unsigned long length = mysql_real_escape_string(mysql, &temp[0], str.c_str(), str.length());
                temp.resize(length);
                return temp;
            }

            // TODO: doesn't support ENUM values as distinct values
            // TODO: doesn't support SET values as distinct values
            void convert_string(core::value &value, const core::string_t &native_column_type)
            {
                if (native_column_type.find("char") != std::string::npos ||
                        native_column_type.find("varchar") != std::string::npos ||
                        native_column_type.find("tinytext") != std::string::npos ||
                        native_column_type.find("text") != std::string::npos ||
                        native_column_type.find("mediumtext") != std::string::npos ||
                        native_column_type.find("longtext") != std::string::npos)
                    value.convert_to_string();
                else if (native_column_type.find("blob") != std::string::npos ||
                         native_column_type.find("mediumblob") != std::string::npos ||
                         native_column_type.find("longblob") != std::string::npos)
                {
                    value.convert_to_string();
                    value.set_subtype(core::blob);
                }
                else if (native_column_type.find("tinyint") != std::string::npos ||
                         native_column_type.find("smallint") != std::string::npos ||
                         native_column_type.find("mediumint") != std::string::npos ||
                         native_column_type.find("int") != std::string::npos ||
                         native_column_type.find("bigint") != std::string::npos ||
                         native_column_type.find("year") != std::string::npos)
                {
                    if (native_column_type.find("unsigned") != std::string::npos)
                        value.convert_to_uint();
                    else
                        value.convert_to_int();
                }
                else if (native_column_type.find("float") != std::string::npos ||
                         native_column_type.find("double") != std::string::npos)
                    value.convert_to_real();
                else if (native_column_type.find("decimal") != std::string::npos)
                    value.set_subtype(core::bignum);
                else if (native_column_type.find("date") != std::string::npos)
                    value.set_subtype(core::date);
                else if (native_column_type.find("time") != std::string::npos)
                    value.set_subtype(core::time);
                else if (native_column_type.find("datetime") != std::string::npos ||
                         native_column_type.find("timestamp") != std::string::npos)
                    value.set_subtype(core::datetime);
            }

            class stream_writer_base : public core::stream_handler
            {
            protected:
                std::ostream &write_string(std::ostream &stream, const std::string &str)
                {
                    for (size_t i = 0; i < str.size(); ++i)
                    {
                        int c = str[i] & 0xff;

                        if (c == '"' || c == '\\')
                        {
                            stream.put('\\');
                            stream.put(c);
                        }
                        else
                        {
                            switch (c)
                            {
                                case '"':
                                case '\\':
                                    stream.put('\\');
                                    stream.put(c);
                                    break;
                                case '\b': stream.write("\\b", 2); break;
                                case '\f': stream.write("\\f", 2); break;
                                case '\n': stream.write("\\n", 2); break;
                                case '\r': stream.write("\\r", 2); break;
                                case '\t': stream.write("\\t", 2); break;
                                default:
                                    if (iscntrl(c))
                                        hex::write(stream.write("\\u00", 4), c);
                                    else
                                        stream.put(str[i]);
                                    break;
                            }
                        }
                    }

                    return stream;
                }
            };
        }

        // The MySQL connection is assumed to be active, with a selected database, when connect() is called.
        // Calling without these preconditions fulfilled will result in an error being thrown.
        class table_parser : public core::stream_input
        {
            MYSQL *mysql;

            core::value metadata;
            /* metadata contains an array of column specifiers with the following element structure:
             *
             * {
             *     "required": boolean[true/false],
             *     "default": default value,
             *     "datatype": native datatype,
             *     "name": column name
             * }
             */
            std::string db;
            std::string table;
            bool escaped;

        public:
            table_parser(MYSQL *connection,
                   const std::string &db,
                   const std::string &table)
                : mysql(connection)
                , db(db)
                , table(table)
                , escaped(false)
            {}

            void connect(const char *host,
                         const char *user,
                         const char *passwd,
                         const char *db,
                         unsigned int port = 0,
                         unsigned long client_flag = 0)
            {
                if (!mysql_real_connect(mysql, host, user, passwd, db, port, NULL, client_flag))
                    throw core::error("MySQL - could not connect to specified database");
            }

            bool provides_prefix_string_size() const {return true;}
            bool provides_prefix_object_size() const {return true;}
            bool provides_prefix_array_size() const {return true;}

            const core::value &refresh_metadata()
            {
                std::string query;
                MYSQL_ROW row;
                MYSQL_RES *result = NULL;

                try
                {
                    if (!escaped)
                    {
                        db = impl::escape(mysql, db);
                        table = impl::escape(mysql, table);
                        escaped = true;
                    }

                    //
                    // `EXPLAIN` to get the types of the data
                    //
                    query = "EXPLAIN " + db + "." + table;
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << query << std::endl;
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - EXPLAIN table query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    unsigned int columns = mysql_num_fields(result);
                    if (columns < 5)
                        throw core::error("MySQL - invalid response while attempting to get column types");

                    metadata.set_null();
                    while ((row = mysql_fetch_row(result)) != NULL)
                    {
                        core::value row_value;
                        row_value["name"] = row[0]? row[0]: "";
                        row_value["datatype"] = row[1]? row[1]: "";
                        row_value["required"] = row[2]? (tolower(*row[2]) == 'y'? false: true): true;
                        row_value["default"] = row[4]? core::value(row[4]): core::value(core::null_t());

                        if (!row_value["default"].is_null())
                            impl::convert_string(row_value["default"], row_value["datatype"].as_string());

                        metadata.push_back(row_value);
                    }

                    mysql_free_result(result);
                    result = NULL;
                }
                catch (core::error)
                {
                    if (result)
                        mysql_free_result(result);
                    throw;
                }

                return metadata;
            }

            // Returns the table metadata from the previous convert() or refresh_metadata() operation
            const core::value &get_metadata() const {return metadata;}

            core::stream_input &convert(core::stream_handler &writer)
            {
                MYSQL_RES *result = NULL;

                writer.begin_array(core::array_t(), core::stream_handler::unknown_size);

                try
                {
                    std::string query;
                    MYSQL_ROW row;

                    refresh_metadata();

                    //
                    // `SELECT` all data out of the specified table
                    //
                    query = "SELECT * FROM " + table;
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - SELECT * FROM table query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    unsigned int columns = mysql_num_fields(result);

                    while ((row = mysql_fetch_row(result)) != NULL)
                    {
                        writer.begin_array(core::array_t(), columns);

                        for (unsigned int i = 0; i < columns; ++i)
                        {
                            if (row[i] == NULL)
                                writer.write(core::null_t());
                            else
                            {
                                // TODO: doesn't support binary row values with embedded NUL characters
                                core::value value(row[i]);
                                const core::string_t &column_type = metadata[i]["datatype"].get_string();

                                impl::convert_string(value, column_type);

                                writer.write(value);
                            }
                        }

                        writer.end_array(core::array_t());
                    }

                    mysql_free_result(result);
                    result = NULL;
                }
                catch (core::error)
                {
                    if (result)
                        mysql_free_result(result);
                    throw;
                }

                writer.end_array(core::array_t());

                return *this;
            }
        };

        // The MySQL connection is assumed to be active, with a selected database, when connect() is called.
        // Calling without these preconditions fulfilled will result in an error being thrown.
        class parser : public core::stream_input
        {
            MYSQL *mysql;

            core::value metadata; // object: keys are table names, values are table metadata (see table_parser for details)
            std::string db;
            bool escaped;

        public:
            parser(MYSQL *connection,
                   const std::string &db)
                : mysql(connection)
                , db(db)
                , escaped(false)
            {}

            void connect(const char *host,
                         const char *user,
                         const char *passwd,
                         const char *db,
                         unsigned int port = 0,
                         unsigned long client_flag = 0)
            {
                if (!mysql_real_connect(mysql, host, user, passwd, db, port, NULL, client_flag))
                    throw core::error("MySQL - could not connect to specified database");
            }

            bool provides_prefix_string_size() const {return true;}
            bool provides_prefix_object_size() const {return true;}
            bool provides_prefix_array_size() const {return true;}

            // Refreshes the metadata to the current state of the table
            const core::value &refresh_metadata()
            {
                std::string query;
                MYSQL_ROW row;
                MYSQL_RES *result = NULL;

                try
                {
                    if (!escaped)
                    {
                        db = impl::escape(mysql, db);
                        escaped = true;
                    }

                    //
                    // `SHOW TABLES` to get the types of the data
                    //
                    query = "SHOW TABLES";
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - SHOW TABLES query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    unsigned int columns = mysql_num_fields(result);
                    if (columns < 1)
                        throw core::error("MySQL - invalid response while attempting to get table names");

                    metadata.set_null();
                    while ((row = mysql_fetch_row(result)) != NULL)
                        metadata.add_member(row[0]? row[0]: "");

                    mysql_free_result(result);
                    result = NULL;
                }
                catch (core::error)
                {
                    if (result)
                        mysql_free_result(result);
                    throw;
                }

                return metadata;
            }

            // Returns the metadata from the previous convert() or refresh_metadata() operation
            const core::value &get_metadata() const {return metadata;}

            // Returns the list of tables from the previous convert() operation
            std::vector<std::string> get_tables() const
            {
                std::vector<std::string> result;
                for (auto const &i: metadata.get_object())
                    result.push_back(i.first.as_string());
                return result;
            }

            core::stream_input &convert(core::stream_handler &writer)
            {
                MYSQL_RES *result = NULL;

                writer.begin_object(core::object_t(), core::stream_handler::unknown_size);

                try
                {
                    std::string query;
                    MYSQL_ROW row;

                    if (!escaped)
                    {
                        db = impl::escape(mysql, db);
                        escaped = true;
                    }

                    //
                    // `SHOW TABLES` to get the types of the data
                    //
                    query = "SHOW TABLES";
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - SHOW TABLES query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    unsigned int columns = mysql_num_fields(result);
                    if (columns < 1)
                        throw core::error("MySQL - invalid response while attempting to get table names");

                    metadata.set_null();
                    while ((row = mysql_fetch_row(result)) != NULL)
                        metadata.add_member(row[0]? row[0]: "");

                    mysql_free_result(result);
                    result = NULL;

                    for (auto &table: metadata.get_object())
                    {
                        table_parser tparser(mysql, db, table.first.as_string());

                        // Read table to writer
                        writer.write(table.first.as_string());
                        tparser >> writer;

                        // Store metadata of table
                        table.second = tparser.get_metadata();
                    }
                }
                catch (core::error)
                {
                    if (result)
                        mysql_free_result(result);
                    throw;
                }

                writer.end_object(core::object_t());

                return *this;
            }
        };

        class table_writer : public impl::stream_writer_base
        {
            MYSQL *mysql;

            core::value metadata;
            /* columns must contain an array of column specifiers, using the following element structure:
             *
             * {
             *     "required": boolean[true/false],
             *     "default": default value,
             *     "datatype": native datatype,
             *     "name": column name
             * }
             */
            std::string db;
            std::string table;
            bool escaped;

            core::string_t buffer_string; // Buffer for string values

            std::string column_names; // Prepared column names in string ready for INSERT query (prepared by create_specification())
            std::string insert_query; // The query to run to insert a row

        public:
            table_writer(MYSQL *connection,
                         const std::string &db,
                         const std::string &table)
                : mysql(connection)
                , db(db)
                , table(table)
                , escaped(false)
            {}

            void connect(const char *host,
                         const char *user,
                         const char *passwd,
                         const char *db,
                         unsigned int port = 0,
                         unsigned long client_flag = 0)
            {
                if (!mysql_real_connect(mysql, host, user, passwd, db, port, NULL, client_flag))
                    throw core::error("MySQL - could not connect to specified database");
            }

            void set_metadata(const core::value &metadata) {this->metadata = metadata;}

        protected:
            std::string create_specification()
            {
                std::string result;

                if (!escaped)
                {
                    db = impl::escape(mysql, db);
                    table = impl::escape(mysql, table);
                    escaped = true;
                }

                result += "CREATE TABLE " + table + " (";
                column_names = "(";
                for (auto column = metadata.get_array().cbegin(); column != metadata.get_array().cend(); ++column)
                {
                    if (column != metadata.get_array().cbegin())
                    {
                        result += ", ";
                        column_names += ", ";
                    }

                    result += impl::escape(mysql, column->member("name").as_string()) + " " + impl::escape(mysql, column->member("datatype").as_string());
                    if (column->member("required").as_bool())
                        result += " NOT";
                    result += " NULL";
                    if (column->is_member("default") && !column->member("default").is_null())
                        result += " " + impl::escape(mysql, column->member("default").as_string());

                    column_names += impl::escape(mysql, column->member("name").as_string());
                }
                result += ")";
                column_names += ")";

                std::cout << result << std::endl;

                return result;
            }

            void begin_()
            {
                std::string query;

                if (!escaped)
                {
                    db = impl::escape(mysql, db);
                    table = impl::escape(mysql, table);
                    escaped = true;
                }

                //
                // `DROP TABLE IF EXISTS` to delete the table
                //
                query = "DROP TABLE IF EXISTS " + table;
                if (mysql_real_query(mysql, query.c_str(), query.length()))
                {
                    std::cerr << mysql_error(mysql) << std::endl;
                    throw core::error("MySQL - DROP TABLE query failed");
                }

                //
                // `CREATE TABLE` to (re)create the table
                //
                query = create_specification();
                if (mysql_real_query(mysql, query.c_str(), query.length()))
                {
                    std::cerr << mysql_error(mysql) << std::endl;
                    throw core::error("MySQL - CREATE TABLE query failed");
                }
            }

            void begin_item_(const core::value &)
            {
                if (current_container_size() > 0)
                    insert_query += ", ";
            }

            void null_(const core::value &) {insert_query += "NULL";}
            void bool_(const core::value &v) {insert_query += (v.get_bool()? "YES": "NO");}
            void integer_(const core::value &v)
            {
                std::stringstream stream;

                stream << v.get_int();
                insert_query += impl::escape(mysql, stream.str());
            }
            void uinteger_(const core::value &v)
            {
                std::stringstream stream;

                stream << v.get_uint();
                insert_query += impl::escape(mysql, stream.str());
            }
            void real_(const core::value &v)
            {
                std::stringstream stream;

                stream << std::setprecision(CPPDATALIB_REAL_DIG);
                stream << v.get_uint();
                insert_query += impl::escape(mysql, stream.str());
            }
            void begin_string_(const core::value &, core::int_t, bool) {buffer_string.clear();}
            void string_data_(const core::value &v, bool) {buffer_string += v.get_string();}
            void end_string_(const core::value &, bool) {insert_query += '"' + impl::escape(mysql, buffer_string) + '"';}

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                    insert_query = "INSERT INTO " + table + " " + column_names + " VALUES (";
            }
            void end_array_(const core::value &, bool)
            {
                if (!insert_query.empty() && insert_query.front() == 'I')
                {
                    insert_query += ")";

                    std::cout << insert_query << std::endl;
                    if (mysql_real_query(mysql, insert_query.c_str(), insert_query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - INSERT INTO table query failed");
                    }
                }
                insert_query.clear();
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                throw core::error("MariaDB - 'object' values not allowed in output");
            }
        };
    }
}

#endif // CPPDATALIB_BUILD_MARIADB

#endif // CPPDATALIB_MARIADB_H
