#ifndef CPPDATALIB_FP_CONVERT_H
#define CPPDATALIB_FP_CONVERT_H

#include <cfloat>
#include <cmath>
#include <cstdint>

namespace cppdatalib
{
    namespace core
    {
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

        inline float float_from_ieee_754_half(uint16_t f)
        {
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
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 14 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (15)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 14 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint16_t float_to_ieee_754_half(float f)
        {
            uint16_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint16_t>(std::signbit(f)) << 15;
            f = std::fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (std::isinf(f))
                return result | (0x1ful << 10);
            else if (std::isnan(f))
                return result | (0x3ful << 9);

            // Then get exponent and significand, base 2
            f = std::frexp(f, &exp);

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
            f *= std::exp2(11 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint16_t>(std::round(f)) & 0x3ff;

            return result;
        }

        inline float float_from_ieee_754(uint32_t f)
        {
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
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int32_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 126 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (127)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<float>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 126 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint32_t float_to_ieee_754(float f)
        {
            uint32_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint32_t>(std::signbit(f)) << 31;
            f = std::fabs(f);

            // Handle special cases
            if (f == 0)
                return result;
            else if (std::isinf(f))
                return result | (0xfful << 23);
            else if (std::isnan(f))
                return result | (0x1fful << 22);

            // Then get exponent and significand, base 2
            f = std::frexp(f, &exp);

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
            f *= std::exp2(24 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint32_t>(std::round(f)) & 0x7fffff;

            return result;
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

        inline double double_from_ieee_754(uint64_t f)
        {
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
            if (exp == 0 && mantissa == 0) // 0, -0
                result = 0;
            else if (exp == exponent_mask) // +/- Infinity, NaN
                result = mantissa == 0? INFINITY: NAN;
            else
            {
                const int64_t normal = exp != 0;

                // `mantissa | (normal << exponent_offset)` evaluates to 1.mantissa if normalized number, and 0.mantissa if denormalized
                // `exp - exponent_offset - 1022 - normal` is the adjusted exponent, removing the mantissa bias and the exponent bias (1023)
                // (The mantissa bias is determined by exponent_offset and normal, equal to the binary length of the mantissa, plus the implicit leading 1
                //  for a normalized number. Denormalized numbers only have a leading 0.)

                result = std::ldexp(static_cast<double>(mantissa | (normal << exponent_offset)),
                                    exp - exponent_offset - 1022 /* sic: the bias is adjusted for the following subtraction */ - normal);
            }

            return f >> sign_offset? -result: result;
        }

        inline uint64_t double_to_ieee_754(double d)
        {
            uint64_t result = 0;
            int exp;

            // Enter sign in result
            result |= static_cast<uint64_t>(std::signbit(d)) << 63;
            d = std::fabs(d);

            // Handle special cases
            if (d == 0)
                return result;
            else if (std::isinf(d))
                return result | (0x7ffull << 52);
            else if (std::isnan(d))
                return result | (0xfffull << 51);

            // Then get exponent and significand, base 2
            d = std::frexp(d, &exp);

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
            d *= std::exp2(53 + exp); // exp will actually be negative here, so this is effectively a subtraction
            result |= static_cast<uint64_t>(std::round(d)) & ((1ull << 52) - 1);

            return result;
        }
    }
}

#endif // CPPDATALIB_FP_CONVERT_H
