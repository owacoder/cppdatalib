/*
 * test_framework.h
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

#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <vector>

template<typename Stream>
class vt100
{
    const char * const const_reset_term = "\033c";
    const char * const const_enable_lwrap = "\033[7h";
    const char * const const_disable_lwrap = "\033[7l";

    const char * const const_default_font = "\033(";
    const char * const const_alternate_font = "\033)";

    const char * const const_move_cursor_screen_home = "\033[H"; // Moves to upper-left of screen, not beginning of line
    const char * const const_move_cursor_home = "\r"; // Moves to beginning of line, not beginning of screen
    std::string internal_move_cursor(int row, int col)
    {
        return "\033[" + std::to_string(row) + ';' + std::to_string(col) + 'H';
    }
    const char * const const_force_move_cursor_home = "\033[f";
    std::string internal_force_move_cursor(int row, int col)
    {
        return "\033[" + std::to_string(row) + ';' + std::to_string(col) + 'f';
    }
    const char * const const_move_cursor_up = "\033[A";
    std::string internal_move_cursor_up_by(int rows)
    {
        return "\033[" + std::to_string(rows) + 'A';
    }
    const char * const const_move_cursor_down = "\033[B";
    std::string internal_move_cursor_down_by(int rows)
    {
        return "\033[" + std::to_string(rows) + 'B';
    }
    const char * const const_move_cursor_right = "\033[C";
    std::string internal_move_cursor_right_by(int cols)
    {
        return "\033[" + std::to_string(cols) + 'C';
    }
    const char * const const_move_cursor_left = "\033[D";
    std::string internal_move_cursor_left_by(int cols)
    {
        return "\033[" + std::to_string(cols) + 'D';
    }
    const char * const const_save_cursor = "\033[s";
    const char * const const_restore_cursor = "\033[u";
    const char * const const_save_cursor_and_attrs = "\0337";
    const char * const const_restore_cursor_and_attrs = "\0338";

    const char * const const_enable_scroll = "\033[r";
    const char * const const_scroll_screen_down = "\033D";
    const char * const const_scroll_screen_up = "\033M";
    std::string internal_scroll_range(int from, int to)
    {
        return "\033[" + std::to_string(from) + ';' + std::to_string(to) + 'r';
    }

    const char * const const_set_tab = "\033H";
    const char * const const_unset_tab = "\033[g";
    const char * const const_unset_all_tabs = "\033[3g";

    const char * const const_erase_to_end_of_line = "\033[K";
    const char * const const_erase_to_start_of_line = "\033[1K";
    const char * const const_erase_line = "\033[2K";
    const char * const const_erase_screen_down = "\033[J";
    const char * const const_erase_screen_up = "\033[1J";
    const char * const const_erase_screen = "\033[2J";

    const char * const const_attr_reset = "\033[0m";
    const char * const const_attr_bright = "\033[1m";
    const char * const const_attr_dim = "\033[2m";
    const char * const const_attr_underscore = "\033[4m";
    const char * const const_attr_blink = "\033[5m";
    const char * const const_attr_reverse = "\033[7m";
    const char * const const_attr_hidden = "\033[8m";

    const char * const const_black = "\033[30m";
    const char * const const_red = "\033[31m";
    const char * const const_green = "\033[32m";
    const char * const const_yellow = "\033[33m";
    const char * const const_blue = "\033[34m";
    const char * const const_magenta = "\033[35m";
    const char * const const_cyan = "\033[36m";
    const char * const const_white = "\033[37m";

    const char * const const_bg_black = "\033[40m";
    const char * const const_bg_red = "\033[41m";
    const char * const const_bg_green = "\033[42m";
    const char * const const_bg_yellow = "\033[43m";
    const char * const const_bg_blue = "\033[44m";
    const char * const const_bg_magenta = "\033[45m";
    const char * const const_bg_cyan = "\033[46m";
    const char * const const_bg_white = "\033[47m";

    Stream &stream;

public:
    enum cursor_movement
    {
        screen_top_left,
        beginning_of_line,
        force_beginning_of_line,
        up,
        down,
        right,
        left
    };

    enum color
    {
        black,
        red,
        green,
        yellow,
        blue,
        magenta,
        cyan,
        white
    };

    enum erase_direction
    {
        erase_all_right,
        erase_all_left,
        erase_current_line,
        erase_all_down,
        erase_all_up,
        erase_all
    };

    enum attribute_style
    {
        attr_bright,
        attr_dim,
        attr_underscored,
        attr_blinking,
        attr_colors_reversed,
        attr_hidden
    };

    vt100(Stream &stream) : stream(stream) {}

    vt100 &reset() {stream << const_reset_term; return *this;}
    vt100 &lwrap(bool enable = true) {stream << (enable? const_enable_lwrap: const_disable_lwrap); return *this;}
    vt100 &font(bool default_font = true) {stream << (default_font? const_default_font: const_alternate_font); return *this;}
    vt100 &move(cursor_movement direction)
    {
        const char *list[] = {const_move_cursor_screen_home,
                              const_move_cursor_home,
                              const_force_move_cursor_home,
                              const_move_cursor_up,
                              const_move_cursor_down,
                              const_move_cursor_right,
                              const_move_cursor_left};

        if (direction >= 0 && direction < sizeof(list)/sizeof(*list))
            stream << list[direction];
        return *this;
    }
    vt100 &move_to(int row, int col) {stream << internal_move_cursor(row, col); return *this;}
    vt100 &force_move_to(int row, int col) {stream << internal_force_move_cursor(row, col); return *this;}
    vt100 &move_by(int amount, cursor_movement direction)
    {
        switch (direction)
        {
            case up: stream << internal_move_cursor_up_by(amount); break;
            case down: stream << internal_move_cursor_down_by(amount); break;
            case right: stream << internal_move_cursor_right_by(amount); break;
            case left: stream << internal_move_cursor_left_by(amount); break;
            default: return move(direction);
        }

        return *this;
    }

    // TODO: save and restore
    // TODO: scrolling
    // TODO: tabs

    vt100 &erase(erase_direction direction)
    {
        const char *list[] = {const_erase_to_end_of_line,
                              const_erase_to_start_of_line,
                              const_erase_line,
                              const_erase_screen_down,
                              const_erase_screen_up,
                              const_erase_screen};

        if (direction >= 0 && direction < sizeof(list)/sizeof(*list))
            stream << list[direction];
        return *this;
    }

    vt100 &fcolor(color c)
    {
        const char *list[] = {const_black,
                              const_red,
                              const_green,
                              const_yellow,
                              const_blue,
                              const_magenta,
                              const_cyan,
                              const_white};

        if (c >= 0 && c < sizeof(list)/sizeof(*list))
            stream << list[c];

        return *this;
    }
    vt100 &fg(color c) {return fcolor(c);}

    vt100 &bcolor(color c)
    {
        const char *list[] = {const_bg_black,
                              const_bg_red,
                              const_bg_green,
                              const_bg_yellow,
                              const_bg_blue,
                              const_bg_magenta,
                              const_bg_cyan,
                              const_bg_white};

        if (c >= 0 && c < sizeof(list)/sizeof(*list))
            stream << list[c];

        return *this;
    }
    vt100 &bg(color c) {return bcolor(c);}

    vt100 &reset_attributes() {stream << const_attr_reset; return *this;}
    vt100 &enable_attribute(attribute_style style)
    {
        const char *list[] = {const_attr_bright,
                              const_attr_dim,
                              const_attr_underscore,
                              const_attr_blink,
                              const_attr_reverse,
                              const_attr_hidden};

        if (style >= 0 && style < sizeof(list)/sizeof(*list))
            stream << list[style];

        return *this;
    }
};

template<typename Stream>
vt100<Stream> make_vt100(Stream &stream) {return vt100<Stream>(stream);}

template<typename T>
std::ostream &operator<<(std::ostream &strm, const std::vector<T> &value)
{
    strm << "[";
    for (auto i = value.begin(); i != value.end(); ++i)
    {
        if (i != value.begin())
            strm << " ";
        strm << *i;
    }
    return strm << "]" << std::flush;
}

template<typename F, typename S = F>
struct TestData : public std::vector<std::pair<F, S>>
{
    using std::vector<std::pair<F, S>>::vector;
};

template<typename Stream, typename Tests, typename FunctorToTest, typename FunctorResult, typename Compare = std::not_equal_to<FunctorResult>>
class Testbed
{
    Stream &stream;
    const char *name;
    const Tests &tests;
    FunctorToTest actual;
    uintmax_t max_errors_before_bail;
    Compare compare;

    bool reverse_items;

public:
    // `tests` is a vector of test cases. The test cases will be normal (first item in tuple is parameter, second is result)
    // FunctorToTest is a functor that should return the actual test result (whether valid or not)
    Testbed(Stream &stream, const char *name, const Tests &tests, FunctorToTest actual, uintmax_t max_errors_before_bail = UINTMAX_MAX, Compare compare = Compare())
        : stream(stream)
        , name(name)
        , tests(tests)
        , actual(actual)
        , max_errors_before_bail(max_errors_before_bail)
        , compare(compare)
        , reverse_items(false)
    {}

    void reverse(bool b = true) {reverse_items = b;}

    bool operator()()
    {
        vt100<Stream> vt(stream);
        uintmax_t percent = 0;
        uintmax_t current = 0;
        uintmax_t failed = 0;

        stream << "Testing " << name << "... ";
        vt.fg(vt.yellow);
        stream << "0%" << std::flush;
        vt.reset_attributes();

        for (const auto &test: tests)
        {
            if (!reverse_items)
            {
                FunctorResult actual_result;

                std::string err;
                bool fail;

                try {
                    actual_result = actual(test.first);
                    fail = compare(test.second, actual_result);
                } catch (std::exception &e) {
                    err = e.what();
                    fail = true;
                } catch (...) {
                    err = "non-standard exception";
                    fail = true;
                }

                if (fail)
                {
                    vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                    stream << "Testing " << name << "... " << std::flush;
                    vt.fg(vt.red);
                    stream << "Test " << (current+1) << " FAILED!\n";
                    stream << "\tInput: " << test.first << "\n";
                    stream << "\tExpected output: " << test.second << "\n";
                    if (err.empty())
                        stream << "\tActual output: " << actual_result << std::endl;
                    else
                        stream << "\tActual output: Exception: " << err << std::endl;
                    vt.reset_attributes();
                    if (failed == max_errors_before_bail)
                        return false;
                    ++failed;
                }
            }
            else
            {
                FunctorResult actual_result;

                std::string err;
                bool fail;

                try {
                    actual_result = actual(test.second);
                    fail = compare(test.first, actual_result);
                } catch (std::exception &e) {
                    err = e.what();
                    fail = true;
                }
                catch (...) {
                    err = "non-standard exception";
                    fail = true;
                }

                if (fail)
                {
                    vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                    stream << "Testing " << name << "... " << std::flush;
                    vt.fg(vt.red);
                    stream << "Test " << (current+1) << " FAILED!\n";
                    stream << "\tInput: " << test.second << "\n";
                    stream << "\tExpected output: " << test.first << "\n";
                    if (err.empty())
                        stream << "\tActual output: " << actual_result << std::endl;
                    else
                        stream << "\tActual output: Exception: " << err << std::endl;
                    vt.reset_attributes();
                    if (failed == max_errors_before_bail)
                        return false;
                    ++failed;
                }
            }

            if (current * 100 / tests.size() > percent)
            {
                percent = current * 100 / tests.size();
                vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                stream << "Testing " << name << "... ";
                vt.fg(vt.yellow);
                stream << percent << "%" << std::flush;
                vt.reset_attributes();
            }
            ++current;
        }

        if (!failed)
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.green);
            stream << "DONE" << std::endl;
            vt.reset_attributes();
        }
        else
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.red);
            stream << "DONE (" << failed << " failed out of " << tests.size() << ")" << std::endl;
            vt.reset_attributes();
        }

        return failed;
    }
};

template<typename Stream, typename FunctorToRangeTest, typename FunctorToRangeTestExpected, typename FunctorResult, typename Compare = std::not_equal_to<FunctorResult>>
class RangeTestbed
{
    Stream &stream;
    const char *name;
    uintmax_t count;
    FunctorToRangeTest actual;
    FunctorToRangeTestExpected expected;
    uintmax_t max_errors_before_bail;
    Compare compare;

public:
    // `tests` is a vector of test cases. The test cases will be normal (first item in tuple is parameter, second is result)
    // FunctorToTest is a functor that should return the actual test result (whether valid or not)
    RangeTestbed(Stream &stream, const char *name, uintmax_t count, FunctorToRangeTest actual, FunctorToRangeTestExpected expected, uintmax_t max_errors_before_bail = UINTMAX_MAX, Compare compare = Compare())
        : stream(stream)
        , name(name)
        , count(count)
        , actual(actual)
        , expected(expected)
        , max_errors_before_bail(max_errors_before_bail)
        , compare(compare)
    {}

    bool operator()()
    {
        vt100<Stream> vt(stream);
        uintmax_t percent = 0;
        uintmax_t current = 0;
        uintmax_t failed = 0;

        stream << "Testing " << name << "... ";
        vt.fg(vt.yellow);
        stream << "0%" << std::flush;
        vt.reset_attributes();

        while (current < count)
        {
            FunctorResult expected_result;
            FunctorResult actual_result;

            std::string err;
            bool fail;

            try {
                expected_result = expected(current);
                actual_result = actual(current);

                fail = compare(expected_result, actual_result);
            } catch (std::exception &e) {
                err = e.what();
                fail = true;
            } catch (...) {
                err = "non-standard exception";
                fail = true;
            }

            if (fail)
            {
                vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                stream << "Testing " << name << "... " << std::flush;
                vt.fg(vt.red);
                stream << "Test " << (current+1) << " FAILED!\n";
                stream << "\tInput: " << current << "\n";
                stream << "\tExpected output: " << expected_result << "\n";
                if (err.empty())
                    stream << "\tActual output: " << actual_result << std::endl;
                else
                    stream << "\tActual output: Exception: " << err << std::endl;
                vt.reset_attributes();
                if (failed == max_errors_before_bail)
                    return false;
                ++failed;
            }

            if (current * 100 / count > percent)
            {
                percent = current * 100 / count;
                vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                stream << "Testing " << name << "... ";
                vt.fg(vt.yellow);
                stream << percent << "%" << std::flush;
                vt.reset_attributes();
            }
            ++current;
        }

        if (!failed)
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.green);
            stream << "DONE" << std::endl;
            vt.reset_attributes();
        }
        else
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.red);
            stream << "DONE (" << failed << " failed out of " << count << ")" << std::endl;
            vt.reset_attributes();
        }

        return failed;
    }
};

template<typename Stream, typename FunctorToRangeTest, typename FunctorToRangeTestExpected, typename FunctorResult, typename Compare = std::not_equal_to<FunctorResult>>
class DirectoryTestbed
{
    Stream &stream;
    const char *name;
    uintmax_t count;
    FunctorToRangeTest actual;
    FunctorToRangeTestExpected expected;
    uintmax_t max_errors_before_bail;
    Compare compare;

public:
    // `tests` is a vector of test cases. The test cases will be normal (first item in tuple is parameter, second is result)
    // FunctorToTest is a functor that should return the actual test result (whether valid or not)
    DirectoryTestbed(Stream &stream, const char *name, uintmax_t count, FunctorToRangeTest actual, FunctorToRangeTestExpected expected, uintmax_t max_errors_before_bail = UINTMAX_MAX, Compare compare = Compare())
        : stream(stream)
        , name(name)
        , count(count)
        , actual(actual)
        , expected(expected)
        , max_errors_before_bail(max_errors_before_bail)
        , compare(compare)
    {}

    bool operator()()
    {
        vt100<Stream> vt(stream);
        uintmax_t percent = 0;
        uintmax_t current = 0;
        uintmax_t failed = 0;

        stream << "Testing " << name << "... ";
        vt.fg(vt.yellow);
        stream << "0%" << std::flush;
        vt.reset_attributes();

        while (current < count)
        {
            FunctorResult expected_result;
            FunctorResult actual_result;

            std::string err;
            bool fail;

            try {
                expected_result = expected(current);
                actual_result = actual(current);

                fail = compare(expected_result, actual_result);
            } catch (std::exception &e) {
                err = e.what();
                fail = true;
            } catch (...) {
                err = "non-standard exception";
                fail = true;
            }

            if (fail)
            {
                vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                stream << "Testing " << name << "... " << std::flush;
                vt.fg(vt.red);
                stream << "Test " << (current+1) << " FAILED!\n";
                stream << "\tInput: " << current << "\n";
                stream << "\tExpected output: " << expected_result << "\n";
                if (err.empty())
                    stream << "\tActual output: " << actual_result << std::endl;
                else
                    stream << "\tActual output: Exception: " << err << std::endl;
                vt.reset_attributes();
                if (failed == max_errors_before_bail)
                    return false;
                ++failed;
            }

            if (current * 100 / count > percent)
            {
                percent = current * 100 / count;
                vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
                stream << "Testing " << name << "... ";
                vt.fg(vt.yellow);
                stream << percent << "%" << std::flush;
                vt.reset_attributes();
            }
            ++current;
        }

        if (!failed)
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.green);
            stream << "DONE" << std::endl;
            vt.reset_attributes();
        }
        else
        {
            vt.erase(vt.erase_current_line).move(vt.beginning_of_line).reset_attributes();
            stream << "Testing " << name << "... ";
            vt.fg(vt.red);
            stream << "DONE (" << failed << " failed out of " << count << ")" << std::endl;
            vt.reset_attributes();
        }

        return failed;
    }
};

template<typename FunctorResult, typename Stream, typename Tests, typename FunctorToTest, typename Compare = std::not_equal_to<FunctorResult>>
Testbed<Stream, Tests, FunctorToTest, FunctorResult, Compare> make_testbed(const char *name, Stream &stream, const Tests &tests, FunctorToTest functor, uintmax_t max_errors_before_bail = UINTMAX_MAX, Compare compare = std::not_equal_to<FunctorResult>())
{
    return {stream, name, tests, functor, max_errors_before_bail, compare};
}

template<typename FunctorResult, typename Stream, typename FunctorToRangeTest, typename FunctorToRangeTestExpected, typename Compare = std::not_equal_to<FunctorResult>>
RangeTestbed<Stream, FunctorToRangeTest, FunctorToRangeTestExpected, FunctorResult, Compare> make_range_testbed(const char *name, Stream &stream, uintmax_t count, FunctorToRangeTest actual, FunctorToRangeTestExpected expected, uintmax_t max_errors_before_bail = UINTMAX_MAX, Compare compare = std::not_equal_to<FunctorResult>())
{
    return {stream, name, count, actual, expected, max_errors_before_bail, compare};
}

#endif // TEST_FRAMEWORK_H
