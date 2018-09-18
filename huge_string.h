#ifndef HUGE_STRING_H
#define HUGE_STRING_H

#include <cstdint>
#include <limits>
#include <cstdlib>
#include <cstdio>

template<typename T, size_t buffer_size = 4096>
class huge_string
{
    typedef huge_string<T, buffer_size> type;

    static constexpr size_t width = sizeof(T);

    struct filename_tag {};
    huge_string(const char *filename, filename_tag) : store_(std::fopen(filename, "wb+")), effective_size_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open '" + std::string(filename) + "' for use");
    }

public:
    class reference
    {
        template<typename, size_t>
        friend class huge_string;

        reference(type &parent, uint64_t pos) : parent(parent), pos(pos) {}
    public:
        operator T() const
        {
            parent.seek(pos);
            return parent.read();
        }
        reference &operator=(T chr)
        {
            parent.seek(pos);
            parent.write(1, chr);
            return *this;
        }

    private:
        type &parent;
        uint64_t pos;
    };

    class const_reference
    {
        template<typename, size_t>
        friend class huge_string;

        const_reference(const type &parent, uint64_t pos) : parent(parent), pos(pos) {}
    public:
        operator T() const
        {
            parent.seek(pos);
            return parent.read();
        }

    private:
        const type &parent;
        uint64_t pos;
    };

    huge_string() : store_(std::tmpfile()), effective_size_(0), effective_pos_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");
    }
    huge_string(const T *string) : store_(std::tmpfile()), effective_size_(string_length(string)), effective_pos_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");

        write(string, effective_size_);
    }
    huge_string(const T *string, size_t len) : store_(std::tmpfile()), effective_size_(len), effective_pos_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");

        write(string, len);
    }
    huge_string(uint64_t len, T chr) : store_(std::tmpfile()), effective_size_(len), effective_pos_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");

        write(len, chr);
    }
    template<typename It>
    huge_string(It first, It last) : store_(std::tmpfile()), effective_size_(0), effective_pos_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");

        while (first != last)
        {
            write(1, *first);
            ++effective_size_;
        }
    }
    huge_string(const type &other) : store_(std::tmpfile()), effective_size_(0), effective_size_(0)
    {
        if (store_ == nullptr)
            throw std::runtime_error("huge_string - could not open temporary file for use");

        std::rewind(other.store_);
        other.effective_pos_ = 0;

        copy(other.store_, other.size());
    }
    huge_string(type &&other) : store_(other.store_), effective_size_(other.effective_size_), effective_size_(other.effective_pos_)
    {
        other.store_ = nullptr;
    }

    ~huge_string()
    {
        if (store_ != nullptr)
            std::fclose(store_);
    }

    uint64_t size() const {return effective_size_;}
    uint64_t length() const {return effective_size_;}
    uint64_t max_size() const {return std::numeric_limits<uint64_t>::max()/width;}

    void resize(uint64_t size) {resize(size, {});}
    void resize(uint64_t size, T chr)
    {
        if (size < effective_size_)
            effective_size_ = size;
        else if (size > effective_size_)
        {
            seek(effective_size_);
            write(size - effective_size_, chr);
        }
    }

    void clear() {effective_size_ = 0;}
    bool empty() {return effective_size_ == 0;}

    void shrink_to_fit()
    {
        type new_;
        std::rewind(store_);
        new_.copy(store_, effective_size_);
        swap(new_);
    }

    void swap(type &other)
    {
        std::swap(store_, other.store_);
        std::swap(effective_size_, other.effective_size_);
        std::swap(effective_pos_, other.effective_pos_);
    }

    type &append(const type &other)
    {
        seek(effective_size_);
        other.seek(0);
        return copy(other.store_, other.effective_size_);
    }

    // TODO: substring append()

    type &append(const T *s) {return append(s, string_length(s));}
    type &append(const T *s, size_t len)
    {
        seek(effective_size_);
        effective_size_ += len;
        return write(s, len);
    }
    type &append(uint64_t len, T chr)
    {
        seek(effective_size_);
        effective_size_ += len;
        return write(len, chr);
    }

    void push_back(T chr)
    {
        seek(effective_size_);
        write(1, chr);
        effective_size_ += 1;
    }
    void pop_back() {effective_size_ -= 1;}

    const_reference operator[](uint64_t pos) const {return const_reference(*this, pos);}
    reference operator[](uint64_t pos) {return reference(*this, pos);}

    const_reference front() const {return const_reference(*this, 0);}
    reference front() {return reference(*this, 0);}

    const_reference back() const {return const_reference(*this, effective_size_ - 1);}
    reference back() {return reference(*this, effective_size_ - 1);}

private:
    static uint64_t string_length(const T *s)
    {
        const T *end = s;
        while (*end) ++end;
        return end - s;
    }

    /* Copies up to `length` bytes from current file position of `other` to current position of current file,
     * and updates `effective_pos_` and `effective_size_` to the new string size */
    type &copy(FILE *other, uint64_t length)
    {
        T buffer[buffer_size];
        size_t read = 0, written = 0;

        do
        {
            uint64_t current_length = std::min(length, uint64_t(buffer_size));

            if ((read = std::fread(buffer, width, current_length, other)) < current_length)
            {
                if (std::ferror(other))
                    throw std::runtime_error("huge_string - unable to read from file");
            }

            if ((written = std::fwrite(buffer, width, read, store_)) < read)
            {
                if (std::ferror(store_))
                    throw std::runtime_error("huge_string - unable to write to file");
            }

            effective_pos_ += read;
            length -= current_length;
        } while (read == buffer_size);

        effective_size_ = effective_pos_;
        return *this;
    }

    T read() const
    {
        T buffer;
        if (std::fread(&buffer, width, 1, store_) != 1)
            throw std::runtime_error("huge_string - unable to read from file");

        effective_pos_ += 1;
        return buffer;
    }

    /* Writes `len` items from `s` to the current position in the file */
    type &write(const T *s, size_t len)
    {
        if (std::fwrite(s, width, len, store_) != len)
            throw std::runtime_error("huge_string - unable to write to file");

        effective_pos_ += len;
        return *this;
    }

    /* Writes `len` copies of `chr` starting at the current position in the file */
    type &write(uint64_t len, T chr)
    {
        T buffer[buffer_size];

        for (size_t i = 0; i < buffer_size; ++i)
            buffer[i] = chr;

        effective_pos_ += len;

        while (len > 0)
        {
            size_t current_len = std::min(len, uint64_t(buffer_size));

            if (std::fwrite(buffer, width, current_len, store_) != current_len)
                throw std::runtime_error("huge_string - unable to write to file");

            len -= current_len;
        }

        return *this;
    }

    const type &seek_relative(int64_t offset) const
    {
        if (std::fseek(store_, offset*width, SEEK_CUR))
            throw std::runtime_error("huge_string - unable to perform relative seek in file");

        effective_pos_ += offset;
        return *this;
    }

    const type &seek(uint64_t absolute) const
    {
        if (absolute == effective_pos_)
            return *this;

        if (std::fseek(store_, absolute*width, SEEK_SET))
            throw std::runtime_error("huge_string - unable to perform absolute seek in file");

        effective_pos_ = absolute;
        return *this;
    }

    mutable FILE *store_;
    uint64_t effective_size_;
    mutable uint64_t effective_pos_;
};

#endif // HUGE_STRING_H
