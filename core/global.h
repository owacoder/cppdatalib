#ifndef CPPDATALIB_GLOBAL_H
#define CPPDATALIB_GLOBAL_H

namespace cppdatalib
{
    namespace core
    {
        enum
        {
            max_utf8_code_sequence_size = 4,
#ifdef CPPDATALIB_BUFFER_SIZE
            buffer_size = CPPDATALIB_BUFFER_SIZE
#else
            buffer_size = 65535
#endif
        };
    }
}

#endif // CPPDATALIB_GLOBAL_H
