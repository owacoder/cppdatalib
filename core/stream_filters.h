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
#include <algorithm> // For sorting and specialty filters

#include <cassert>

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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                default: value.set_null(); break;
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
                                        arr.push_back(it.first);
                                        arr.push_back(it.second);
                                    }

                                    value.set_array(arr);
                                    break;
                                }
                                default: value.set_null(); break;
                            }
                            break;
                        default: break;
                    }
                }
            };
        }

        enum buffer_filter_flags
        {
            buffer_strings = 0x01,
            buffer_arrays = 0x02,
            buffer_objects = 0x04
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

                return stream_filter_base::required_features() & ~mask_out;
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
                    if (size != unknown_size && size_t(size) == v.size())
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
                    output.begin_array(v, size);
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
                    if (size != unknown_size && size_t(size) == v.size())
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
                    output.begin_object(v, size);
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
                    if (size != unknown_size && size_t(size) == v.size())
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
                    output.begin_string(v, size);
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
                (void) is_key;
                if (v.get_type() == measure)
                {
                    core::real_t value = v.as_real();

                    ++samples;

                    sum += value;
                    product *= value;
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

        protected:
            void begin_()
            {
                base::begin_();
                samples.set_null();
                calculated = false;
            }

            bool write_(const value &v, bool is_key)
            {
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
                    sort(samples.get_array_ref().begin(), samples.get_array_ref().end());
                    sorted = true;
                }

                if (samples.array_size() == 0) // No samples
                    return core::null_t();
                else if (samples.array_size() & 1) // Odd number of samples
                    return samples.element(samples.array_size() / 2);
                else // Even number of samples
                    return (samples.element(samples.array_size() / 2 - 1).as_real() +
                            samples.element(samples.array_size() / 2).as_real()) / 2;
            }

            size_t sample_size() const
            {
                return samples.size();
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

        protected:
            void begin_()
            {
                stream_filter_base::begin_();
                samples.set_null();
                calculated = false;
            }

            bool write_(const value &v, bool is_key)
            {
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

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                using namespace std;

                if (v.get_type() == core::array)
                {
                    core::value sorted;

                    sorted = v;
                    if (direction == ascending_sort)
                        sort(sorted.get_array_ref().begin(), sorted.get_array_ref().end());
                    else
                        sort(sorted.get_array_ref().rbegin(), sorted.get_array_ref().rend());

                    buffer_filter::write_buffered_value_(sorted, is_key);
                }
                else
                    buffer_filter::write_buffered_value_(v, is_key);
            }
        };

        class table_to_array_of_maps_filter : public core::buffer_filter
        {
            core::value column_names;
            bool fail_on_missing_column;

        public:
            table_to_array_of_maps_filter(core::stream_handler &output, const core::value &column_names, bool fail_on_missing_column = false)
                : core::buffer_filter(output, buffer_arrays, 1)
                , column_names(column_names)
                , fail_on_missing_column(fail_on_missing_column)
            {}

        protected:
            void write_buffered_value_(const value &v, bool is_key)
            {
                core::value map_;

                if ((v.is_array() && v.array_size() > column_names.array_size())
                        || column_names.array_size() == 0) // Not enough column names to cover all attributes!!
                    throw core::error("cppdatalib::core::table_to_array_of_maps_filter - not enough column names provided for specified data");

                if (v.is_array())
                    for (size_t i = 0; i < column_names.array_size(); ++i)
                    {
                        if (i >= v.size() && fail_on_missing_column)
                            throw core::error("cppdatalib::core::table_to_array_of_maps_filter - missing column entry in table row");
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
    }
}

#endif // CPPDATALIB_STREAM_FILTERS_H
