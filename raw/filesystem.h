/*
 * filesystem.h
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#if __cplusplus >= 201703L

#include "../core/core.h"
#include <filesystem>
#include <iostream>
#include <fstream>

#elif __cplusplus >= 201402L

#include "../core/core.h"
#include <experimental/filesystem>
#include <iostream>
#include <fstream>

#endif

#include <chrono>

#if __cplusplus >= 201402L
namespace cppdatalib
{
    namespace filesystem
    {
#if __cplusplus >= 201703L
        namespace fs = std::filesystem;
#else
        namespace fs = std::experimental::filesystem;
#endif

        class parser : public core::stream_input
        {
            fs::path root_path;
            bool recursive_parse;
            fs::directory_iterator it;
            fs::recursive_directory_iterator r_it;

            fs::path filepath;
            std::ifstream stream;

            fs::directory_options options;
            unsigned long file_options;

            std::unique_ptr<char []> buffer;

        public:
            enum read_options
            {
                read_block_devices = 0x01, // Read special block files to output (if not enabled, just output as empty file)
                read_char_devices = 0x02, // Read character files to output (if not enabled, just output as empty file)
                read_fifo_devices = 0x04, // Read IPC pipe files to output (if not enabled, just output as empty file)
                read_socket_devices = 0x08, // Read IPC socket files to output (if not enabled, just output as empty file)
                skip_unread_files = 0x10, // If enabled, don't output even the filename of unread files (as mentioned above)
                skip_file_reading = 0x20 // If enabled, don't output any file contents
            };

            parser(const fs::path &root, bool recursive_parse = true, unsigned long file_options = 0, fs::directory_options options = fs::directory_options())
                : root_path(root)
                , recursive_parse(recursive_parse)
                , it(!recursive_parse && fs::is_directory(root)? fs::directory_iterator(root, options): fs::directory_iterator())
                , r_it(recursive_parse && fs::is_directory(root)? fs::recursive_directory_iterator(root, options): fs::recursive_directory_iterator())
                , options(options)
                , file_options(file_options)
                , buffer(new char [core::buffer_size])
            {
                reset();
            }

            unsigned int features() const {return provides_prefix_string_size;}

            bool busy() const {return core::stream_input::busy() || (!was_just_reset() && (it != fs::end(it) || r_it != fs::end(r_it) || stream.is_open()));}

        protected:
            void reset_()
            {
                bool is_dir = fs::is_directory(root_path);
                it = !recursive_parse && is_dir? fs::directory_iterator(root_path, options): fs::directory_iterator();
                r_it = recursive_parse && is_dir? fs::recursive_directory_iterator(root_path, options): fs::recursive_directory_iterator();
                filepath.clear();
            }

            intmax_t file_time_to_unix_time(fs::file_time_type time)
            {
                return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
            }

            void begin_read_file_contents(const fs::directory_entry &entry)
            {
                core::value v;
                uintmax_t filesize = fs::file_size(entry.path());

                v.set_string(entry.path().filename());
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                v.add_attribute("permissions", core::value(unsigned(entry.status().permissions() & fs::perms::mask)));
                v.add_attribute("size", core::value(filesize));
                v.add_attribute("modified", core::value(file_time_to_unix_time(fs::last_write_time(entry.path())), core::unix_timestamp));
#endif
                get_output()->write(v);

                if (file_options & skip_file_reading)
                    get_output()->write(core::value());
                else
                {
                    get_output()->begin_string(core::value(core::string_t(), core::blob), filesize);
                    stream.open(filepath = entry.path());
                    if (!stream)
                        throw core::custom_error("filesystem - could not open \"" + entry.path().native() + "\" for input");
                }
            }

            void write_one_()
            {
                try
                {
                    if (was_just_reset())
                    {
                        if (fs::is_directory(root_path))
                            get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                        else
                        {
                            if ((file_options & skip_file_reading) == 0)
                            {
                                get_output()->begin_string(core::value(core::string_t(), core::blob), fs::file_size(root_path));
                                stream.open(root_path);
                            }
                            else
                                get_output()->write(core::value(core::string_t(), core::blob));
                        }
                        return;
                    }
                    else if (stream.is_open()) // Read file contents
                    {
                        stream.read(buffer.get(), core::buffer_size);
                        if (stream)
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), core::buffer_size)));
                        else if (stream.eof())
                        {
                            get_output()->append_to_string(core::value(core::string_t(buffer.get(), stream.gcount())));
                            get_output()->end_string(core::value(core::string_t(), core::blob));
                            stream.close();
                        }
                        else
                            throw core::custom_error("filesystem - an error occured while reading \"" + filepath.native() + "\"");
                        return;
                    }

                    core::value v;
                    fs::directory_entry entry;

                    if (recursive_parse)
                    {
                        if (r_it == fs::end(r_it))
                        {
                            while (nesting_depth())
                                get_output()->end_object(core::value(core::object_t()));
                            return;
                        }

                        // If the iterator went up, we need to close directory objects until we get back to the same level
                        while (r_it.depth() < 0 || unsigned(r_it.depth()) + 1 < nesting_depth())
                            get_output()->end_object(core::value(core::object_t()));

                        entry = *r_it++;
                    }
                    else
                    {
                        if (it == fs::end(it))
                        {
                            while (nesting_depth())
                                get_output()->end_object(core::value(core::object_t()));
                            return;
                        }

                        // If the iterator went up, we need to close directory objects until we get back to the same level
                        while (nesting_depth() > 1)
                            get_output()->end_object(core::value(core::object_t()));

                        entry = *it++;
                    }

                    switch (entry.status().type())
                    {
                        case fs::file_type::block:
                            if (file_options & read_block_devices)
                                begin_read_file_contents(entry);
                            else if ((file_options & skip_unread_files) == 0)
                            {
                                v.set_string(entry.path().filename());
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                                v.add_attribute("permissions", core::value(unsigned(entry.status().permissions() & fs::perms::mask)));
                                v.add_attribute("size", core::value(fs::file_size(entry.path())));
                                v.add_attribute("modified", core::value(file_time_to_unix_time(fs::last_write_time(entry.path())), core::unix_timestamp));
#endif
                                get_output()->write(v);
                                get_output()->write(core::value());
                            }
                            break;
                        case fs::file_type::character:
                            if (file_options & read_char_devices)
                                begin_read_file_contents(entry);
                            else if ((file_options & skip_unread_files) == 0)
                            {
                                v.set_string(entry.path().filename());
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                                v.add_attribute("permissions", core::value(unsigned(entry.status().permissions() & fs::perms::mask)));
                                v.add_attribute("size", core::value(fs::file_size(entry.path())));
                                v.add_attribute("modified", core::value(file_time_to_unix_time(fs::last_write_time(entry.path())), core::unix_timestamp));
#endif
                                get_output()->write(v);
                                get_output()->write(core::value());
                            }
                            break;
                        case fs::file_type::directory:
                            v.set_string(entry.path().filename());
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                            v.add_attribute("permissions", core::value(unsigned(entry.status().permissions() & fs::perms::mask)));
                            v.add_attribute("size", core::value(0));
                            v.add_attribute("modified", core::value(file_time_to_unix_time(fs::last_write_time(entry.path())), core::unix_timestamp));
#endif

                            get_output()->write(v);
                            get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                            break;
                        case fs::file_type::regular:
                        case fs::file_type::unknown: // Exists, but unknown type. Attempt to read it
                            begin_read_file_contents(entry);
                            break;
                        case fs::file_type::not_found:
                            // Do nothing
                            break;
                        case fs::file_type::none:
                        default:
                            throw core::custom_error("filesystem - an error occured while reading file type information for \"" + entry.path().native() + "\"");
                    }
                } catch (const fs::filesystem_error &e) {
                    throw core::custom_error("filesystem - " + std::string(e.what()));
                }
            }
        };

        class stream_writer : public core::stream_handler
        {
            fs::path path_root;
            std::vector<core::value> directories;

            std::ofstream stream;
            core::value key;
            bool scalar_is_key;

            bool safe_write; // true: don't allow overwriting files, false: allow overwriting files
            bool safe_dirs; // true: don't allow pre-existing directories, false: allow pre-existing directories

        public:
            // By default, pre-existing files and directories are not allowed
            stream_writer(const fs::path &path_root, bool safe_write = true, bool safe_dirs = true)
                : path_root(path_root)
                , key("")
                , safe_write(safe_write)
                , safe_dirs(safe_dirs)
            {}

            std::string name() const {return "cppdatalib::filesystem::stream_writer";}

            void set_safe_write(bool safe) {safe_write = safe;}
            bool is_safe_write() const {return safe_write;}

            void set_safe_dirs(bool safe) {safe_dirs = safe;}
            bool is_safe_dirs() const {return safe_dirs;}

        protected:
            fs::path path() const {
                fs::path result(path_root);
                for (const std::string &dir: directories)
                    result /= dir;
                return result;
            }

            void begin_() {stream.precision(CPPDATALIB_REAL_DIG);}

            // Keys are file or directory names
            void begin_key_(const core::value &v)
            {
                if (!v.is_string() && !v.is_int() && !v.is_uint() && !v.is_real())
                    throw core::error("filesystem - cannot write non-string, non-numeric key");
                scalar_is_key = true;
            }
            void end_key_(const core::value &)
            {
                if (key.get_string_unchecked().empty() ||
                    key.get_string_unchecked() == "." ||
                    key.get_string_unchecked() == ".." ||
                    key.get_string_unchecked().find('/') != key.get_string_unchecked().npos ||
                    key.get_string_unchecked().find(fs::path::preferred_separator) != key.get_string_unchecked().npos)
                    throw core::custom_error("filesystem - invalid filename, cannot open \"" + (path() / key.get_string_unchecked()).native() + "\" for writing");
                scalar_is_key = false;
            }

            // Items are either files or lists of directory items
            void begin_item_(const core::value &v)
            {
                if (current_container() == core::array)
                    key = std::to_string(current_container_size());

                if (!v.is_array() && !v.is_object())
                {
                    fs::path p = path() / key.get_string_unchecked();

                    if (safe_write && fs::exists(p))
                        throw core::custom_error("filesystem - file \"" + p.native() + "\" already exists");

                    stream.open(p);
                    if (!stream)
                        throw core::custom_error("filesystem - error when opening \"" + p.native() + "\" for writing");
                }
            }
            void end_item_(const core::value &v)
            {
                if (!v.is_array() && !v.is_object())
                {
                    stream.close();

#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    fs::path p = path() / key.get_string_unchecked();

                    core::value permissions = key.const_attribute("permissions");
                    if (permissions.is_uint())
                        fs::permissions(p, static_cast<fs::perms>(permissions.get_uint()) & fs::perms::mask);

                    core::value write_time = key.const_attribute("modified");
                    if (write_time.is_int())
                        fs::last_write_time(p, fs::file_time_type(std::chrono::seconds(write_time.get_int())));
#endif
                }
            }

            void bool_(const core::value &v) {stream << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v)
            {
                if (scalar_is_key)
                    key = std::to_string(v.get_int_unchecked());
                else
                    stream << v.get_int_unchecked();
            }
            void uinteger_(const core::value &v)
            {
                if (scalar_is_key)
                    key = std::to_string(v.get_uint_unchecked());
                else
                    stream << v.get_uint_unchecked();
            }
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("filesystem - cannot write 'NaN' or 'Infinity' values");

                if (scalar_is_key)
                    key = std::to_string(v.get_real_unchecked());
                else
                    stream << v.get_real_unchecked();
            }
            void begin_string_(const core::value &v, core::int_t, bool is_key)
            {
                if (is_key)
                {
                    key = v; // Copy attributes, if any
                    key.get_string_ref().clear();
                }
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    key.get_string_ref() += v.get_string_unchecked();
                else
                    stream << v.get_string_unchecked();
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                {
                    fs::path p = path() / key.get_string_unchecked();

                    if (fs::status(p).type() == fs::file_type::directory)
                    {
                        if (safe_dirs)
                            throw core::custom_error("filesystem - directory \"" + p.native() + "\" already exists");
                    }
                    else
                    {
                        if (!fs::create_directory(path() / key.get_string_unchecked()))
                            throw core::custom_error("filesystem - unable to create directory \"" + p.native() + "\"");
                    }

                    directories.push_back(key);
                }
            }
            void end_array_(const core::value &, bool)
            {
                if (nesting_depth() > 1)
                {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    fs::path p = path();

                    core::value permissions = directories.back().const_attribute("permissions");
                    if (permissions.is_uint())
                        fs::permissions(p, static_cast<fs::perms>(permissions.get_uint()) & fs::perms::mask);

                    core::value write_time = directories.back().const_attribute("modified");
                    if (write_time.is_int())
                        fs::last_write_time(p, fs::file_time_type(std::chrono::seconds(write_time.get_int())));
#endif

                    directories.pop_back();
                }
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                {
                    fs::path p = path() / key.get_string_unchecked();

                    if (fs::status(p).type() == fs::file_type::directory)
                    {
                        if (safe_dirs)
                            throw core::custom_error("filesystem - directory \"" + p.native() + "\" already exists");
                    }
                    else
                    {
                        if (!fs::create_directory(path() / key.get_string_unchecked()))
                            throw core::custom_error("filesystem - unable to create directory \"" + p.native() + "\"");
                    }

                    directories.push_back(key);
                }
            }
            void end_object_(const core::value &, bool)
            {
                if (nesting_depth() > 1)
                {
#ifdef CPPDATALIB_ENABLE_ATTRIBUTES
                    fs::path p = path();

                    core::value permissions = directories.back().const_attribute("permissions");
                    if (permissions.is_uint())
                        fs::permissions(p, static_cast<fs::perms>(permissions.get_uint()) & fs::perms::mask);

                    core::value write_time = directories.back().const_attribute("modified");
                    if (write_time.is_int())
                        fs::last_write_time(p, fs::file_time_type(std::chrono::seconds(write_time.get_int())));
#endif

                    directories.pop_back();
                }
            }
        };
    }
}

#endif

#endif // FILESYSTEM_H
