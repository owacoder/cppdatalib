#ifndef CPPDATALIB_BASE64_H
#define CPPDATALIB_BASE64_H

#include <iostream>
#include <sstream>

namespace cppdatalib
{
    namespace base64
    {
        inline std::ostream &write(std::ostream &stream, const std::string &str)
        {
            const char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            uint32_t temp;

            for (size_t i = 0; i < str.size();)
            {
                temp = (uint32_t) (str[i++] & 0xFF) << 16;
                if (i + 2 <= str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    temp |= (uint32_t) (str[i++] & 0xFF);
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.put(alpha[(temp >>  6) & 0x3F]);
                    stream.put(alpha[ temp        & 0x3F]);
                }
                else if (i + 1 == str.size())
                {
                    temp |= (uint32_t) (str[i++] & 0xFF) << 8;
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.put(alpha[(temp >>  6) & 0x3F]);
                    stream.put('=');
                }
                else if (i == str.size())
                {
                    stream.put(alpha[(temp >> 18) & 0x3F]);
                    stream.put(alpha[(temp >> 12) & 0x3F]);
                    stream.write("==", 2);
                }
            }

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            std::ostringstream stream;
            write(stream, str);
            return stream.str();
        }

        inline std::string decode(const std::string &str)
        {
            const std::string alpha = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string result;
            size_t input = 0;
            uint32_t temp = 0;

            for (size_t i = 0; i < str.size(); ++i)
            {
                size_t pos = alpha.find(str[i]);
                if (pos == std::string::npos)
                    continue;
                temp |= (uint32_t) pos << (18 - 6 * input++);
                if (input == 4)
                {
                    result.push_back((temp >> 16) & 0xFF);
                    result.push_back((temp >>  8) & 0xFF);
                    result.push_back( temp        & 0xFF);
                    input = 0;
                    temp = 0;
                }
            }

            if (input > 1) result.push_back((temp >> 16) & 0xFF);
            if (input > 2) result.push_back((temp >>  8) & 0xFF);

            return result;
        }
    }
}

#endif // CPPDATALIB_BASE64_H
