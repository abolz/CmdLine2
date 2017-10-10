// Copyright (c) 2017 Alexander Bolz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef CXX_STRING_VIEW_H
#define CXX_STRING_VIEW_H 1

#include <cstddef> // for _HAS_CXX17 etc...

#ifndef CXX_HAS_INCLUDE
#if defined(__has_include)
#define CXX_HAS_INCLUDE(X) __has_include(X)
#else
#define CXX_HAS_INCLUDE(X) 0
#endif
#endif

#ifndef CXX_HAS_STD_STRING_VIEW
#if __cplusplus >= 201703 || (_MSC_VER >= 1910 && _HAS_CXX17)
#define CXX_HAS_STD_STRING_VIEW 1
#endif
#endif

#ifndef CXX_HAS_STD_EXPERIMENTAL_STRING_VIEW
#if CXX_HAS_INCLUDE(<experimental/string_view>) && __cplusplus > 201103
#define CXX_HAS_STD_EXPERIMENTAL_STRING_VIEW 1
#endif
#endif

#if CXX_HAS_STD_STRING_VIEW

#include <string_view>
namespace cxx { using std::string_view; }

#elif CXX_HAS_STD_EXPERIMENTAL_STRING_VIEW

#include <experimental/string_view>
namespace cxx { using std::experimental::string_view; }

#else

// Replacement for std::string_view.
// Incomplete.
// Does not support constexpr and noexcept!

// Set to 1 to enable iterator debugging.
#define CXX_STRING_VIEW_CHECKED_ITERATOR 0

#include <cassert>
#include <cstdint>
#include <cstring>
#if CXX_STRING_VIEW_CHECKED_ITERATOR
#include <iterator> // random_access_iterator_tag...
#endif
#include <type_traits>

namespace cxx {

#if CXX_STRING_VIEW_CHECKED_ITERATOR

class string_view_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = char;
    using reference         = char const&;
    using pointer           = char const*;
    using difference_type   = intptr_t;

private:
    pointer         ptr_  = nullptr;
    difference_type pos_  = 0;
    difference_type size_ = 0;

public:
    /*constexpr*/ string_view_iterator() /*noexcept*/ = default;
    /*constexpr*/ string_view_iterator(string_view_iterator const&) /*noexcept*/ = default;

    /*constexpr*/ string_view_iterator(pointer ptr, difference_type size, difference_type pos = 0) /*noexcept*/
        : ptr_(ptr)
        , pos_(pos)
        , size_(size)
    {
        assert(size_ >= 0);
        assert(size_ == 0 || ptr_ != nullptr);
        assert(pos_ >= 0);
        assert(pos_ <= size_);
    }

    /*constexpr*/ reference operator*() const /*noexcept*/
    {
        assert(ptr_ != nullptr);
        assert(pos_ < size_);
        return ptr_[pos_];
    }

    /*constexpr*/ pointer operator->() const /*noexcept*/
    {
        assert(ptr_ != nullptr);
        assert(pos_ < size_);
        return ptr_ + pos_;
    }

    /*constexpr*/ string_view_iterator& operator++() /*noexcept*/
    {
        assert(pos_ < size_);
        ++pos_;
        return *this;
    }

    /*constexpr*/ string_view_iterator operator++(int) /*noexcept*/
    {
        auto t = *this;
        ++(*this);
        return t;
    }

    /*constexpr*/ string_view_iterator& operator--() /*noexcept*/
    {
        assert(pos_ > 0);
        --pos_;
        return *this;
    }

    /*constexpr*/ string_view_iterator operator--(int) /*noexcept*/
    {
        auto t = *this;
        --(*this);
        return t;
    }

    /*constexpr*/ string_view_iterator& operator+=(difference_type n) /*noexcept*/
    {
        assert(pos_ + n >= 0);
        assert(pos_ + n <= size_);
        pos_ += n;
        return *this;
    }

    /*constexpr*/ string_view_iterator operator+(difference_type n) const /*noexcept*/
    {
        auto t = *this;
        t += n;
        return t;
    }

    /*constexpr*/ friend string_view_iterator operator+(difference_type n, string_view_iterator it) /*noexcept*/
    {
        return it + n;
    }

    /*constexpr*/ string_view_iterator& operator-=(difference_type n) /*noexcept*/
    {
        assert(pos_ - n >= 0);
        assert(pos_ - n <= size_);
        pos_ -= n;
        return *this;
    }

    /*constexpr*/ string_view_iterator operator-(difference_type n) const /*noexcept*/
    {
        auto t = *this;
        t -= n;
        return t;
    }

    /*constexpr*/ difference_type operator-(string_view_iterator rhs) const /*noexcept*/
    {
        assert(ptr_ == rhs.ptr_);
        return pos_ - rhs.pos_;
    }

    /*constexpr*/ reference operator[](difference_type index) const /*noexcept*/
    {
        assert(ptr_ != nullptr);
        assert(pos_ + index >= 0);
        assert(pos_ + index < size_);
        return ptr_[pos_ + index];
    }

    /*constexpr*/ friend bool operator==(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        assert(lhs.ptr_ == rhs.ptr_);
        return lhs.pos_ == rhs.pos_;
    }

    /*constexpr*/ friend bool operator!=(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        return !(lhs == rhs);
    }

    /*constexpr*/ friend bool operator<(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        assert(lhs.ptr_ == rhs.ptr_);
        return lhs.pos_ < rhs.pos_;
    }

    /*constexpr*/ friend bool operator>(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        return rhs < lhs;
    }

    /*constexpr*/ friend bool operator<=(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        return !(rhs < lhs);
    }

    /*constexpr*/ friend bool operator>=(string_view_iterator lhs, string_view_iterator rhs) /*noexcept*/
    {
        return !(lhs < rhs);
    }

#ifdef _MSC_VER
    using _Checked_type = string_view_iterator;
    using _Unchecked_type = pointer;

    /*constexpr*/ friend _Unchecked_type _Unchecked(_Checked_type it) /*noexcept*/
    {
        return it.ptr_ + it.pos_;
    }

    /*constexpr*/ friend _Checked_type& _Rechecked(_Checked_type& it, _Unchecked_type p) /*noexcept*/
    {
        it.pos_ = p - it.ptr_;
        return it;
    }
#endif
};

#endif // CXX_STRING_VIEW_CHECKED_ITERATOR

class string_view // A minimal std::string_view replacement
{
public:
    using value_type        = char;
    using pointer           = char const*;
    using const_pointer     = char const*;
    using reference         = char const&;
    using const_reference   = char const&;
#if CXX_STRING_VIEW_CHECKED_ITERATOR
    using iterator          = string_view_iterator;
    using const_iterator    = string_view_iterator;
#else
    using iterator          = char const*;
    using const_iterator    = char const*;
#endif
    using size_type         = size_t;

private:
    const_pointer data_ = nullptr;
    size_t        size_ = 0; // intptr_t size_ = 0; XXX

private:
    static size_t Min(size_t x, size_t y) { return y < x ? y : x; }
    static size_t Max(size_t x, size_t y) { return y < x ? x : y; }

    static /*__forceinline*/ int Compare(char const* s1, char const* s2, size_t n) /*noexcept*/
    {
        return n == 0 ? 0 : ::memcmp(s1, s2, n);
    }

    static /*__forceinline*/ char const* Find(char const* s, size_t n, char ch) /*noexcept*/
    {
        assert(n != 0);
        return static_cast<char const*>( ::memchr(s, static_cast<unsigned char>(ch), n) );
    }

public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    /*constexpr*/ string_view() /*noexcept*/ = default;
    /*constexpr*/ string_view(string_view const&) /*noexcept*/ = default;

    /*constexpr*/ string_view(const_pointer ptr, size_t len) /*noexcept*/
        : data_(ptr)
        , size_(len)
    {
        assert(size_ <= max_size());
        assert(size_ == 0 || data_ != nullptr);
    }

    /*constexpr*/ string_view(const_pointer c_str) /*noexcept*/
        : data_(c_str)
        , size_(c_str ? ::strlen(c_str) : 0u)
    {
        assert(size_ <= max_size());
    }

    template <
        typename String,
        typename DataT = decltype(std::declval<String const&>().data()),
        typename SizeT = decltype(std::declval<String const&>().size()),
        typename = typename std::enable_if<
            std::is_convertible<DataT, const_pointer>::value &&
            std::is_convertible<SizeT, size_t>::value
        >::type
    >
    string_view(String const& str)
        : data_(str.data())
        , size_(str.size())
    {
        assert(size_ <= max_size());
        assert(size_ == 0 || data_ != nullptr);
    }

    template <
        typename T,
        typename = typename std::enable_if<
            std::is_constructible<T, const_iterator, const_iterator>::value
        >::type
    >
    explicit operator T() const
    {
        return T(begin(), end());
    }

    /*constexpr*/ size_t max_size() const /*noexcept*/
    {
        return INTPTR_MAX;
    }

    // Returns a pointer to the start of the string.
    /*constexpr*/ const_pointer data() const /*noexcept*/
    {
        return data_;
    }

    // Returns the length of the string.
    /*constexpr*/ size_t size() const /*noexcept*/
    {
        return size_;
    }

    // Returns the length of the string.
    /*constexpr*/ intptr_t ssize() const /*noexcept*/
    {
        assert(size_ <= max_size());
        return static_cast<intptr_t>(size_);
    }

    // Returns the length of the string.
    /*constexpr*/ size_t length() const /*noexcept*/
    {
        return size();
    }

    // Returns the length of the string.
    /*constexpr*/ intptr_t slength() const /*noexcept*/
    {
        return ssize();
    }

    // Returns whether the string is empty.
    /*constexpr*/ bool empty() const /*noexcept*/
    {
        return size_ == 0;
    }

    // Returns an iterator pointing to the start of the string.
    /*constexpr*/ const_iterator begin() const /*noexcept*/
    {
#if CXX_STRING_VIEW_CHECKED_ITERATOR
        return const_iterator(data_, ssize(), 0);
#else
        return data_;
#endif
    }

    // Returns an iterator pointing past the end of the string.
    /*constexpr*/ const_iterator end() const /*noexcept*/
    {
#if CXX_STRING_VIEW_CHECKED_ITERATOR
        return const_iterator(data_, ssize(), ssize());
#else
        return data_ + size_;
#endif
    }

    // Returns a reference to the N-th character of the string.
    /*constexpr*/ const_reference operator[](size_t n) const /*noexcept*/
    {
        assert(n < size_);
        return data_[n];
    }

    bool _cmp_eq(string_view other) const /*noexcept*/
    {
        return size() == other.size()
            && Compare(data(), other.data(), size()) == 0;
    }

    bool _cmp_lt(string_view other) const /*noexcept*/
    {
        int const c = Compare(data(), other.data(), Min(size(), other.size()));
        return c < 0 || (c == 0 && size() < other.size());
    }

    // Lexicographically compare this string with another string OTHER.
    int compare(string_view other)
    {
        int const c = Compare(data(), other.data(), Min(size(), other.size()));
        if (c != 0)
            return c;
        if (size() < other.size())
            return -1;
        if (size() > other.size())
            return +1;
        return 0;
    }

    // Removes the first N characters from the string.
    void remove_prefix(size_t n) /*noexcept*/
    {
        assert(n <= size_);
        data_ += n;
        size_ -= n;
    }

    // Removes the last N characters from the string.
    void remove_suffix(size_t n) /*noexcept*/
    {
        assert(n <= size_);
        size_ -= n;
    }

    // Returns a reference to the first character of the string.
    /*constexpr*/ const_reference front() const /*noexcept*/
    {
        assert(size_ != 0);
        return data_[0];
    }

    // Returns a reference to the last character of the string.
    /*constexpr*/ const_reference back() const /*noexcept*/
    {
        assert(size_ != 0);
        return data_[size_ - 1];
    }

    // Returns the substring [first, +count)
    /*constexpr*/ string_view substr(size_t first = 0) const /*noexcept*/
    {
        assert(first <= size_);
        return string_view(data_ + first, size_ - first);
    }

    // Returns the substring [first, +count)
    /*constexpr*/ string_view substr(size_t first, size_t count) const /*noexcept*/
    {
        assert(first <= size_);
        return string_view(data_ + first, Min(count, size_ - first));
    }

    // Search for the first character ch in the sub-string [from, end)
    size_t find(char ch, size_t from = 0) const /*noexcept*/;

    // Search for the first substring str in the sub-string [from, end)
    size_t find(string_view str, size_t from = 0) const /*noexcept*/;

    // Search for the first character ch in the sub-string [from, end)
    size_t find_first_of(char ch, size_t from = 0) const /*noexcept*/;

    // Search for the first character in the sub-string [from, end)
    // which matches any of the characters in chars.
    size_t find_first_of(string_view chars, size_t from = 0) const /*noexcept*/;

    // Search for the first character in the sub-string [from, end)
    // which does not match any of the characters in chars.
    size_t find_first_not_of(string_view chars, size_t from = 0) const /*noexcept*/;

    // Search for the last character ch in the sub-string [0, from)
    size_t rfind(char ch, size_t from = npos) const /*noexcept*/;

    // Search for the last character ch in the sub-string [0, from)
    size_t find_last_of(char ch, size_t from = npos) const /*noexcept*/;

    // Search for the last character in the sub-string [0, from)
    // which matches any of the characters in chars.
    size_t find_last_of(string_view chars, size_t from = npos) const /*noexcept*/;

    // Search for the last character in the sub-string [0, from)
    // which does not match any of the characters in chars.
    size_t find_last_not_of(string_view chars, size_t from = npos) const /*noexcept*/;
};

inline size_t string_view::find(char ch, size_t from) const /*noexcept*/
{
    if (from >= size())
        return npos;

    if (auto I = Find(data() + from, size() - from, ch))
        return static_cast<size_t>(I - data());

    return npos;
}

inline size_t string_view::find(string_view str, size_t from) const /*noexcept*/
{
    if (str.size() == 1)
        return find(str[0], from);

    if (from > size() || str.size() > size())
        return npos;

    if (str.empty())
        return from;

    for (auto I = from; I != size() - str.size() + 1; ++I)
    {
        if (Compare(data() + I, str.data(), str.size()) == 0)
            return I;
    }

    return npos;
}

inline size_t string_view::find_first_of(char ch, size_t from) const /*noexcept*/
{
    return find(ch, from);
}

inline size_t string_view::find_first_of(string_view chars, size_t from) const /*noexcept*/
{
    if (chars.size() == 1)
        return find(chars[0], from);

    if (from >= size() || chars.empty())
        return npos;

    for (auto I = from; I != size(); ++I)
    {
        if (Find(chars.data(), chars.size(), data()[I]))
            return I;
    }

    return npos;
}

inline size_t string_view::find_first_not_of(string_view chars, size_t from) const /*noexcept*/
{
    if (from >= size())
        return npos;

    for (auto I = from; I != size(); ++I)
    {
        if (!Find(chars.data(), chars.size(), data()[I]))
            return I;
    }

    return npos;
}

inline size_t string_view::rfind(char ch, size_t from) const /*noexcept*/
{
    if (from < size())
        ++from;
    else
        from = size();

    for (auto I = from; I != 0; --I)
    {
        if (static_cast<unsigned char>(ch) == static_cast<unsigned char>(data()[I - 1]))
            return I - 1;
    }

    return npos;
}

inline size_t string_view::find_last_of(char ch, size_t from) const /*noexcept*/
{
    return rfind(ch, from);
}

inline size_t string_view::find_last_of(string_view chars, size_t from) const /*noexcept*/
{
    if (chars.size() == 1)
        return rfind(chars[0], from);

    if (chars.empty())
        return npos;

    if (from < size())
        ++from;
    else
        from = size();

    for (auto I = from; I != 0; --I)
    {
        if (Find(chars.data(), chars.size(), data()[I - 1]))
            return I - 1;
    }

    return npos;
}

inline size_t string_view::find_last_not_of(string_view chars, size_t from) const /*noexcept*/
{
    if (from < size())
        ++from;
    else
        from = size();

    for (auto I = from; I != 0; --I)
    {
        if (!Find(chars.data(), chars.size(), data()[I - 1]))
            return I - 1;
    }

    return npos;
}

inline bool operator==(string_view s1, string_view s2) /*noexcept*/
{
    return s1._cmp_eq(s2);
}

inline bool operator!=(string_view s1, string_view s2) /*noexcept*/
{
    return !(s1 == s2);
}

inline bool operator<(string_view s1, string_view s2) /*noexcept*/
{
    return s1._cmp_lt(s2);
}

inline bool operator<=(string_view s1, string_view s2) /*noexcept*/
{
    return !(s2 < s1);
}

inline bool operator>(string_view s1, string_view s2) /*noexcept*/
{
    return s2 < s1;
}

inline bool operator>=(string_view s1, string_view s2) /*noexcept*/
{
    return !(s1 < s2);
}

} // namespace cxx

#endif

#endif
