#ifndef CPPDATALIB_ERROR_H
#define CPPDATALIB_ERROR_H

namespace cppdatalib
{
    namespace core
    {
        struct error
        {
            error(const char *reason) : what_(reason) {}

            const char *what() const {return what_;}

        private:
            const char *what_;
        };
    }
}

#endif // CPPDATALIB_ERROR_H
