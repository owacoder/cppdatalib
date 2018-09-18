/*
 * fp_convert.h
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

#ifndef CPPDATALIB_FP_CONVERT_H
#define CPPDATALIB_FP_CONVERT_H

#include <cfloat>
#include <cmath>
#include <math.h>
#include <cstdint>
#include <limits>
#include "global.h"

namespace cppdatalib
{
    namespace core
    {
        inline float truncf(float f) {return f < 0.0? ceil(f): floor(f);}
        inline double trunc(double d) {return d < 0.0? ceil(d): floor(d);}

        inline uint32_t float_cast_to_ieee_754(float f)
        {
            union
            {
                uint32_t output;
                float input;
            } result;

            result.input = f;
            return result.output;
        }

        inline float float_cast_from_ieee_754(uint32_t f)
        {
            union
            {
                uint32_t input;
                float output;
            } result;

            result.input = f;
            return result.output;
        }

        inline uint64_t double_cast_to_ieee_754(double d)
        {
            union
            {
                uint64_t output;
                double input;
            } result;

            result.input = d;
            return result.output;
        }

        inline double double_cast_from_ieee_754(uint64_t d)
        {
            union
            {
                uint64_t input;
                double output;
            } result;

            result.input = d;
            return result.output;
        }

        // If `**after == 0` on returning from this function, success!
        // If `*after == nullptr` on returning from this function, failure.
        template<typename Float>
        inline Float fp_from_string(const char *begin, char **after)
        {
            Float value = 0.0;

            using namespace std;
            bool negative = false;

            // Skip leading whitespace
            for (; *begin == ' ' || *begin == '\t'; ++begin)
                ;

            if (!*begin)
                goto cleanup;

            // Parse sign
            if (*begin == '+' || *begin == '-')
            {
                negative = *begin == '-';
                ++begin;
            }

            if (!*begin)
                goto cleanup;

            // Parse initial integral value
            for (; isdigit(*begin); ++begin)
                value = Float(value * 10.0) + Float(*begin - '0');

            // Parse fractional value
            if (*begin == '.')
            {
                Float fraction = 0.0;
                unsigned long places = 0;
                ++begin; // Skip '.'

                if (!*begin)
                    goto cleanup;

                for (; isdigit(*begin); ++begin)
                {
                    fraction = Float(fraction * 10.0) + Float(*begin - '0');
                    ++places;
                }

                value += fraction / pow(Float(10.0), Float(places));
            }

            // Parse exponent value
            if (*begin == 'e' || *begin == 'E')
            {
                Float exponent = 0.0;
                bool negative_exp = false;

                ++begin; // Skip 'E'
                if (*begin == '+' || *begin == '-')
                {
                    negative_exp = *begin == '-';
                    ++begin; // Skip sign
                }

                if (!*begin)
                    goto cleanup;

                for (; isdigit(*begin); ++begin)
                    exponent = Float(exponent * 10.0) + Float(*begin - '0');

                if (negative_exp)
                    value /= pow(Float(10.0), exponent);
                else
                    value *= pow(Float(10.0), exponent);
            }

            if (after != nullptr)
                *after = const_cast<char *>(begin);

            if (isinf(value) || isnan(value))
            {
                errno = ERANGE;
                return Float(negative? -HUGE_VAL: HUGE_VAL);
            }

            return negative? -value: value;

        cleanup:
            if (after)
                *after = nullptr;
            return 0.0;
        }

        // If `*after == end` on returning from this function, success!
        // If `*after == nullptr` on returning from this function, failure.
        template<typename Float>
        inline Float fp_from_in_string(const char *begin, const char *end, char **after)
        {
            Float value = 0.0;

            using namespace std;
            bool negative = false;

            // Skip leading whitespace
            for (; begin != end && (*begin == ' ' || *begin == '\t'); ++begin)
                ;

            if (begin == end)
                goto cleanup;

            // Parse sign
            if (*begin == '+' || *begin == '-')
            {
                negative = *begin == '-';
                ++begin;
            }

            if (begin == end)
                goto cleanup;

            // Parse initial integral value
            for (; begin != end && isdigit(*begin); ++begin)
                value = Float(value * 10.0) + Float(*begin - '0');

            // Parse fractional value
            if (begin != end && *begin == '.')
            {
                Float fraction = 0.0;
                Float places = 1.0;
                ++begin; // Skip '.'

                if (begin == end)
                    goto cleanup;

                for (; begin != end && isdigit(*begin); ++begin)
                {
                    fraction = Float(fraction * 10.0) + Float(*begin - '0');
                    places *= 10.0;
                }

                value += fraction / places;
            }

            // Parse exponent value
            if (begin != end && (*begin == 'e' || *begin == 'E'))
            {
                Float exponent = 0.0;
                bool negative_exp = false;

                ++begin; // Skip 'E'
                if (begin != end && (*begin == '+' || *begin == '-'))
                {
                    negative_exp = *begin == '-';
                    ++begin; // Skip sign
                }

                if (begin == end)
                    goto cleanup;

                for (; begin != end && isdigit(*begin); ++begin)
                    exponent = Float(exponent * 10.0) + Float(*begin - '0');

                if (negative_exp)
                    value /= pow(Float(10.0), exponent);
                else
                    value *= pow(Float(10.0), exponent);
            }

            if (after != nullptr)
                *after = const_cast<char *>(begin);

            if (isinf(value) || isnan(value))
            {
                errno = ERANGE;
                return Float(negative? -HUGE_VAL: HUGE_VAL);
            }

            return negative? -value: value;

        cleanup:
            if (after)
                *after = nullptr;
            return 0.0;
        }

        inline float float_from_ieee_754_half(uint16_t f)
        {
            using namespace std;

            const int32_t mantissa_mask = 0x3ff;
            const int32_t exponent_offset = 10;
            const int32_t exponent_mask = 0x1f;
            const int32_t sign_offset = 15;

            float result;
            int32_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == exponent_mask) // +/- Infinity, NaN
            {
                if (mantissa == 0) // +/- Infinity
                    result = CPPDATALIB_FP_INFINITY(float);
                else // qNaN, sNaN
                    result = (f >> (exponent_offset - 1)) & 1?
                        CPPDATALIB_FP_QNAN(float):
                        CPPDATALIB_FP_SNAN(float);
            }
            else if (exp == 0 && mantissa == 0) // 0, -0
                result = 0.0;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 14 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (15)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 14 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint16_t float_to_ieee_754_half(float f)
        {
            using namespace std;

            uint16_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint16_t>(signbit(f)) << 15;
            f = fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (isinf(f))
                return result | (0x1ful << 10);
            else if (isnan(f))
                return result | (0x3ful << 9);

            // Then get exponent and significand, base 2
            f = frexp(f, &exp);

            // Enter exponent in result
            if (exp > -14) // Normalized number
            {
                if (exp + 14 >= 0x1f)
                    return result | (0x1fu << 10);

                result |= static_cast<uint16_t>(exp + 14) << 10;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 13;

            // Bias significand so we can extract it as an integer
#ifdef CPPDATALIB_CPP11
            f *= exp2(static_cast<float>(11 + exp)); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint16_t>(round(f)) & 0x3ff;
#else // not C++11
            f *= pow(2.0f, static_cast<float>(11 + exp)); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint16_t>(truncf(f + 0.5)) & 0x3ff;
#endif

            return result;
        }

#ifdef CPPDATALIB_X86
        inline float float_from_ieee_754(uint32_t f) {return float_cast_from_ieee_754(f);}
        inline uint32_t float_to_ieee_754(float f) {return float_cast_to_ieee_754(f);}
        inline double double_from_ieee_754(uint64_t d) {return double_cast_from_ieee_754(d);}
        inline uint64_t double_to_ieee_754(double d) {return double_cast_to_ieee_754(d);}
#else
        inline float float_from_ieee_754(uint32_t f)
        {
            using namespace std;

            const int32_t mantissa_mask = 0x7fffff;
            const int32_t exponent_offset = 23;
            const int32_t exponent_mask = 0xff;
            const int32_t sign_offset = 31;

            float result;
            int32_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == exponent_mask) // +/- Infinity, NaN
            {
                if (mantissa == 0) // +/- Infinity
                    result = CPPDATALIB_FP_INFINITY(float);
                else // qNaN, sNaN
                    result = (f >> (exponent_offset - 1)) & 1?
                        CPPDATALIB_FP_QNAN(float):
                        CPPDATALIB_FP_SNAN(float);
            }
            else if (exp == 0 && mantissa == 0) // 0, -0
                result = 0.0;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 126 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (127)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 126 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint32_t float_to_ieee_754(float f)
        {
            using namespace std;

            uint32_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint32_t>(signbit(f)) << 31;
            f = fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (isinf(f))
                return result | (0xfful << 23);
            else if (isnan(f))
                return result | (0x1fful << 22);

            // Then get exponent and significand, base 2
            f = frexp(f, &exp);

            // Enter exponent in result
            if (exp > -126) // Normalized number
            {
                if (exp + 126 >= 0xff)
                    return result | (0xfful << 23);

                result |= static_cast<uint32_t>((exp + 126) & 0xff) << 23;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 125;

            // Bias significand so we can extract it as an integer
#ifdef CPPDATALIB_CPP11
            f *= exp2(static_cast<float>(24 + exp)); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint32_t>(round(f)) & 0x7fffff;
#else // not C++11
            f *= pow(2.0f, static_cast<float>(24 + exp)); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint32_t>(truncf(f + 0.5)) & 0x7fffff;
#endif

            return result;
        }

        inline double double_from_ieee_754(uint64_t f)
        {
            using namespace std;

            const int64_t mantissa_mask = 0xfffffffffffff;
            const int64_t exponent_offset = 52;
            const int64_t exponent_mask = 0x7ff;
            const int64_t sign_offset = 63;

            double result;
            int64_t exp, mantissa;

            // Extract exponent and mantissa
            exp = (f >> exponent_offset) & exponent_mask;
            mantissa = f & mantissa_mask;

            // Handle special cases
            if (exp == exponent_mask) // +/- Infinity, NaN
            {
                if (mantissa == 0) // +/- Infinity
                    result = CPPDATALIB_FP_INFINITY(double);
                else // qNaN, sNaN
                    result = (f >> (exponent_offset - 1)) & 1?
                        CPPDATALIB_FP_QNAN(double):
                        CPPDATALIB_FP_SNAN(double);
            }
            else if (exp == 0 && mantissa == 0)
                result = 0.0;
            else
            {
                const int64_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 1022 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (1023)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = ldexp(static_cast<double>(mantissa | (normal << exponent_offset)),
                                    static_cast<int>(exp - exponent_offset - 1022 /* sic: the bias is adjusted for the following subtraction */ - normal));
            }

            return f >> sign_offset? -result: result;
        }

        inline uint64_t double_to_ieee_754(double d)
        {
            using namespace std;

            uint64_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint64_t>(signbit(d)) << 63;
            d = fabs(d);

            // Handle special cases
            if (d == 0)
                return result;
            else if (isinf(d))
                return result | (0x7ffull << 52);
            else if (isnan(d))
                return result | (0xfffull << 51);

            // Then get exponent and significand, base 2
            d = frexp(d, &exp);

            // Enter exponent in result
            if (exp > -1022) // Normalized number
            {
                if (exp + 1022 >= 0x7ff)
                    return result | (0x7ffull << 52);

                result |= static_cast<uint64_t>((exp + 1022) & 0x7ff) << 52;
                exp = 0;
            }
            else // Denormalized number, adjust shift count
                exp += 1021;

            // Bias significand so we can extract it as an integer
#ifdef CPPDATALIB_CPP11
            d *= exp2(53 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint64_t>(round(d)) & ((1ull << 52) - 1);
#else // not C++11
            d *= pow(2.0, static_cast<double>(53 + exp)); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint64_t>(trunc(d + 0.5)) & ((1ull << 52) - 1);
#endif

            return result;
        }
#endif // CPPDATALIB_X86
    }
}

#endif // CPPDATALIB_FP_CONVERT_H
