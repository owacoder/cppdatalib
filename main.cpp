#include <iostream>

#define CPPDATALIB_FAST_IO
#define CPPDATALIB_DISABLE_WRITE_CHECKS
#define CPPDATALIB_DISABLE_FAST_IO_GCOUNT
#include "cppdatalib.h"

using namespace cppdatalib;

template<typename F, typename S = F>
struct TestData : public std::vector<std::pair<F, S>>
{
    using std::vector<std::pair<F, S>>::vector;
};

// `tests` is a vector of test cases. The test cases will be normal (first item in tuple is parameter, second is result)
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename F, typename S, typename ResultCode, typename Compare = std::not_equal_to<S>>
bool Test(const char *name, const TestData<F, S> &tests, ResultCode actual, Compare compare = Compare(), bool bail_early = true)
{
    std::cout << "Testing " << name << "... " << std::flush;

    for (const auto &test: tests)
        if (compare(test.second, actual(test.first)))
        {
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test.first << "\n";
            std::cout << "\tExpected output: " << test.second << "\n";
            std::cout << "\tActual output: " << actual(test.first) << std::endl;
            if (bail_early)
                return false;
        }

    std::cout << "done." << std::endl;
    return true;
}

// `tests` is a vector of test cases. The test cases will be reversed (first item in tuple is result, second is parameter)
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename F, typename S, typename ResultCode, typename Compare = std::not_equal_to<S>>
bool ReverseTest(const char *name, const TestData<F, S> &tests, ResultCode actual, Compare compare = Compare(), bool bail_early = true)
{
    std::cout << "Testing " << name << "... " << std::flush;

    for (const auto &test: tests)
        if (compare(test.first, actual(test.second)))
        {
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test.second << "\n";
            std::cout << "\tExpected output: " << test.first << "\n";
            std::cout << "\tActual output: " << actual(test.second) << std::endl;
            if (bail_early)
                return false;
        }

    std::cout << "done." << std::endl;
    return true;
}

// `tests` is the maximum of a range, 0 -> tests, each element of which is passed to the test code
// ExpectedCode is a functor that should return the expected, valid, test result
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<uintmax_t, typename ExpectedOutput, typename ResultCode, typename Compare>
bool TestRange(const char *name, uintmax_t tests, ExpectedOutput expected, ResultCode actual, Compare compare = Compare(), bool bail_early = true)
{
    std::cout << "Testing " << name << "... " << std::flush;

    for (uintmax_t test = 0; test < tests; ++test)
        if (compare(expected(test), actual(test)))
        {
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test << "\n";
            std::cout << "\tExpected output: " << expected(test) << "\n";
            std::cout << "\tActual output: " << actual(test) << std::endl;
            if (bail_early)
                return false;
        }

    std::cout << "done." << std::endl;
    return true;
}

// `tests` is the maximum of a range, 0 -> tests, each element of which is passed to the test code
// ExpectedCode is a functor that should return the expected, valid, test result
// ResultCode is a functor that should return the actual test result (whether valid or not)
template<typename ExpectedCode, typename ResultCode, typename Compare>
bool TestRangeAndShowProgress(const char *name, uintmax_t tests, ExpectedCode expected, ResultCode actual, Compare compare = Compare(), bool bail_early = true)
{
    std::cout << "Testing " << name << "... " << std::flush;
    size_t percent = 0;

    for (uintmax_t test = 0; test < tests; ++test)
    {
        if (compare(expected(test), actual(test)))
        {
            std::cout << "FAILED!\n";
            std::cout << "\tInput: " << test << "\n";
            std::cout << "\tExpected output: " << expected(test) << "\n";
            std::cout << "\tActual output: " << actual(test) << std::endl;
            if (bail_early)
                return false;
        }

        if (test * 100 / tests > percent)
        {
            percent = test * 100 / tests;
            std::cout << percent << "%\nTesting " << name << "... " << std::flush;
        }
    }

    std::cout << "done." << std::endl;
    return true;
}

TestData<std::string> debug_hex_encode_tests = {
    {"Hello World!", "H e l l o   W o r l d ! "},
    {"A", "A "},
    {"1234", "1 2 3 4 "},
    {"..\1\n", ". . 01 0A "},
    {"", ""}
};

TestData<std::string> hex_encode_tests = {
    {"Hello World!", "48656C6C6F20576F726C6421"},
    {"A", "41"},
    {"1234", "31323334"},
    {"..\1\n", "2E2E010A"},
    {"", ""}
};

TestData<std::string> base64_encode_tests = {
    {"Hello World!", "SGVsbG8gV29ybGQh"},
    {"A", "QQ=="},
    {"1234", "MTIzNA=="},
    {"..\1\n", "Li4BCg=="},
    {"", ""}
};

int main()
{
    Test("base64_encode", base64_encode_tests, [](const auto &test){return base64::encode(test);});
    ReverseTest("base64_decode", base64_encode_tests, [](const auto &test){return base64::decode(test);});
    Test("debug_hex_encode", debug_hex_encode_tests, [](const auto &test){return hex::debug_encode(test);});
    Test("hex_encode", hex_encode_tests, [](const auto &test){return hex::encode(test);});
    TestRangeAndShowProgress("float_ieee754 conversions", UINT32_MAX, core::float_cast_from_ieee_754,
                             core::float_from_ieee_754, [](const auto &f, const auto &s){return f != s && !isnan(f) && !isnan(s);});
    TestRangeAndShowProgress("double_ieee754 conversions", UINT32_MAX, core::double_cast_from_ieee_754,
                             core::double_from_ieee_754, std::not_equal_to<double>());
    return 0;
}
