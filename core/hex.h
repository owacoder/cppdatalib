#ifndef CPPDATALIB_HEX_H
#define CPPDATALIB_HEX_H

#include <iostream>
#include <sstream>

namespace cppdatalib
{
    namespace hex
    {
        inline std::ostream &write(std::ostream &stream, unsigned char c)
        {
            const char alpha[] = "0123456789ABCDEF";

            stream.put(alpha[(c & 0xff) >> 4]);
            stream.put(alpha[c & 0xf]);

            return stream;
        }

        inline std::ostream &write(std::ostream &stream, const std::string &str)
        {
            for (auto c: str)
                write(stream, c);

            return stream;
        }

        inline std::string encode(const std::string &str)
        {
            std::ostringstream stream;
            write(stream, str);
            return stream.str();
        }
    }
}

#endif // CPPDATALIB_HEX_H
