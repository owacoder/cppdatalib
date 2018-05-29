/*
 * stream_filters.h
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

#ifndef CPPDATALIB_STREAM_FILTERS_H
#define CPPDATALIB_STREAM_FILTERS_H

#include "value_builder.h"
#include <set> // For duplicate_key_check_filter
#include <list> // For duplicate_key_check filter
#include <algorithm> // For sorting and specialty filters
#include <functional> // For sorting
#include <cmath> // For M_PI and math functions

#include <cassert>

#ifndef M_PI
#define M_PI 3.141592653589793238462642795
#endif

namespace cppdatalib
{
    namespace core
    {
        namespace impl
        {
            class stream_filter_base : public stream_handler
            {
            protected:
                core::stream_handler &output;

            public:
                stream_filter_base(core::stream_handler &output)
                    : output(output)
                {}
                stream_filter_base(const stream_filter_base &f)
                    : output(f.output)
                {
                    assert(("cppdatalib::impl::stream_filter_base(const stream_filter_base &) - attempted to copy a stream_filter_base while active" && !f.active()));
                }
                stream_filter_base(stream_filter_base &&f)
                    : output(f.output)
                {
                    assert(("cppdatalib::impl::stream_filter_base(stream_filter_base &&) - attempted to move a stream_filter_base while active" && !f.active()));
                }

                unsigned int required_features() const {return output.required_features();}

            protected:
                void begin_() {output.begin();}
                void end_() {output.end();}

                bool write_(const value &v, bool is_key)
                {
                    (void) is_key;
                    output.write(v);
                    return true;
                }

                void begin_array_(const value &v, int_t size, bool)
                {
                    output.begin_array(v, size);
                }
                void end_array_(const value &v, bool)
                {
                    output.end_array(v);
                }

                void begin_object_(const value &v, int_t size, bool)
                {
                    output.begin_object(v, size);
                }
                void end_object_(const value &v, bool)
                {
                    output.end_object(v);
                }

                void begin_string_(const value &v, int_t size, bool)
                {
                    output.begin_string(v, size);
                }
                void string_data_(const value &v, bool)
                {
                    output.append_to_string(v);
                }
                void end_string_(const value &v, bool)
                {
                    output.end_string(v);
                }
            };

#ifndef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
            // TODO: implement conversions for when CPPDATALIB_DISABLE_CONVERSION_LOSS is defined

            template<core::type from, core::type to>
            struct stream_filter_converter
            {
                void operator()(core::value &value)
                {
                    if (value.get_type() != from || from == to)
                        return;

                    switch (from)
                    {
                        case null:
                            switch (to)
                            {
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case boolean:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case integer: value.set_int(value.get_bool_unchecked()); break;
                                case uinteger: value.set_uint(value.get_bool_unchecked()); break;
                                case real: value.set_real(value.get_bool_unchecked()); break;
                                case string: value.set_string(value.get_bool_unchecked()? "true": "false"); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case integer:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_int_unchecked() != 0); break;
                                case uinteger: value.convert_to_uint(); break;
                                case real: value.set_real(value.get_int_unchecked()); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case uinteger:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_uint_unchecked() != 0); break;
                                case integer: value.convert_to_int(); break;
                                case real: value.set_real(value.get_uint_unchecked()); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case real:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_real_unchecked() != 0.0); break;
                                case integer: value.convert_to_int(); break;
                                case uinteger: value.convert_to_uint(); break;
                                case string: value.convert_to_string(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case string:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(value.get_string_unchecked() == "true" || value.as_int()); break;
                                case integer: value.convert_to_int(); break;
                                case uinteger: value.convert_to_uint(); break;
                                case real: value.convert_to_real(); break;
                                case array: value.set_array(array_t()); break;
                                case object: value.set_object(object_t()); break;
                            }
                            break;
                        case array:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case object:
                                {
                                    core::value obj;

                                    if (value.size() % 2 != 0)
                                        throw core::error("cppdatalib::core::stream_filter_converter - cannot convert 'array' to 'object' with odd number of elements");

                                    for (size_t i = 0; i < value.size(); i += 2)
                                        obj.add_member(value[i], value[i+1]);

                                    value = obj;
                                    break;
                                }
                            }
                            break;
                        case object:
                            switch (to)
                            {
                                case null: value.set_null(); break;
                                case boolean: value.set_bool(false); break;
                                case integer: value.set_int(0); break;
                                case uinteger: value.set_uint(0); break;
                                case real: value.set_real(0.0); break;
                                case string: value.set_string(""); break;
                                case array:
                                {
                                    array_t arr;

                                    for (auto const &it: value.get_object_unchecked())
                                    {
                                        arr.data().push_back(it.first);
                                        arr.data().push_back(it.second);
                                    }

                                    value.set_array(arr);
                                    break;
                                }
                            }
                            break;
                    }
                }
            };
#endif // CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
        }

        enum buffer_filter_flags
        {
            buffer_none = 0x00,
            buffer_strings = 0x01,
            buffer_arrays = 0x02,
            buffer_objects = 0x04,
            buffer_ignore_reported_sizes = 0x08
        };

        class buffer_filter : public impl::stream_filter_base
        {
        protected:
            core::value cache_val;
            core::value_builder cache;

            const buffer_filter_flags flags;
            const size_t nesting_level;

            size_t ignore_nesting;

        public:
            buffer_filter(core::stream_handler &output, buffer_filter_flags flags, size_t nesting_level = 0)
                : stream_filter_base(output)
                , cache(cache_val)
                , flags(flags)
                , nesting_level(nesting_level)
            {}
            buffer_filter(const buffer_filter &f)
                : stream_filter_base(f.output)
                , cache(cache_val)
                , flags(f.flags)
                , nesting_level(f.nesting_level)
            {}
            buffer_filter(buffer_filter &&f)
                : stream_filter_base(f.output)
                , cache(cache_val)
                , flags(f.flags)
                , nesting_level(f.nesting_level)
            {}

            unsigned int required_features() const
            {
                unsigned int mask_out = 0;

                if (flags & buffer_strings)
                    mask_out |= requires_prefix_string_size | requires_buffered_strings;
                if (flags & buffer_arrays)
                    mask_out |= requires_prefix_array_size | requires_buffered_arrays;
                if (flags & buffer_objects)
                    mask_out |= requires_prefix_object_size | requires_buffered_objects;
                if (flags & buffer_ignore_reported_sizes) // No sizes are passed straight through unless they're part of a buffered object
                    // TODO: what is the proper "required features" return value if the buffer does not pass-through reported sizes?
                    mask_out = 0;

                return stream_filter_base::required_features() & ~mask_out;
            }

            std::string name() const
            {
                std::string n = "cppdatalib::core::buffer_filter(" + output.name();
                if (flags & buffer_strings)
                    n += ", buffer_strings";
                if (flags & buffer_arrays)
                    n += ", buffer_arrays";
                if (flags & buffer_objects)
                    n += ", buffer_objects";
                return n + ")";
            }

        protected:
            // Called when this filter is done buffering. This function should write the value to the output stream.
            // The default implementation is just a pass-through to the output, as you can see.
            virtual void write_buffered_value_(const value &v, bool is_key) {(void) is_key; output.write(v);}

            void begin_()
            {
                output.begin();
                ignore_nesting = 0;
            }
            void end_()
            {
                if (cache.active())
                    cache.end();
                output.end();
            }

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                if (ignore_nesting)
                    return true;
                else if (cache.active())
                    return cache.write(v);
                else
                    return output.write(v);
            }

            void begin_array_(const value &v, int_t size, bool is_key)
            {
                if (ignore_nesting)
                    ++ignore_nesting;
                else if (cache.active() || (flags & buffer_arrays && nesting_level <= nesting_depth()))
                {
                    if (size != unknown_size && uintmax_t(size) == v.size())
                    {
                        ++ignore_nesting;
                        write_buffered_value_(v, is_key);
                        return;
                    }

                    if (!cache.active())
                        cache.begin();

                    cache.begin_array(v, size);
                }
                else
                    output.begin_array(v, flags & buffer_ignore_reported_sizes? core::int_t(core::stream_handler::unknown_size): size);
            }
            void end_array_(const value &v, bool is_key)
            {
                if (ignore_nesting)
                    --ignore_nesting;
                else if (cache.active())
                {
                    cache.end_array(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool is_key)
            {
                if (ignore_nesting)
                    ++ignore_nesting;
                else if (cache.active() || (flags & buffer_objects && nesting_level <= nesting_depth()))
                {
                    if (size != unknown_size && uintmax_t(size) == v.size())
                    {
                        ++ignore_nesting;
                        write_buffered_value_(v, is_key);
                        return;
                    }

                    if (!cache.active())
                        cache.begin();

                    cache.begin_object(v, size);
                }
                else
                    output.begin_object(v, flags & buffer_ignore_reported_sizes? core::int_t(core::stream_handler::unknown_size): size);
            }
            void end_object_(const value &v, bool is_key)
            {
                if (ignore_nesting)
                    --ignore_nesting;
                else if (cache.active())
                {
                    cache.end_object(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool is_key)
            {
                if (ignore_nesting)
                    ++ignore_nesting;
                else if (cache.active() || (flags & buffer_strings && nesting_level <= nesting_depth()))
                {
                    if (size != unknown_size && uintmax_t(size) == v.size())
                    {
                        ++ignore_nesting;
                        write_buffered_value_(v, is_key);
                        return;
                    }

                    if (!cache.active())
                        cache.begin();

                    cache.begin_string(v, size);
                }
                else
                    output.begin_string(v, flags & buffer_ignore_reported_sizes? core::int_t(core::stream_handler::unknown_size): size);
            }
            void string_data_(const value &v, bool)
            {
                if (ignore_nesting)
                    return;
                else if (cache.active())
                    cache.append_to_string(v);
                else
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool is_key)
            {
                if (ignore_nesting)
                    --ignore_nesting;
                else if (cache.active())
                {
                    cache.end_string(v);

                    if (cache.nesting_depth() == 0) // This was the last container
                    {
                        cache.end();
                        write_buffered_value_(cache_val, is_key);
                        cache_val.set_null();
                    }
                }
                else
                    output.end_string(v);
            }
        };

        class automatic_buffer_filter : public core::buffer_filter
        {
        public:
            automatic_buffer_filter(core::stream_handler &output)
                : buffer_filter(output, static_cast<buffer_filter_flags>(
                                    (output.required_features() & (stream_handler::requires_prefix_array_size | stream_handler::requires_buffered_arrays)? buffer_arrays: 0) |
                                    (output.required_features() & (stream_handler::requires_prefix_object_size | stream_handler::requires_buffered_objects)? buffer_objects: 0) |
                                    (output.required_features() & (stream_handler::requires_prefix_string_size | stream_handler::requires_buffered_strings)? buffer_strings: 0)))
            {}

            std::string name() const
            {
                return "cppdatalib::core::automatic_buffer_filter(" + output.name() + ")";
            }
        };

        class tee_filter : public impl::stream_filter_base
        {
        protected:
            core::stream_handler &output2;

        public:
            tee_filter(core::stream_handler &output,
                       core::stream_handler &output2)
                : stream_filter_base(output)
                , output2(output2)
            {}

            unsigned int required_features() const {return stream_filter_base::required_features() | output2.required_features();}

            std::string name() const
            {
                return "cppdatalib::core::tee_filter(" + output.name() + ", " + output2.name() + ")";
            }

        protected:
            void begin_() {stream_filter_base::begin_(); output2.begin();}
            void end_() {stream_filter_base::end_(); output2.end();}

            bool write_(const value &v, bool is_key)
            {
                (void) is_key;
                output.write(v);
                output2.write(v);
                return true;
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                output.begin_array(v, size);
                output2.begin_array(v, size);
            }
            void end_array_(const value &v, bool)
            {
                output.end_array(v);
                output2.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                output.begin_object(v, size);
                output2.begin_object(v, size);
            }
            void end_object_(const value &v, bool)
            {
                output.end_object(v);
                output2.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                output.begin_string(v, size);
                output2.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                output.append_to_string(v);
                output2.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                output.end_string(v);
                output2.end_string(v);
            }
        };

        template<core::type measure, typename Viewer>
        class view_filter : public core::buffer_filter
        {
            Viewer view;

        public:
            view_filter(core::stream_handler &output, Viewer v = Viewer())
                : buffer_filter(output, static_cast<buffer_filter_flags>((measure == core::string? buffer_strings: 0) |
                                                                         (measure == core::array? buffer_arrays: 0) |
                                                                         (measure == core::object? buffer_objects: 0)))
                , view(v)
            {}

            std::string name() const
            {
                return "cppdatalib::core::view_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == measure)
                    view(v);
                buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == measure)
                    view(v);
                return buffer_filter::write_(v, is_key);
            }
        };

        template<core::type measure, typename Viewer>
        view_filter<measure, Viewer> make_view_filter(core::stream_handler &output, Viewer v)
        {
            return view_filter<measure, Viewer>(output, v);
        }

        class object_keys_to_array_filter : public impl::stream_filter_base
        {
            size_t m_is_key; // Number of nested objects to write
            size_t m_ignore_key; // Number of nested objects to ignore
            bool in_object;

        public:
            object_keys_to_array_filter(core::stream_handler &output)
                : impl::stream_filter_base(output)
            {}

            std::string name() const
            {
                return "cppdatalib::core::object_keys_to_array_filter(" + output.name() + ")";
            }

        protected:
            void begin_() {
                impl::stream_filter_base::begin_();
                output.begin_array(core::array_t(), core::stream_handler::unknown_size);
                m_is_key = 0;
                m_ignore_key = 0;
                in_object = false;
            }
            void end_()
            {
                output.end_array(core::array_t());
                impl::stream_filter_base::end_();
            }

            bool write_(const value &v, bool is_key)
            {
                if (!m_ignore_key && (m_is_key || is_key))
                    return output.write(v);
                return false;
            }

            void begin_array_(const value &v, int_t size, bool is_key)
            {
                if (!m_ignore_key && (m_is_key || is_key))
                {
                    ++m_is_key;
                    output.begin_array(v, size);
                }
            }
            void end_array_(const value &v, bool)
            {
                if (!m_ignore_key && m_is_key)
                {
                    --m_is_key;
                    output.end_array(v);
                }
            }

            void begin_object_(const value &v, int_t size, bool is_key)
            {
                if (!in_object) // If not handling an object, start one
                {
                    output.begin_array(core::array_t(), size);
                    in_object = true;
                }
                else if (!m_ignore_key && (m_is_key || is_key)) // If handling an object, and this is part of a key, add it
                {
                    ++m_is_key;
                    output.begin_object(v, size);
                }
                else
                    ++m_ignore_key;
            }
            void end_object_(const value &v, bool)
            {
                if (!m_ignore_key)
                {
                    if (!m_is_key) // If not in the middle of an object, end it
                    {
                        output.end_array(core::array_t());
                        in_object = false;
                    }
                    else // Otherwise, we're in the middle of a key, so finish the object
                    {
                        --m_is_key;
                        output.end_object(v);
                    }
                }
                else
                    --m_ignore_key;
            }

            void begin_string_(const value &v, int_t size, bool is_key)
            {
                if (!m_ignore_key && (m_is_key || is_key))
                    output.begin_string(v, size);
            }
            void string_data_(const value &v, bool is_key)
            {
                if (!m_ignore_key && (m_is_key || is_key))
                    output.append_to_string(v);
            }
            void end_string_(const value &v, bool is_key)
            {
                if (!m_ignore_key && (m_is_key || is_key))
                    output.end_string(v);
            }
        };

        template<core::type measure = core::real>
        class range_filter : public core::buffer_filter
        {
            core::value max_, min_;
            bool started;

            void check(const value &v)
            {
                if (!started)
                {
                    max_ = min_ = v;
                    started = true;
                }
                else if (v < min_)
                    min_ = v;
                else if (max_ < v)
                    max_ = v;
            }

        public:
            range_filter(core::stream_handler &output)
                : buffer_filter(output, static_cast<buffer_filter_flags>((measure == core::string? buffer_strings: 0) |
                                                                         (measure == core::array? buffer_arrays: 0) |
                                                                         (measure == core::object? buffer_objects: 0)))
            {}

            const core::value &get_max() const {return max_;}
            const core::value &get_min() const {return min_;}
            core::value get_midpoint() const {return (core::real_t(max_) + core::real_t(min_)) / 2.0;}

            std::string name() const
            {
                return "cppdatalib::core::range_filter(" + output.name() + ")";
            }

        protected:
            void begin_()
            {
                buffer_filter::begin_();
                started = false;
            }

            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == measure)
                    check(v);
                buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == measure)
                    check(v);
                return buffer_filter::write_(v, is_key);
            }
        };

        enum mean_filter_flag
        {
            arithmetic_mean,
            geometric_mean,
            harmonic_mean
        };

        template<core::type measure = core::real>
        class mean_filter : public impl::stream_filter_base
        {
            size_t samples;
            core::real_t sum, product, inverted_sum;

        public:
            mean_filter(core::stream_handler &output)
                : stream_filter_base(output)
                , samples(0)
                , sum(0)
                , product(0)
                , inverted_sum(0)
            {}

            core::real_t get_mean(mean_filter_flag mean_type) const
            {
                if (samples == 0)
                    return NAN;

                switch (mean_type)
                {
                    case arithmetic_mean: return sum / samples;
                    case geometric_mean: return pow(product, 1.0 / samples);
                    case harmonic_mean: return samples / inverted_sum;
                    default: return NAN;
                }
            }

            core::real_t get_arithmetic_mean() const {return get_mean(arithmetic_mean);}
            core::real_t get_geometric_mean() const {return get_mean(geometric_mean);}
            core::real_t get_harmonic_mean() const {return get_mean(harmonic_mean);}

            size_t sample_size() const
            {
                return samples;
            }

            std::string name() const
            {
                return "cppdatalib::core::mean_filter(" + output.name() + ")";
            }

        protected:
            void begin_()
            {
                stream_filter_base::begin_();
                samples = 0;
                inverted_sum = sum = 0;
                product = 1;
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.is_object() || v.is_array())
                    return false;

                (void) is_key;
                if (v.get_type() == measure)
                {
                    core::real_t value = v.as_real();

                    ++samples;

                    sum += value;
                    product *= value;
                    if (value == 0.0)
                        inverted_sum = INFINITY;
                    else
                        inverted_sum += 1.0 / value;
                }
                output.write(v);
                return true;
            }
        };

        template<core::type measure = core::real>
        class dispersion_filter : public core::mean_filter<measure>
        {
            typedef core::mean_filter<measure> base;

            core::value samples;
            core::real_t variance_;
            bool calculated;

        public:
            dispersion_filter(core::stream_handler &output)
                : base(output)
                , calculated(false)
            {}

            core::real_t get_variance()
            {
                if (!calculated && samples.is_array())
                {
                    variance_ = 0.0;
                    for (auto const &sample: samples.get_array_unchecked())
                        variance_ += pow(sample.as_real() - base::get_arithmetic_mean(), 2.0);
                    calculated = true;
                }

                return variance_ / base::sample_size();
            }

            core::real_t get_standard_deviation() {return sqrt(get_variance());}

            std::string name() const
            {
                return "cppdatalib::core::dispersion_filter(" + base::output.name() + ")";
            }

        protected:
            void begin_()
            {
                base::begin_();
                samples.set_null();
                calculated = false;
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.is_object() || v.is_array())
                    return false;

                base::write_(v, is_key);
                if (v.get_type() == measure)
                {
                    samples.push_back(v);
                    calculated = false;
                }
                return true;
            }
        };

        template<core::type measure = core::real>
        class median_filter : public impl::stream_filter_base
        {
            core::value samples;
            bool sorted;

        public:
            median_filter(core::stream_handler &output)
                : stream_filter_base(output)
                , sorted(false)
            {}

            core::value get_median()
            {
                using namespace std;

                if (!sorted && samples.is_array())
                {
                    sort(samples.get_array_ref().begin().data(), samples.get_array_ref().end().data());
                    sorted = true;
                }

                if (samples.array_size() == 0) // No samples
                    return core::null_t();
                else if (samples.array_size() & 1) // Odd number of samples
                    return samples.element(samples.array_size() / 2);
                else // Even number of samples
                    return cppdatalib::core::value((samples.element(samples.array_size() / 2 - 1).as_real() +
                            samples.element(samples.array_size() / 2).as_real()) / 2);
            }

            size_t sample_size() const
            {
                return samples.size();
            }

            std::string name() const
            {
                return "cppdatalib::core::median_filter(" + output.name() + ")";
            }

        protected:
            void begin_()
            {
                stream_filter_base::begin_();
                samples.set_null();
                sorted = false;
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.is_object() || v.is_array())
                    return false;

                stream_filter_base::write_(v, is_key);
                if (v.get_type() == measure)
                {
                    samples.push_back(v);
                    sorted = false;
                }
                return true;
            }
        };

        template<core::type measure = core::real>
        class mode_filter : public impl::stream_filter_base
        {
            core::value samples;
            core::value modes;
            bool calculated;

        public:
            mode_filter(core::stream_handler &output)
                : stream_filter_base(output)
                , calculated(false)
            {}

            const core::value &get_modes()
            {
                using namespace std;

                if (!calculated)
                {
                    core::uint_t max_frequency = 1;
                    modes.set_null();

                    if (samples.is_object())
                    {
                        for (const auto &item: samples.get_object_unchecked())
                            max_frequency = std::max(item.second.get_uint_unchecked(), max_frequency); // Don't worry about get_uint_unchecked, we know all values are unsigned integers

                        for (const auto &item: samples.get_object_unchecked())
                        {
                            if (item.second.get_uint_unchecked() == max_frequency)
                                modes.push_back(item.first);
                        }

                        if (modes.size() == samples.size())
                            modes.set_null();
                    }

                    calculated = true;
                }

                return modes;
            }

            size_t sample_size() const
            {
                return samples.size();
            }

            std::string name() const
            {
                return "cppdatalib::core::mode_filter(" + output.name() + ")";
            }

        protected:
            void begin_()
            {
                stream_filter_base::begin_();
                samples.set_null();
                calculated = false;
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.is_object() || v.is_array())
                    return false;

                stream_filter_base::write_(v, is_key);
                if (v.get_type() == measure)
                {
                    samples.member(v).get_uint_ref() += 1;
                    calculated = false;
                }
                return true;
            }
        };

        enum sort_filter_flag
        {
            ascending_sort,
            descending_sort
        };

        template<sort_filter_flag direction = ascending_sort>
        class array_sort_filter : public core::buffer_filter
        {
        public:
            array_sort_filter(core::stream_handler &output, size_t nesting_level = 0)
                : core::buffer_filter(output, buffer_arrays, nesting_level)
            {}

            std::string name() const
            {
                return "cppdatalib::core::array_sort_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                using namespace std;

                if (v.get_type() == core::array)
                {
                    core::value sorted = v;

                    if (direction == ascending_sort)
                        sort(sorted.get_array_ref().data().begin(), sorted.get_array_ref().data().end());
                    else
						sort(sorted.get_array_ref().data().begin(), sorted.get_array_ref().data().end(), std::greater<cppdatalib::core::value>());

                    buffer_filter::write_buffered_value_(sorted, is_key);
                }
                else
                    buffer_filter::write_buffered_value_(v, is_key);
            }
        };

        class array_of_objects_to_object_of_arrays_filter : public core::buffer_filter
        {
            bool fail_on_extra_column;

        public:
            array_of_objects_to_object_of_arrays_filter(core::stream_handler &output, bool fail_on_extra_column = false)
                : core::buffer_filter(output, buffer_arrays)
                , fail_on_extra_column(fail_on_extra_column)
            {}

            std::string name() const
            {
                return "cppdatalib::core::array_of_objects_to_object_of_arrays_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                core::value map_ = core::value(core::object_t());

                if (v.is_array())
                {
                    size_t i = 0;

                    for (const auto &element: v.get_array_unchecked())
                    {
                        if (element.is_object())
                        {
                            for (const auto &pair: element.get_object_unchecked())
                            {
                                core::value &ref = map_.member(pair.first);
                                if (fail_on_extra_column && ref.array_size() + 1 < i)
                                    throw core::error("cppdatalib::core::array_of_objects_to_object_of_arrays_filter - extra column found in array entry");
                                ref[i] = pair.second;
                            }
                        }
                        else
                            throw core::error("cppdatalib::core::array_of_objects_to_object_of_arrays_filter - array entry is not an object");

                        ++i;
                    }
                }

                buffer_filter::write_buffered_value_(map_, is_key);
            }
        };

        class table_to_array_of_objects_filter : public core::buffer_filter
        {
            core::value column_names;
            bool fail_on_missing_column;

        public:
            table_to_array_of_objects_filter(core::stream_handler &output, const core::value &column_names, bool fail_on_missing_column = false)
                : core::buffer_filter(output, buffer_arrays, 1)
                , column_names(column_names)
                , fail_on_missing_column(fail_on_missing_column)
            {}

            std::string name() const
            {
                return "cppdatalib::core::table_to_array_of_objects_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                core::value map_;

                if ((v.is_array() && v.array_size() > column_names.array_size())
                        || column_names.array_size() == 0) // Not enough column names to cover all attributes!!
                    throw core::error("cppdatalib::core::table_to_array_of_objects_filter - not enough column names provided for specified data");

                if (v.is_array())
                    for (size_t i = 0; i < column_names.array_size(); ++i)
                    {
                        if (i >= v.size() && fail_on_missing_column)
                            throw core::error("cppdatalib::core::table_to_array_of_objects_filter - missing column entry in table row");
                        map_.add_member(column_names[i], i < v.size()? v[i]: core::value(core::null_t()));
                    }
                else
                    map_.add_member(column_names.element(0), v);

                buffer_filter::write_buffered_value_(map_, is_key);
            }
        };

        class duplicate_key_check_filter : public impl::stream_filter_base
        {
            class layer
            {
                void operator=(const layer &) {}

            public:
                layer() : key_builder(key) {}
                // WARNING: layers should NOT be copied while key_builder.active() is true!!!
                // This will corrupt the internal state, since value_builders cannot be copied!
                // The following constructors are SOLELY for interoperability with the STL, when the layer is inactive
                layer(const layer &other)
                    : key(other.key)
                    , key_builder(key)
                    , keys(other.keys)
                {}
                layer(layer &&other)
                    : key(std::move(other.key))
                    , key_builder(key)
                    , keys(std::move(other.keys))
                {}

                void begin() {key_builder.begin();}
                void end() {key_builder.end();}

                value key;
                value_builder key_builder;
                std::set<core::value> keys;
            };

            // WARNING: Underlying container type of `layer`s MUST not copy stored layers when adding to it,
            // so the value_builders in the layer don't get corrupted (i.e. DON'T USE A VECTOR)
            std::list<layer> layers;

        public:
            duplicate_key_check_filter(core::stream_handler &output) : stream_filter_base(output) {}

            std::string name() const
            {
                return "cppdatalib::core::duplicate_key_check_filter(" + output.name() + ")";
            }

        protected:
            void begin_() {stream_filter_base::begin_(); layers.clear();}

            void begin_key_(const value &) {layers.back().begin();}
            void end_key_(const value &)
            {
                layers.back().end();

                // Check against already parsed keys for current object, if it exists, throw an error
                if (layers.back().keys.find(layers.back().key) != layers.back().keys.end())
                    throw core::error("cppdatalib::core::duplicate_key_check_filter - invalid duplicate object key found");
                else
                    layers.back().keys.insert(layers.back().key);
            }

            void begin_scalar_(const value &v, bool)
            {
                output.write(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.write(v);
            }

            void begin_array_(const value &v, int_t size, bool)
            {
                output.begin_array(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_array(v, size);
            }
            void end_array_(const value &v, bool)
            {
                output.end_array(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_array(v);
            }

            void begin_object_(const value &v, int_t size, bool)
            {
                output.begin_object(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_object(v, size);
                layers.push_back(layer());
            }
            void end_object_(const value &v, bool)
            {
                output.end_object(v);
                layers.pop_back();
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_object(v);
            }

            void begin_string_(const value &v, int_t size, bool)
            {
                output.begin_string(v, size);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.begin_string(v, size);
            }
            void string_data_(const value &v, bool)
            {
                output.append_to_string(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.append_to_string(v);
            }
            void end_string_(const value &v, bool)
            {
                output.end_string(v);
                for (auto i = layers.begin(); i != layers.end(); ++i)
                    if (i->key_builder.active())
                        i->key_builder.end_string(v);
            }
        };

        template<typename Selecter>
        class select_from_array_filter : public core::buffer_filter
        {
            Selecter select;

        public:
            select_from_array_filter(core::stream_handler &output, Selecter s = Selecter())
                : buffer_filter(output, static_cast<buffer_filter_flags>(buffer_strings | buffer_arrays | buffer_objects | buffer_ignore_reported_sizes), 1)
                , select(s)
            {}

            std::string name() const
            {
                return "cppdatalib::core::select_from_array_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (select(v))
                    buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (current_container() != core::array)
                    return false;

                if (select(v))
                    return buffer_filter::write_(v, is_key);
                return true; // The value was handled, so don't keep processing it
            }
        };

        template<typename Selecter>
        select_from_array_filter<Selecter> make_select_from_array_filter(core::stream_handler &output, Selecter s)
        {
            return select_from_array_filter<Selecter>(output, s);
        }

        namespace impl
        {
            struct sql_function
            {
                // Args should be equal to (-n - 1) for a minimum number of arguments equal to n.
                // Args should be equal to n for an exact number of arguments equal to n.
                sql_function(long args,
                             std::function<core::value (const core::value &)> impl)
                    : args(args)
                    , impl(impl)
                {}

                bool proper_number_of_arguments(size_t passed_args) const
                {
                    if (args < 0)
                        return size_t(-args) - 1 <= passed_args;
                    else
                        return size_t(args) == passed_args;
                }

                long args;
                std::function<core::value (const core::value &)> impl;
            };

            typedef std::map<std::string, sql_function> sql_function_list;

            // This evaluator can only test objects (i.e. tuples must be represented as objects, not arrays)
            // Tuple properties may be objects (that is, in the incoming data),
            // but object values may not appear in any queries since objects are used for the query heirarchy.
            //
            // If values have the subtype "symbol", it refers to the field name, not a specific string.
            // This means field names are only capable of referencing keys with the "normal" UTF-8 subtype.
            //
            // Equality compare:
            //      {"eq": [value, value]}
            // Inequality compare:
            //      {"neq": [value, value]}
            // Less-than compare:
            //      {"lt": [value, value]}
            // Greater-than compare:
            //      {"gt": [value, value]}
            // Less-than or equal to compare:
            //      {"lte": [value, value]}
            // Greater-than or equal to compare:
            //      {"gte": [value, value]}
            // Logical And (short-circuits, doesn't compute second value if first is false):
            //      {"and": [value, value]}
            // Logical Or (short-circuits, doesn't compute second value if first is true):
            //      {"or": [value, value]}
            // Between compare (first value is test value, second is start of range, third is end of range, all values inclusive):
            //      {"between": [value, value, value]}
            // In-set compare (first value is test value, second is list of options to match equality against):
            //      {"in": [value, value]}
            // Null compare (value is value to test for null):
            //      {"nul": value}
            // Not-null compare (value is value to test for non-null):
            //      {"nnul": value}
            // Not distinct from (both nulls or both equal):
            //      {"ndistinct": [value, value]}
            //
            // The following are not specific operators, but rather a sort of mode of operation (used for sql_select_filter)
            // `non_bool_result` must be non-null to use these fields
            //
            // Select only specific fields in tuple (values are either field names, or must be renamed with the "as" operator):
            // The value should be null if all fields are requested, since an empty array means select nothing from the tuple
            //      {"select": [value, ...]}
            // Rename field in tuple (rename first (either name or expression) to second (must be a name):
            //      {"as": [value, value]}
            //
            class sql_expression_evaluator
            {
                const sql_function_list &functions_;
                const core::value &query;

                enum operator_type
                {
                    select,
                    where,

                    func,

                    equals,
                    nequals,
                    less_than,
                    greater_than,
                    less_than_or_equal,
                    greater_than_or_equal,
                    in_set,
                    not_distinct,
                    is, // Is
                    isnt, // Is not
                    AND,
                    OR,
                    XOR,
                    as,
                    add,
                    sub,
                    mul,
                    div,
                    mod
                };

                static bool compare(operator_type type, const core::value *one, const core::value *two, core::value *non_bool_result = nullptr)
                {
                    bool result = false;

                    switch (type)
                    {
                        case equals:
                        case not_distinct:
                        {
                            if (type == not_distinct && one->is_null() && two->is_null())
                            {
                                if (non_bool_result)
                                    *non_bool_result = core::value(true);
                                return true;
                            }

                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() == two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else if (one->get_string_unchecked().size() != two->get_string_unchecked().size())
                                    result = false;
                                else
                                {
                                    result = true;

                                    for (size_t i = 0; i < one->get_string_unchecked().size(); ++i)
                                        if (tolower(one->get_string_unchecked()[i] & 0xff) != tolower(two->get_string_unchecked()[i] & 0xff))
                                            result = false;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() == two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() == two->as_uint();
                                else
                                    result = one->as_real() == two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() == two->as_uint();
                            else
                                result = one->as_real() == two->as_real();

                            break;
                        }
                        case is:
                        {
                            if (one->is_null() && two->is_null())
                            {
                                if (non_bool_result)
                                    *non_bool_result = core::value(true);
                                return true;
                            }

                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    *non_bool_result = core::value(false);
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() == two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else if (one->get_string_unchecked().size() != two->get_string_unchecked().size())
                                    result = false;
                                else
                                {
                                    result = true;

                                    for (size_t i = 0; i < one->get_string_unchecked().size(); ++i)
                                        if (tolower(one->get_string_unchecked()[i] & 0xff) != tolower(two->get_string_unchecked()[i] & 0xff))
                                            result = false;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() == two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() == two->as_uint();
                                else
                                    result = one->as_real() == two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() == two->as_uint();
                            else
                                result = one->as_real() == two->as_real();

                            break;
                        }
                        case isnt:
                        {
                            if (one->is_null() && two->is_null())
                            {
                                if (non_bool_result)
                                    *non_bool_result = core::value(false);
                                return false;
                            }

                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    *non_bool_result = core::value(true);
                                return true;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() != two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else if (one->get_string_unchecked().size() != two->get_string_unchecked().size())
                                    result = true;
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < one->get_string_unchecked().size(); ++i)
                                        if (tolower(one->get_string_unchecked()[i] & 0xff) != tolower(two->get_string_unchecked()[i] & 0xff))
                                            result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() != two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() != two->as_uint();
                                else
                                    result = one->as_real() != two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() != two->as_uint();
                            else
                                result = one->as_real() != two->as_real();

                            break;
                        }
                        case nequals:
                        {
                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return true;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() != two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else if (one->get_string_unchecked().size() != two->get_string_unchecked().size())
                                    result = true;
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < one->get_string_unchecked().size(); ++i)
                                        if (tolower(one->get_string_unchecked()[i] & 0xff) != tolower(two->get_string_unchecked()[i] & 0xff))
                                            result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() != two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() != two->as_uint();
                                else
                                    result = one->as_real() != two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() != two->as_uint();
                            else
                                result = one->as_real() != two->as_real();

                            break;
                        }
                        case less_than:
                        {
                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() < two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < std::min(one->get_string_unchecked().size(), two->get_string_unchecked().size()); ++i)
                                    {
                                        int a = tolower(one->get_string_unchecked()[i] & 0xff);
                                        int b = tolower(two->get_string_unchecked()[i] & 0xff);
                                        if (a < b)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return true;
                                        }
                                        else if (b < a)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return false;
                                        }
                                    }

                                    if (one->get_string_unchecked().size() < two->get_string_unchecked().size())
                                        result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() < two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() < two->as_uint();
                                else
                                    result = one->as_real() < two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() < two->as_uint();
                            else
                                result = one->as_real() < two->as_real();

                            break;
                        }
                        case greater_than:
                        {
                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() > two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < std::min(one->get_string_unchecked().size(), two->get_string_unchecked().size()); ++i)
                                    {
                                        int a = tolower(one->get_string_unchecked()[i] & 0xff);
                                        int b = tolower(two->get_string_unchecked()[i] & 0xff);
                                        if (a > b)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return true;
                                        }
                                        else if (b > a)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return false;
                                        }
                                    }

                                    if (one->get_string_unchecked().size() > two->get_string_unchecked().size())
                                        result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() > two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() > two->as_uint();
                                else
                                    result = one->as_real() > two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() > two->as_uint();
                            else
                                result = one->as_real() > two->as_real();

                            break;
                        }
                        case less_than_or_equal:
                        {
                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() <= two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < std::min(one->get_string_unchecked().size(), two->get_string_unchecked().size()); ++i)
                                    {
                                        int a = tolower(one->get_string_unchecked()[i] & 0xff);
                                        int b = tolower(two->get_string_unchecked()[i] & 0xff);
                                        if (a < b)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return true;
                                        }
                                        else if (b < a)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return false;
                                        }
                                    }

                                    if (one->get_string_unchecked().size() <= two->get_string_unchecked().size())
                                        result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() <= two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() <= two->as_uint();
                                else
                                    result = one->as_real() <= two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() <= two->as_uint();
                            else
                                result = one->as_real() <= two->as_real();

                            break;
                        }
                        case greater_than_or_equal:
                        {
                            if (one->is_null() || two->is_null())
                            {
                                if (non_bool_result)
                                    non_bool_result->set_null();
                                return false;
                            }

                            if (one->is_string() && two->is_string())
                            {
                                // Compare binary strings as case sensitive
                                if (!core::subtype_is_text_string(one->get_subtype()) ||
                                        !core::subtype_is_text_string(two->get_subtype()))
                                    result = one->get_string_unchecked() >= two->get_string_unchecked();
                                // Compare text strings as case insensitive
                                else
                                {
                                    result = false;

                                    for (size_t i = 0; i < std::min(one->get_string_unchecked().size(), two->get_string_unchecked().size()); ++i)
                                    {
                                        int a = tolower(one->get_string_unchecked()[i] & 0xff);
                                        int b = tolower(two->get_string_unchecked()[i] & 0xff);
                                        if (a > b)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return true;
                                        }
                                        else if (b > a)
                                        {
                                            if (non_bool_result)
                                                *non_bool_result = core::value(result);
                                            return false;
                                        }
                                    }

                                    if (one->get_string_unchecked().size() >= two->get_string_unchecked().size())
                                        result = true;
                                }
                            }
                            else if (one->is_int())
                            {
                                if (two->is_int())
                                    result = one->get_int() >= two->get_int();
                                else if (two->is_uint())
                                    result = one->as_uint() >= two->as_uint();
                                else
                                    result = one->as_real() >= two->as_real();
                            }
                            else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                result = one->as_uint() >= two->as_uint();
                            else
                                result = one->as_real() >= two->as_real();

                            break;
                        }
                        default:
                            return false;
                    }

                    if (non_bool_result)
                        *non_bool_result = core::value(result);

                    return result;
                }

            public:
                sql_expression_evaluator(const sql_function_list &functions, const core::value &query) : functions_(functions), query(query) {}

                static bool are_equal(const core::value &one, const core::value &two) {return compare(equals, &one, &two);}

                bool operator()(const core::value &test, core::value *non_bool_result = nullptr, bool order_is_important = false)
                {
                    if (query.is_null())
                    {
                        if (non_bool_result)
                            *non_bool_result = true;
                        return true;
                    }

                    bool result = false;
                    operator_type operator_;

                    if (!test.is_object())
                        throw core::error("SQL - tuples must be represented as objects for 'SELECT' to work");

                    const core::value *predicate;

                    if ((operator_ = select, predicate = query.member_ptr("select")))
                    {
                        if (!non_bool_result)
                            throw core::error("SQL - cannot use 'SELECT' as a predicate");

                        if (predicate->is_null())
                        {
                            if (order_is_important)
                            {
                                non_bool_result->set_array(core::array_t());

                                for (const auto &item: test.get_object_unchecked())
                                    non_bool_result->push_back(core::value(core::object_t{{item.first, item.second}}));
                            }
                            else
                                *non_bool_result = test;
                            return true;
                        }
                        else if (predicate->is_array())
                        {
                            core::value temp_one;
                            const core::value *one;

                            if (order_is_important)
                                non_bool_result->set_array(core::array_t());
                            else
                                non_bool_result->set_object(core::object_t());

                            for (const auto &item: predicate->get_array_unchecked())
                            {
                                bool was_named_field = false;
                                one = &item;

                                if (one->is_string() && one->get_subtype() == core::symbol)
                                {
                                    was_named_field = true; // This query is a direct reference to a field
                                    one = test.member_ptr(core::value(one->get_string_unchecked(), core::domain_comparable));
                                    if (!one) // No member with specified name
                                    {
                                        *non_bool_result = core::value(false);
                                        return false;
                                    }
                                }
                                else if (one->is_object()) // Another predicate specified
                                {
                                    sql_expression_evaluator{functions_, *one}(test, &temp_one);

                                    one = &temp_one;
                                }

                                // Now check the results of our expression
                                if (order_is_important)
                                {
                                    if (was_named_field) // Is direct field selection
                                        non_bool_result->push_back(core::value(core::object_t{{core::value(item.get_string_unchecked()), *one}}));
                                    else if (one->is_object()) // Must be an 'AS' expression, providing the field name to write to
                                        non_bool_result->push_back(*one);
                                    else
                                        non_bool_result->push_back(core::value(core::object_t{{core::value(std::to_string(non_bool_result->array_size())), *one}}));
                                }
                                else
                                {
                                    if (was_named_field) // Is direct field selection
                                        non_bool_result->add_member_at_end(core::value(item.get_string_unchecked()), *one);
                                    else if (one->is_object()) // Must be an 'AS' expression, providing the field name to write to
                                        non_bool_result->add_member_at_end(one->get_object_unchecked().begin()->first,
                                                                           one->get_object_unchecked().begin()->second);
                                    else
                                        non_bool_result->add_member_at_end(core::value(std::to_string(non_bool_result->object_size())), *one);
                                }
                            }

                            return true;
                        }
                        else
                            throw core::error("SQL - unknown operation requested in expression");
                    }
                    else if ((operator_ = func, predicate = query.member_ptr("func")))
                    {
                        if (!non_bool_result)
                            throw core::error("SQL - cannot use a function call as a predicate");

                        if (predicate->is_array())
                        {
                            size_t idx = 0;

                            core::string_t function_name;
                            core::value parameters; // Contains an array of parameters

                            core::value temp_one;
                            const core::value *one;

                            for (const auto &item: predicate->get_array_unchecked())
                            {
                                if (idx == 0)
                                {
                                    if (!item.is_string())
                                        throw core::error("SQL - function name must be a string");
                                    function_name = static_cast<core::string_t>(item.get_string_unchecked());
                                }
                                else
                                {
                                    one = &item;

                                    if (one->is_string() && one->get_subtype() == core::symbol)
                                    {
                                        one = test.member_ptr(core::value(one->get_string_unchecked(), core::domain_comparable));
                                        if (!one) // No member with specified name
                                        {
                                            *non_bool_result = core::value(false);
                                            return false;
                                        }
                                    }
                                    else if (one->is_object()) // Another predicate specified
                                    {
                                        sql_expression_evaluator{functions_, *one}(test, &temp_one);

                                        one = &temp_one;
                                    }

                                    parameters.push_back(*one);
                                }

                                ++idx;
                            }

                            auto n = functions_.find(function_name);
                            if (n != functions_.end())
                            {
                                if (!n->second.proper_number_of_arguments(parameters.size()))
                                    throw core::custom_error("SQL - function '" + ascii_uppercase_copy(function_name) + "' being called improperly with " + std::to_string(parameters.size()) + " argument(s)");

                                *non_bool_result = n->second.impl(parameters);
                            }
                            else
                            {
                                throw core::custom_error("SQL - unknown function '" + ascii_uppercase_copy(function_name) + "' being called with " + std::to_string(parameters.size()) + " argument(s)");
                            }

                            return true;
                        }
                        else
                            throw core::error("SQL - unknown operation requested in expression");
                    }
                    else if ((operator_ = where, predicate = query.member_ptr("where")))
                    {
                        if (predicate->is_null())
                            result = true;
                        else if (predicate->is_object())
                            result = sql_expression_evaluator{functions_, *predicate}(test);
                    }

                    //
                    // Check for dual-argument expressions
                    //
                    else if ((operator_ = equals, predicate = query.member_ptr("eq")) ||
                        (operator_ = nequals, predicate = query.member_ptr("neq")) ||
                        (operator_ = less_than, predicate = query.member_ptr("lt")) ||
                        (operator_ = greater_than, predicate = query.member_ptr("gt")) ||
                        (operator_ = less_than_or_equal, predicate = query.member_ptr("lte")) ||
                        (operator_ = greater_than_or_equal, predicate = query.member_ptr("gte")) ||
                        (operator_ = in_set, predicate = query.member_ptr("in")) ||
                        (operator_ = not_distinct, predicate = query.member_ptr("ndistinct")) ||
                        (operator_ = AND, predicate = query.member_ptr("and")) ||
                        (operator_ = OR, predicate = query.member_ptr("or")) ||
                        (operator_ = XOR, predicate = query.member_ptr("xor")) ||
                        (operator_ = is, predicate = query.member_ptr("is")) ||
                        (operator_ = isnt, predicate = query.member_ptr("isnt")) ||
                        (non_bool_result &&
                         ((operator_ = add, predicate = query.member_ptr("add")) ||
                          (operator_ = sub, predicate = query.member_ptr("sub")) ||
                          (operator_ = mul, predicate = query.member_ptr("mul")) ||
                          (operator_ = div, predicate = query.member_ptr("div")) ||
                          (operator_ = mod, predicate = query.member_ptr("mod")) ||
                          (operator_ = as, predicate = query.member_ptr("as")))))
                    {
                        if (predicate->is_array() && predicate->array_size() == 2)
                        {
                            core::value temp_one, temp_two;
                            const core::value *one = &predicate->get_array_unchecked()[0];
                            const core::value *two = &predicate->get_array_unchecked()[1];

                            if (one->is_string() && one->get_subtype() == core::symbol)
                            {
                                one = test.member_ptr(core::value(one->get_string_unchecked(), core::domain_comparable));
                                if (!one) // No member with specified name
                                {
                                    if (non_bool_result)
                                        *non_bool_result = core::value(operator_ == nequals);
                                    return operator_ == nequals;
                                }
                            }
                            else if (one->is_object()) // Another predicate specified
                            {
                                sql_expression_evaluator{functions_, *one}(test, &temp_one);

                                one = &temp_one;
                            } // Otherwise a normal value is assumed

                            if (two->is_string() && two->get_subtype() == core::symbol)
                            {
                                two = test.member_ptr(core::value(two->get_string_unchecked(), core::domain_comparable));
                                if (operator_ == as)
                                {
                                    if (two)
                                        throw core::error("SQL - cannot rename field with 'AS' because field already exists in source tuple");
                                    two = &predicate->get_array_unchecked()[1]; // Reset back to the original value
                                }
                                else if (!two) // No member with specified name
                                {
                                    if (non_bool_result)
                                        *non_bool_result = core::value(operator_ == nequals);
                                    return operator_ == nequals;
                                }
                            }
                            else if (two->is_object()) // Another predicate specified
                            {
                                if (operator_ == as)
                                    throw core::error("SQL - invalid query has non-name specified as target of 'AS' expression");

                                sql_expression_evaluator{functions_, *two}(test, &temp_two);

                                two = &temp_two;
                            } // Otherwise a normal value is assumed

                            // Perform the operation
                            switch (operator_)
                            {
                                case equals:
                                case nequals:
                                case less_than:
                                case greater_than:
                                case less_than_or_equal:
                                case greater_than_or_equal:
                                case not_distinct:
                                case is:
                                case isnt:
                                    result = compare(operator_, one, two, non_bool_result);
                                    break;
                                case in_set:
                                {
                                    if (!two->is_array() || one->is_null())
                                    {
                                        if (non_bool_result)
                                            non_bool_result->set_null();
                                        return false;
                                    }

                                    for (const auto &item: two->get_array_unchecked())
                                        if (compare(equals, one, &item, non_bool_result))
                                            return true;

                                    result = false;
                                    break;
                                }
                                case AND:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        if (non_bool_result)
                                            non_bool_result->set_null();
                                        return false;
                                    }
                                    result = one->as_bool() && two->as_bool();
                                    break;
                                }
                                case OR:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        if (non_bool_result)
                                            non_bool_result->set_null();
                                        return false;
                                    }
                                    result = one->as_bool() || two->as_bool();
                                    break;
                                }
                                case XOR:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        if (non_bool_result)
                                            non_bool_result->set_null();
                                        return false;
                                    }
                                    result = one->as_bool() ^ two->as_bool();
                                    break;
                                }
                                case as:
                                {
                                    non_bool_result->set_object(core::object_t());
                                    non_bool_result->add_member_at_end(core::value(two->get_string_unchecked()), *one);

                                    return true;
                                }
                                case add:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        non_bool_result->set_null();
                                        return true;
                                    }

                                    if (one->is_int())
                                    {
                                        if (two->is_int())
                                            non_bool_result->set_int(one->get_int() + two->get_int());
                                        else if (two->is_uint())
                                            non_bool_result->set_uint(one->as_uint() + two->as_uint());
                                        else
                                            non_bool_result->set_real(one->as_real() + two->as_real());
                                    }
                                    else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                        non_bool_result->set_uint(one->as_uint() + two->as_uint());
                                    else
                                        non_bool_result->set_real(one->as_real() + two->as_real());

                                    return true;
                                }
                                case sub:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        non_bool_result->set_null();
                                        return true;
                                    }

                                    if (one->is_int())
                                    {
                                        if (two->is_int())
                                            non_bool_result->set_int(one->get_int() - two->get_int());
                                        else if (two->is_uint())
                                            non_bool_result->set_uint(one->as_uint() - two->as_uint());
                                        else
                                            non_bool_result->set_real(one->as_real() - two->as_real());
                                    }
                                    else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                        non_bool_result->set_uint(one->as_uint() - two->as_uint());
                                    else
                                        non_bool_result->set_real(one->as_real() - two->as_real());

                                    return true;
                                }
                                case mul:
                                {
                                    if (one->is_null() || two->is_null())
                                    {
                                        non_bool_result->set_null();
                                        return true;
                                    }

                                    if (one->is_int())
                                    {
                                        if (two->is_int())
                                            non_bool_result->set_int(one->get_int() * two->get_int());
                                        else if (two->is_uint())
                                            non_bool_result->set_uint(one->as_uint() * two->as_uint());
                                        else
                                            non_bool_result->set_real(one->as_real() * two->as_real());
                                    }
                                    else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                        non_bool_result->set_uint(one->as_uint() * two->as_uint());
                                    else
                                        non_bool_result->set_real(one->as_real() * two->as_real());

                                    return true;
                                }
                                case div:
                                {
                                    if (one->is_null() || two->is_null() || two->as_real() == 0.0)
                                    {
                                        non_bool_result->set_null();
                                        return true;
                                    }

                                    if (one->is_int())
                                    {
                                        if (two->is_int())
                                            non_bool_result->set_int(one->get_int() / two->get_int());
                                        else if (two->is_uint())
                                            non_bool_result->set_uint(one->as_uint() / two->as_uint());
                                        else
                                            non_bool_result->set_real(one->as_real() / two->as_real());
                                    }
                                    else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                        non_bool_result->set_uint(one->as_uint() / two->as_uint());
                                    else
                                        non_bool_result->set_real(one->as_real() / two->as_real());

                                    return true;
                                }
                                case mod:
                                {
                                    if (one->is_null() || two->is_null() || two->as_real() == 0.0)
                                    {
                                        non_bool_result->set_null();
                                        return true;
                                    }

                                    if (one->is_int())
                                    {
                                        if (two->is_int())
                                            non_bool_result->set_int(one->get_int() % two->get_int());
                                        else if (two->is_uint())
                                            non_bool_result->set_uint(one->as_uint() % two->as_uint());
                                        else
                                            non_bool_result->set_real(fmod(one->as_real(), two->as_real()));
                                    }
                                    else if (one->is_uint() && (two->is_int() || two->is_uint()))
                                        non_bool_result->set_uint(one->as_uint() % two->as_uint());
                                    else
                                        non_bool_result->set_real(fmod(one->as_real(), two->as_real()));

                                    return true;
                                }
                                default:
                                    break;
                            }
                        }
                        else
                            throw core::error("SQL - invalid number of arguments provided in expression");
                    }
                    else
                        throw core::error("SQL - unknown or invalid operator encountered in expression");

                    if (non_bool_result)
                        *non_bool_result = core::value(result);

                    return result;
                }
            };

            inline sql_function_list make_sql_function_list()
            {
                sql_function_list result;

                result.insert({"abs", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_int()? core::value(std::abs(v->as_int())): !v->is_real() && !v->is_string()? *v: core::value(fabs(v->as_real()));
                               })});
                result.insert({"acos", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < -1.0 || r > 1.0? core::value(): core::value(acos(r));
                               })});
                result.insert({"ascii", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::string_t s = v->as_string();
                                   return v->is_null()? *v: s.empty()? core::value(0): core::value(s.front() & 0xff);
                               })});
                result.insert({"asin", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < -1.0 || r > 1.0? core::value(): core::value(asin(r));
                               })});
                result.insert({"atan", sql_function(-2, [](const core::value &params)
                               {
                                   if (params.size() == 1)
                                   {
                                       const core::value *v = params.element_ptr(0);
                                       core::real_t r = v->as_real();
                                       return v->is_null()? core::value(): core::value(atan(r));
                                   }
                                   else if (params.size() == 2)
                                   {
                                       core::value y = params.const_element(0);
                                       core::value x = params.const_element(1);
                                       return y.is_null() || x.is_null()? core::value(): core::value(atan2(y.as_real(), x.as_real()));
                                   }
                                   else
                                       throw core::error("SQL - function 'ATAN' not called with one or two arguments");
                               })});
                result.insert({"atan2", sql_function(2, [](const core::value &params)
                               {
                                   core::value y = params.const_element(0);
                                   core::value x = params.const_element(1);
                                   return y.is_null() || x.is_null()? core::value(): core::value(atan2(y.as_real(), x.as_real()));
                               })});
                // TODO: `BIN()`
                result.insert({"bit_length", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? *v: core::value(v->as_string().size() * 8);
                               })});
                result.insert({"ceil", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return !v->is_real() && !v->is_string()? *v: core::value(ceil(v->as_real()));
                               })});
                result.insert({"ceiling", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return !v->is_real() && !v->is_string()? *v: core::value(ceil(v->as_real()));
                               })});
                // TODO: `CHAR()`
                result.insert({"char_length", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::string_t s = v->as_string();
                                   if (v->is_null())
                                       return core::value();

                                   size_t count = 0;
                                   for (size_t i = 0; i < s.size(); ++count)
                                       core::utf8_to_ucs(s, i, i);
                                   return core::value(count);
                               })});
                result.insert({"character_length", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::string_t s = v->as_string();
                                   if (v->is_null())
                                       return core::value();

                                   size_t count = 0;
                                   for (size_t i = 0; i < s.size(); ++count)
                                       core::utf8_to_ucs(s, i, i);
                                   return core::value(count);
                               })});
                result.insert({"concat", sql_function(-2, [](const core::value &params)
                               {
                                   core::value result;

                                   for (const auto &item: params.get_array_unchecked())
                                   {
                                       if (item.is_null())
                                           return core::value();
                                       else if (item.is_string() && !core::subtype_is_text_string(item.get_subtype()))
                                           result.set_subtype(core::blob);

                                       result.get_owned_string_ref() += item.as_string();
                                   }

                                   return result;
                               })});
                result.insert({"concat_ws", sql_function(-3, [](const core::value &params)
                               {
                                   core::value result;
                                   core::value separator = params.const_element(0);
                                   size_t idx = 0;

                                   if (separator.is_null())
                                       return separator;
                                   separator.convert_to_string();

                                   for (const auto &item: params.get_array_unchecked())
                                   {
                                       if (++idx <= 1 || item.is_null())
                                           continue;
                                       else if (item.is_string() && !core::subtype_is_text_string(item.get_subtype()))
                                           result.set_subtype(core::blob);

                                       if (idx > 2)
                                           result.get_owned_string_ref() += separator.get_string_unchecked();

                                       result.get_owned_string_ref() += item.as_string();
                                   }

                                   return result;
                               })});
                // TODO: `CONV()`
                result.insert({"cos", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? *v: core::value(cos(v->as_real()));
                               })});
                result.insert({"cot", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? *v: core::value(1.0 / tan(v->as_real()));
                               })});
                // TODO: `CRC32()`
                result.insert({"degrees", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? *v: core::value(v->as_real() / M_PI * 180);
                               })});
                result.insert({"elt", sql_function(-2, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   if (v->is_null() || v->as_int() < 1 || v->as_uint() >= params.size())
                                       return core::value();

                                   v = params.element_ptr(size_t(v->as_uint()));
                                   if (v->is_string())
                                       return *v;

                                   return core::value(v->as_string());
                               })});
                result.insert({"exp", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? core::value(): core::value(exp(v->as_real()));
                               })});
                // TODO: `EXPORT_SET()`
                result.insert({"floor", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return !v->is_real() && !v->is_string()? *v: core::value(floor(v->as_real()));
                               })});
                // TODO: `FORMAT()`
                // TODO: `HEX()`
                result.insert({"if", sql_function(3, [](const core::value &params)
                               {
                                   core::value predicate = params.const_element(0);
                                   return predicate.is_null() || predicate.as_real() == 0.0? params.const_element(2): params.const_element(1);
                               })});
                result.insert({"ifnull", sql_function(2, [](const core::value &params)
                               {
                                   core::value predicate = params.const_element(0);
                                   return predicate.is_null()? params.const_element(1): predicate;
                               })});
                result.insert({"nullif", sql_function(2, [](const core::value &params)
                               {
                                   core::value one = params.const_element(0);
                                   core::value two = params.const_element(1);
                                   return impl::sql_expression_evaluator::are_equal(one, two)? core::value(): one;
                               })});
                result.insert({"lcase", sql_function(1, [](const core::value &params)
                               {
                                   core::value v = params.const_element(0);
                                   if (v.is_null())
                                       return core::value();

                                   v.convert_to_string();
                                   ascii_lowercase(v.get_owned_string_ref().begin(), v.get_owned_string_ref().end());
                                   return v;
                               })});
                result.insert({"ln", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < 0.0? core::value(): core::value(log(r));
                               })});
                result.insert({"lower", sql_function(1, [](const core::value &params)
                               {
                                   core::value v = params.const_element(0);
                                   if (v.is_null())
                                       return core::value();

                                   v.convert_to_string();
                                   ascii_lowercase(v.get_owned_string_ref().begin(), v.get_owned_string_ref().end());
                                   return v;
                               })});
                // TODO: `LOG()`
                result.insert({"log2", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < 0.0? core::value(): core::value(log2(r));
                               })});
                result.insert({"log10", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < 0.0? core::value(): core::value(log(r));
                               })});
                // TODO: `MOD()`
                result.insert({"pi", sql_function(0, [](const core::value &)
                               {
                                   return core::value(M_PI);
                               })});
                result.insert({"pow", sql_function(2, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::value w = params.const_element(1);
                                   return v->is_null() || w.is_null()? core::value(): core::value(pow(v->as_real(), w.as_real()));
                               })});
                result.insert({"power", sql_function(2, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::value w = params.const_element(1);
                                   return v->is_null() || w.is_null()? core::value(): core::value(pow(v->as_real(), w.as_real()));
                               })});
                result.insert({"radians", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? core::value(): core::value(v->as_real() / 180 * M_PI);
                               })});
                // TODO: `RAND()`
                result.insert({"repeat", sql_function(2, [](const core::value &params)
                               {
                                   core::value str = params.const_element(0);
                                   core::value times = params.const_element(1);
                                   core::value result("");
                                   if (str.is_null() || times.is_null())
                                       return core::value();
                                   else if (times.as_int() < 1)
                                       return result;

                                   for (core::uint_t i = times.as_uint(); i; --i)
                                       result.get_owned_string_ref() += str.get_string_unchecked();

                                   return result;
                               })});
                // TODO: `ROUND()`
                // TODO: `SIGN()`
                result.insert({"sin", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? core::value(): core::value(sin(v->as_real()));
                               })});
                result.insert({"sqrt", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   core::real_t r = v->as_real();
                                   return v->is_null() || r < 0.0? core::value(): core::value(sqrt(r));
                               })});
                result.insert({"tan", sql_function(1, [](const core::value &params)
                               {
                                   const core::value *v = params.element_ptr(0);
                                   return v->is_null()? core::value(): core::value(tan(v->as_real()));
                               })});
                // TODO: `TRUNCATE()`
                result.insert({"ucase", sql_function(1, [](const core::value &params)
                               {
                                   core::value v = params.const_element(0);
                                   if (v.is_null())
                                       return core::value();

                                   v.convert_to_string();
                                   ascii_uppercase(v.get_owned_string_ref().begin(), v.get_owned_string_ref().end());
                                   return v;
                               })});
                result.insert({"upper", sql_function(1, [](const core::value &params)
                               {
                                   core::value v = params.const_element(0);
                                   if (v.is_null())
                                       return core::value();

                                   v.convert_to_string();
                                   ascii_uppercase(v.get_owned_string_ref().begin(), v.get_owned_string_ref().end());
                                   return v;
                               })});
                result.insert({"version", sql_function(0, [](const core::value &)
                               {
                                   return core::value("1.0.0");
                               })});

                return result;
            }
        }

        namespace impl
        {
            class sql_parser : public stream_parser
            {
                core::value select, where;

            public:
                enum element
                {
                    select_element,
                    where_element
                };

            private:
                element state;

            public:
                sql_parser(core::istream_handle input, element parse = select_element)
                    : stream_parser(input)
                    , state(parse)
                {}

                core::value get_select() const {return core::value(core::object_t{{core::value("select"), select}});}
                core::value get_where() const {return core::value(core::object_t{{core::value("where"), where}});}

            protected:
                void reset_() {select.set_null(); where.set_null(); stream() >> std::skipws;}

                void write_one_()
                {
                    char buffer[32];

                    if (!stream().read(buffer, 6) || (ascii_lowercase(buffer, buffer + 6), 0) || memcmp(buffer, "select", 6))
                        throw core::error("SQL - invalid statement");

                    if (state == select_element)
                    {
                        get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                        get_output()->write(core::value("select"));
                        get_output()->write(select = parse_select_expression());
                        get_output()->end_object(core::value(core::object_t()));
                    }
                    else
                        select = parse_select_expression();

                    char c;
                    stream() >> c;

                    if (!stream())
                        return;

                    if (tolower(c) == 'w')
                    {
                        buffer[0] = c;
                        if (!stream().read(buffer+1, 4) || (ascii_lowercase(buffer+1, buffer+5), 0) || memcmp(buffer, "where", 5))
                            throw core::error("SQL - invalid 'WHERE' clause");

                        if (state == where_element)
                        {
                            get_output()->begin_object(core::value(core::object_t()), core::stream_handler::unknown_size);
                            get_output()->write(core::value("where"));
                            get_output()->write(where = parse_expression());
                            get_output()->end_object(core::value(core::object_t()));
                        }
                        else
                            where = parse_expression();
                    }

                    stream() >> c;
                    if (stream())
                        throw core::custom_error("SQL - unexpected '" + std::string(1, c) + "' in statement");
                }

                core::value parse_select_expression()
                {
                    core::value result;
                    char c;

                    stream() >> c;

                    if (!stream())
                        throw core::error("SQL - 'SELECT' must be followed by an expression");
                    else if (c == '*')
                        return core::value();
                    else
                    {
                        stream().unget();

                        result.set_array(core::array_t());
                        do
                        {
                            result.push_back(parse_expression());
                            stream() >> c;
                        } while (stream() && c == ',');

                        if (stream())
                            stream().unget();

                        return result;
                    }
                }

                core::value parse_expression()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_or();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (isalpha(c & 0xff))
                        {
                            switch (tolower(c))
                            {
                                case 'a':
                                    c = stream().get();
                                    if (!stream() || tolower(c) != 's')
                                        throw core::error("SQL - invalid operator in expression");

                                    operand_key.set_string("as");
                                    break;
                                default:
                                    stream().unget();
                                    return result;
                            }
                        }
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_or()})}});
                    }

                    return result;
                }

                core::value parse_or()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_xor();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (isalpha(c & 0xff))
                        {
                            switch (tolower(c))
                            {
                                case 'o':
                                    c = stream().peek();
                                    if (!stream() || tolower(c) != 'r')
                                    {
                                        stream().unget();
                                        return result;
                                    }

                                    stream().get();
                                    operand_key.set_string("or");
                                    break;
                                default:
                                    stream().unget();
                                    return result;
                            }
                        }
                        else if (c == '|' && stream().peek() == '|')
                        {
                            stream().get(); // Eat '|'
                            operand_key.set_string("or");
                        }
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_xor()})}});
                    }

                    return result;
                }

                core::value parse_xor()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_and();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (isalpha(c & 0xff))
                        {
                            switch (tolower(c))
                            {
                                case 'x':
                                    c = stream().peek();
                                    if (!stream() || tolower(c) != 'o')
                                    {
                                        stream().unget();
                                        return result;
                                    }

                                    stream().get();
                                    if (tolower(stream().get()) != 'r')
                                        throw core::error("SQL - invalid operator in expression");

                                    operand_key.set_string("xor");
                                    break;
                                default:
                                    stream().unget();
                                    return result;
                            }
                        }
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_and()})}});
                    }

                    return result;
                }

                core::value parse_and()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_compare();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (isalpha(c & 0xff))
                        {
                            switch (tolower(c))
                            {
                                case 'a':
                                    c = stream().peek();
                                    if (!stream() || tolower(c) != 'n')
                                    {
                                        stream().unget();
                                        return result;
                                    }

                                    stream().get();
                                    if (tolower(stream().get()) != 'd')
                                        throw core::error("SQL - invalid operator in expression");

                                    operand_key.set_string("and");
                                    break;
                                default:
                                    stream().unget();
                                    return result;
                            }
                        }
                        else if (c == '&' && stream().peek() == '&')
                        {
                            stream().get(); // Eat '&'
                            operand_key.set_string("and");
                        }
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_compare()})}});
                    }

                    return result;
                }

                core::value parse_compare()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_plus_minus();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (c == '=')
                            operand_key.set_string("eq");
                        else if (c == '<')
                        {
                            c = stream().get();
                            if (stream() && (c == '>' || c == '='))
                            {
                                if (c == '>')
                                    operand_key.set_string("neq");
                                else
                                    operand_key.set_string("lte");
                            }
                            else
                            {
                                if (stream())
                                    stream().unget();
                                operand_key.set_string("lt");
                            }
                        }
                        else if (c == '>')
                        {
                            if (stream().get() == '=')
                                operand_key.set_string("gte");
                            else
                            {
                                if (stream())
                                    stream().unget();
                                operand_key.set_string("gt");
                            }
                        }
                        else if (c == '!' && stream().peek() == '=')
                        {
                            stream().get(); // Eat '='
                            operand_key.set_string("neq");
                        }
                        else if (tolower(c) == 'i' && tolower(stream().peek()) == 's')
                        {
                            bool negate = false;
                            std::string buffer;

                            stream().get(); // Eat 's'
                            if (!isspace(stream().peek()))
                                throw core::error("SQL - unknown operator in expression");

                            do
                            {
                                negate = buffer == "not";
                                buffer.clear();

                                stream() >> c;
                                if (!stream() || !isalpha(c & 0xff))
                                    throw core::error("SQL - missing argument in 'IS' expression");

                                while (isalpha(c & 0xff) && stream())
                                {
                                    buffer.push_back(tolower(c & 0xff));
                                    c = stream().get();
                                }

                                if (stream())
                                    stream().unget();
                            } while (buffer == "not" && !negate);

                            // `buffer` now contains the word after 'IS', converted to lowercase
                            if (buffer == "true")
                                result = core::value(core::object_t{{core::value(negate? "isnt": "is"), core::value(core::array_t{result, core::value(true)})}});
                            else if (buffer == "false")
                                result = core::value(core::object_t{{core::value(negate? "isnt": "is"), core::value(core::array_t{result, core::value(false)})}});
                            else if (buffer == "unknown" || buffer == "null")
                                result = core::value(core::object_t{{core::value(negate? "isnt": "is"), core::value(core::array_t{result, core::value()})}});
                            else
                                throw core::custom_error("SQL - invalid option '" + buffer + "' specified after 'IS" + (negate? " NOT": "") + "'");
                            continue;
                        }
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_plus_minus()})}});
                    }

                    return result;
                }

                core::value parse_plus_minus()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_times_div();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (c == '+')
                            operand_key.set_string("add");
                        else if (c == '-')
                            operand_key.set_string("sub");
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_times_div()})}});
                    }

                    return result;
                }

                core::value parse_times_div()
                {
                    char c;
                    core::value operand_key;
                    core::value result;

                    result = parse_value();

                    while (true)
                    {
                        stream() >> c;
                        if (!stream())
                            return result;

                        if (c == '*')
                            operand_key.set_string("mul");
                        else if (c == '/')
                            operand_key.set_string("div");
                        else if (c == '%')
                            operand_key.set_string("mod");
                        else
                        {
                            stream().unget();
                            return result;
                        }

                        result = core::value(core::object_t{{operand_key, core::value(core::array_t{result, parse_value()})}});
                    }

                    return result;
                }

                core::value parse_value(bool strictly_values = false)
                {
                    char c;

                    stream() >> c;

                    if (!stream())
                        throw core::error("SQL - expected value");
                    else if (c == '-')
                    {
                        core::value result = parse_value(strictly_values);

                        if (result.is_int() || result.is_uint())
                            result.set_int(-result.as_int());
                        else
                            result.set_real(-result.as_real());

                        return result;
                    }
                    else if (c == '+')
                    {
                        core::value result = parse_value(strictly_values);

                        if (!result.is_int() && !result.is_uint())
                            result.convert_to_real();

                        return result;
                    }
                    else if (c == '(' && !strictly_values)
                    {
                        core::value result = parse_expression();
                        stream() >> c;
                        if (!stream() || c != ')')
                            throw core::error("SQL - expected ')'");

                        return result;
                    }
                    else if (c == '\'' || c == '"' || c == '`')
                    {
                        core::string_t buffer;
                        char str_type = c;

                        while (c = stream().get(), stream())
                        {
                            if (c == '\\')
                            {
                                c = stream().get();
                                if (!stream())
                                    throw core::error("SQL - unexpected end of string");

                                switch (c)
                                {
                                    case '0': buffer.push_back(0); break;
                                    case 'b': buffer.push_back('\b'); break;
                                    case 'n': buffer.push_back('\n'); break;
                                    case 'r': buffer.push_back('\r'); break;
                                    case 't': buffer.push_back('\r'); break;
                                    case 'Z': buffer.push_back(26); break;
                                    case '%':
                                    case '_':
                                        buffer.push_back('\\');
                                        // Fallthrough
                                    default:
                                        buffer.push_back(c); break;
                                }
                            }
                            else if (c == str_type)
                            {
                                if (stream().peek() == str_type)
                                    buffer.push_back(stream().get());
                                else
                                    break; // Done with string
                            }
                            else
                                buffer.push_back(c & 0xff);
                        }

                        return core::value(buffer, str_type == '`'? core::symbol: core::normal);
                    }
                    else if (isalnum(c & 0xff))
                    {
                        std::string buffer;

                        do
                            buffer.push_back(tolower(c & 0xff));
                        while (c = stream().get(), stream() && (isalnum(c & 0xff) ||
                                                                c == '_' ||
                                                                c == '$' ||
                                                                (isdigit(buffer.front() & 0xff) && (c == '+' || c == '-' || c == '.')) ||
                                                                ((c & 0xff) >= 0x80)));

                        if (stream())
                            stream().unget();

                        // Attempt to read as an integer
                        {
                            core::istring_wrapper_stream temp_stream(buffer);
                            core::int_t value;
                            temp_stream >> value;
                            if (!temp_stream.fail() && temp_stream.get() == EOF)
                                return core::value(value);
                        }

                        // Attempt to read as an unsigned integer
                        {
                            core::istring_wrapper_stream temp_stream(buffer);
                            core::uint_t value;
                            temp_stream >> value;
                            if (!temp_stream.fail() && temp_stream.get() == EOF)
                                return core::value(value);
                        }

                        // Attempt to read as a real
                        if (buffer.find_first_of("eE.") != std::string::npos)
                        {
                            core::istring_wrapper_stream temp_stream(buffer);
                            core::real_t value;
                            temp_stream >> value;
                            if (!temp_stream.fail() && temp_stream.get() == EOF)
                                return core::value(value);
                        }

                        // Revert to string, check if it is a function
                        stream() >> c;
                        if (stream() && c == '(')
                        {
                            core::value name_and_params;

                            name_and_params.set_array(core::array_t());
                            name_and_params.push_back(core::value(buffer));

                            stream() >> c;
                            if (stream() && c != ')')
                            {
                                stream().unget();

                                do
                                {
                                    name_and_params.push_back(parse_expression());
                                    stream() >> c;
                                } while (stream() && c == ',');
                            }

                            if (c != ')')
                                throw core::error("SQL - expected ')' closing function call");

                            return core::value(core::object_t{{core::value("func"), name_and_params}});
                        }
                        else if (stream())
                            stream().unget();

                        if (buffer == "null")
                            return core::value();
                        else if (buffer == "binary")
                        {
                            core::value result = parse_value(strictly_values);

                            if (result.is_string())
                                result.set_subtype(core::blob);

                            return result;
                        }
                        return core::value(buffer, core::symbol);
                    }
                    else
                        throw core::custom_error("SQL - expected value but found unexpected '" + ucs_to_utf8(c) + "'");

                    // Never reached
                    return core::value();
                }
            };
        }

        // sql_select_where_filter tests tuples for whether they should be included in the output (equivalent to the SQL 'WHERE' clause)
        //
        // This filter can only test arrays of objects (i.e. tuples must be represented as objects, not arrays)
        // See impl::sql_expression_evaluator for details
        class sql_select_where_filter : public core::buffer_filter
        {
            impl::sql_function_list functions;
            impl::sql_expression_evaluator select;

        public:
            sql_select_where_filter(core::stream_handler &output, const core::value &query)
                : buffer_filter(output, static_cast<buffer_filter_flags>(buffer_strings | buffer_arrays | buffer_objects | buffer_ignore_reported_sizes), 1)
                , functions(impl::make_sql_function_list())
                , select(functions, query)
            {}

            std::string name() const
            {
                return "cppdatalib::core::select_where_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (current_container() != core::array)
                    return;

                if (select(v))
                    buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (current_container() != core::array)
                    return false;

                if (select(v))
                    return buffer_filter::write_(v, is_key);
                return true; // The value was handled, so don't keep processing it
            }
        };

        // sql_select_filter modifies tuples to only contain the appropriate elements (equivalent to the SQL 'SELECT' statement, without extra modifications)
        //
        // This filter can only test arrays of objects (i.e. tuples must be represented as objects, not arrays)
        // See impl::sql_expression_evaluator for details
        class sql_select_filter : public core::buffer_filter
        {
            impl::sql_function_list functions;
            core::value select, where;
            bool selection_order_is_important;

        public:
            sql_select_filter(core::stream_handler &output, core::string_view_t sql_query = core::string_view_t(), bool selection_order_is_important = false)
                : buffer_filter(output, static_cast<buffer_filter_flags>(buffer_strings | buffer_arrays | buffer_objects | buffer_ignore_reported_sizes), 1)
                , functions(impl::make_sql_function_list())
                , selection_order_is_important(selection_order_is_important)
            {
                set_query(sql_query);
            }

            sql_select_filter(core::stream_handler &output, const core::value &select_query, const core::value &where_clause, bool selection_order_is_important)
                : buffer_filter(output, static_cast<buffer_filter_flags>(buffer_strings | buffer_arrays | buffer_objects | buffer_ignore_reported_sizes), 1)
                , functions(impl::make_sql_function_list())
                , select(select_query)
                , where(where_clause)
                , selection_order_is_important(selection_order_is_important)
            {}

            std::string name() const
            {
                return "cppdatalib::core::sql_select_filter(" + output.name() + ")";
            }

            void set_query(core::string_view_t sql_query)
            {
                if (sql_query.empty())
                    select = where = core::value();
                else
                {
                    impl::sql_parser parser(sql_query);
                    parser >> select; // Parse "SELECT" field into select, but other clauses are parsed as well
                    where = parser.get_where(); // This value was parsed via the expression in the line above
                }
            }
            void set_functions(const impl::sql_function_list &funcs) {functions = funcs;}
            const impl::sql_function_list &get_functions() const {return functions;}

            const core::value &get_select() const {return select;}
            const core::value &get_where() const {return where;}

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (parent_container() != core::array)
                {
                    buffer_filter::write_buffered_value_(v, is_key);
                    return;
                }

                core::value copy;
                if (impl::sql_expression_evaluator{functions, where}(v) &&
                    impl::sql_expression_evaluator{functions, select}(v, &copy, selection_order_is_important))
                    buffer_filter::write_buffered_value_(copy, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (current_container() != core::array)
                    return buffer_filter::write_(v, is_key);

                core::value copy;
                if (impl::sql_expression_evaluator{functions, where}(v) &&
                    impl::sql_expression_evaluator{functions, select}(v, &copy, selection_order_is_important))
                    return buffer_filter::write_(copy, is_key);
                return true; // The value was handled, so don't keep processing it
            }
        };

#ifndef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
        template<core::type from, core::type to>
        class converter_filter : public core::buffer_filter
        {
            static_assert(from != to, "cppdatalib::core::converter_filter - filter is being used as a pass-through filter");

            impl::stream_filter_converter<from, to> convert;

        public:
            converter_filter(core::stream_handler &output)
                : buffer_filter(output, static_cast<buffer_filter_flags>((from == core::string? buffer_strings: 0) |
                                                                         (from == core::array? buffer_arrays: 0) |
                                                                         (from == core::object? buffer_objects: 0)))
            {}

            std::string name() const
            {
                return "cppdatalib::core::converter_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    convert(copy);
                    buffer_filter::write_buffered_value_(copy, is_key);
                    return;
                }
                buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == from && from != to)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_(copy, is_key);
                }
                return buffer_filter::write_(v, is_key);
            }
        };
#endif // CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS

        template<core::type from, typename Converter>
        class custom_converter_filter : public core::buffer_filter
        {
            Converter convert;

        public:
            custom_converter_filter(core::stream_handler &output, Converter c = Converter())
                : buffer_filter(output, static_cast<buffer_filter_flags>((from == core::string? buffer_strings: 0) |
                                                                         (from == core::array? buffer_arrays: 0) |
                                                                         (from == core::object? buffer_objects: 0)))
                , convert(c)
            {}

            std::string name() const
            {
                return "cppdatalib::core::custom_converter_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                if (v.get_type() == from)
                {
                    value copy(v);
                    convert(copy);
                    buffer_filter::write_buffered_value_(copy, is_key);
                    return;
                }
                buffer_filter::write_buffered_value_(v, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                if (v.get_type() == from)
                {
                    value copy(v);
                    convert(copy);
                    return buffer_filter::write_(copy, is_key);
                }
                return buffer_filter::write_(v, is_key);
            }
        };

        template<core::type measure, typename Converter>
        custom_converter_filter<measure, Converter> make_custom_converter_filter(core::stream_handler &output, Converter c)
        {
            return custom_converter_filter<measure, Converter>(output, c);
        }

#ifndef CPPDATALIB_DISABLE_IMPLICIT_DATA_CONVERSIONS
        class string_converter_filter : public impl::stream_filter_base
        {
            struct string_converter_filter_impl
            {
                void operator()(core::value &v)
                {
                    v.convert_to_string();
                }
            } convert;

        public:
            string_converter_filter(core::stream_handler &output)
                : stream_filter_base(output)
            {}

            std::string name() const
            {
                return "cppdatalib::core::string_converter_filter(" + output.name() + ")";
            }

        protected:
            bool write_(const value &v, bool is_key)
            {
                if (!v.is_string())
                {
                    value copy(v);
                    convert(copy);
                    return stream_filter_base::write_(copy, is_key);
                }
                return false;
            }
        };
#endif

        // TODO: Currently, conversions from arrays and objects are not supported.
        //       This is because conversion of arrays and objects would require a cache of the entire stream,
        //       and conversion of scalar values would then not operate properly.
        // Conversions to all types are supported.
        //
        // This class supports multiple scalar conversions in one filter.
        template<typename Converter>
        class generic_converter_filter : public buffer_filter
        {
            Converter convert;

        public:
            generic_converter_filter(core::stream_handler &output, Converter c = Converter())
                : buffer_filter(output, buffer_strings)
                , convert(c)
            {}

            std::string name() const
            {
                return "cppdatalib::core::generic_converter_filter(" + output.name() + ")";
            }

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                value copy(v);
                convert(copy);
                buffer_filter::write_buffered_value_(copy, is_key);
            }

            bool write_(const value &v, bool is_key)
            {
                value copy(v);
                convert(copy);
                return buffer_filter::write_(copy, is_key);
            }
        };

        template<typename Converter>
        generic_converter_filter<Converter> make_generic_converter_filter(core::stream_handler &output, Converter c)
        {
            return generic_converter_filter<Converter>(output, c);
        }
    }
}

#endif // CPPDATALIB_STREAM_FILTERS_H
