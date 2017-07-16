#pragma once

#ifndef STD_STRING_VIEW_CHECKED_ITERATOR
#ifndef NDEBUG
#define STD_STRING_VIEW_CHECKED_ITERATOR 1
#else
#define STD_STRING_VIEW_CHECKED_ITERATOR 0
#endif
#endif

#include <cassert>
#include <cstring>
#if STD_STRING_VIEW_CHECKED_ITERATOR
#include <iterator> // random_access_iterator_tag...
#endif
#include <type_traits>

#if STD_STRING_VIEW_CHECKED_ITERATOR
class std__string_view_iterator
{
public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type        = char;
    using reference         = char const&;
    using pointer           = char const*;
    using difference_type   = std::ptrdiff_t;

private:
    pointer         ptr_  = nullptr;
    difference_type pos_  = 0;
    difference_type size_ = 0;

public:
    /*constexpr*/ std__string_view_iterator() noexcept = default;
    /*constexpr*/ std__string_view_iterator(std__string_view_iterator const&) noexcept = default;
    /*constexpr*/ std__string_view_iterator& operator=(std__string_view_iterator const&) noexcept = default;

    /*constexpr*/ std__string_view_iterator(pointer ptr, difference_type size, difference_type pos = 0) noexcept
        : ptr_(ptr)
        , pos_(pos)
        , size_(size)
    {
        assert(size_ >= 0);
        assert(size_ == 0 || ptr_ != nullptr);
        assert(pos_ >= 0);
        assert(pos_ <= size_);
    }

    /*constexpr*/ pointer ptr() const noexcept {
        return ptr_ + pos_;
    }

    /*constexpr*/ reference operator*() const noexcept {
        assert(ptr_ != nullptr);
        assert(pos_ < size_);
        return ptr_[pos_];
    }

    /*constexpr*/ pointer operator->() const noexcept {
        assert(ptr_ != nullptr);
        assert(pos_ < size_);
        return ptr_ + pos_;
    }

    /*constexpr*/ std__string_view_iterator& operator++() noexcept {
        assert(pos_ < size_);
        ++pos_;
        return *this;
    }

    /*constexpr*/ std__string_view_iterator operator++(int) noexcept {
        auto t = *this;
        ++(*this);
        return t;
    }

    /*constexpr*/ std__string_view_iterator& operator--() noexcept {
        assert(pos_ > 0);
        --pos_;
        return *this;
    }

    /*constexpr*/ std__string_view_iterator operator--(int) noexcept {
        auto t = *this;
        --(*this);
        return t;
    }

    /*constexpr*/ std__string_view_iterator& operator+=(difference_type n) noexcept {
        assert(pos_ + n >= 0);
        assert(pos_ + n <= size_);
        pos_ += n;
        return *this;
    }

    /*constexpr*/ std__string_view_iterator operator+(difference_type n) const noexcept {
        auto t = *this;
        t += n;
        return t;
    }

    /*constexpr*/ friend std__string_view_iterator operator+(difference_type n, std__string_view_iterator it) noexcept {
        return it + n;
    }

    /*constexpr*/ std__string_view_iterator& operator-=(difference_type n) noexcept {
        assert(pos_ - n >= 0);
        assert(pos_ - n <= size_);
        pos_ -= n;
        return *this;
    }

    /*constexpr*/ std__string_view_iterator operator-(difference_type n) const noexcept {
        auto t = *this;
        t -= n;
        return t;
    }

    /*constexpr*/ difference_type operator-(std__string_view_iterator rhs) const noexcept {
        assert(ptr_ == rhs.ptr_);
        return pos_ - rhs.pos_;
    }

    /*constexpr*/ reference operator[](difference_type index) const noexcept {
        assert(ptr_ != nullptr);
        assert(pos_ + index >= 0);
        assert(pos_ + index < size_);
        return ptr_[pos_ + index];
    }

    /*constexpr*/ friend bool operator==(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        assert(lhs.ptr_ == rhs.ptr_);
        return lhs.pos_ == rhs.pos_;
    }

    /*constexpr*/ friend bool operator!=(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        return !(lhs == rhs);
    }

    /*constexpr*/ friend bool operator<(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        assert(lhs.ptr_ == rhs.ptr_);
        return lhs.pos_ < rhs.pos_;
    }

    /*constexpr*/ friend bool operator>(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        return rhs < lhs;
    }

    /*constexpr*/ friend bool operator<=(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        return !(rhs < lhs);
    }

    /*constexpr*/ friend bool operator>=(std__string_view_iterator lhs, std__string_view_iterator rhs) noexcept {
        return !(lhs < rhs);
    }

#ifdef _MSC_VER
    using _Checked_type = std__string_view_iterator;
    using _Unchecked_type = pointer;

    /*constexpr*/ friend _Unchecked_type _Unchecked(_Checked_type it) noexcept {
        return it.ptr_ + it.pos_;
    }

    /*constexpr*/ friend _Checked_type& _Rechecked(_Checked_type& it, _Unchecked_type p) noexcept {
        it.pos_ = p - it.ptr_;
        return it;
    }
#endif
};
#endif

class std__string_view
{
public:
    using value_type        = char;
    using pointer           = char const*;
    using const_pointer     = char const*;
    using reference         = char const&;
    using const_reference   = char const&;
#if STD_STRING_VIEW_CHECKED_ITERATOR
    using iterator          = std__string_view_iterator;
    using const_iterator    = std__string_view_iterator;
#else
    using iterator          = char const*;
    using const_iterator    = char const*;
#endif
    using size_type         = size_t;

private:
    const_pointer data_ = nullptr;
    size_t        size_ = 0;

private:
    static size_t Min(size_t x, size_t y) { return y < x ? y : x; }
    static size_t Max(size_t x, size_t y) { return y < x ? x : y; }

    static /*__forceinline*/ int Compare(char const* s1, char const* s2, size_t n) noexcept
    {
        return n == 0 ? 0 : ::memcmp(s1, s2, n);
    }

    static /*__forceinline*/ char const* Find(char const* s, size_t n, char ch) noexcept
    {
        return static_cast<char const*>( ::memchr(s, static_cast<unsigned char>(ch), n) );
    }

public:
    static constexpr size_t npos = static_cast<size_t>(-1);

    /*constexpr*/ std__string_view() noexcept = default;
    /*constexpr*/ std__string_view(std__string_view const&) noexcept = default;

    /*constexpr*/ std__string_view(const_pointer ptr, size_t len) noexcept
        : data_(ptr)
        , size_(len)
    {
        assert(size_ == 0 || data_ != nullptr);
    }

    std__string_view(const_pointer c_str) noexcept
        : data_(c_str)
        , size_(c_str ? ::strlen(c_str) : 0u)
    {
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
    std__string_view(String const& str)
        : data_(str.data())
        , size_(str.size())
    {
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

    // Returns a pointer to the start of the string.
    /*constexpr*/ const_pointer data() const noexcept
    {
        return data_;
    }

    // Returns the length of the string.
    /*constexpr*/ size_t size() const noexcept
    {
        return size_;
    }

    // Returns the length of the string.
    /*constexpr*/ size_t length() const noexcept
    {
        return size_;
    }

    // Returns whether the string is empty.
    /*constexpr*/ bool empty() const noexcept
    {
        return size_ == 0;
    }

    // Returns an iterator pointing to the start of the string.
    /*constexpr*/ const_iterator begin() const noexcept
    {
#if STD_STRING_VIEW_CHECKED_ITERATOR
        return const_iterator{data_, static_cast<std::ptrdiff_t>(size_), 0};
#else
        return data_;
#endif
    }

    // Returns an iterator pointing past the end of the string.
    /*constexpr*/ const_iterator end() const noexcept
    {
#if STD_STRING_VIEW_CHECKED_ITERATOR
        return const_iterator{data_, static_cast<std::ptrdiff_t>(size_), static_cast<std::ptrdiff_t>(size_)};
#else
        return data_ + size_;
#endif
    }

    // Returns a reference to the N-th character of the string.
    /*constexpr*/ const_reference operator[](size_t n) const noexcept
    {
        assert(n < size_);
        return data_[n];
    }

    // Returns whether this string is equal to another string STR.
    bool equal(std__string_view str) const noexcept
    {
        return size() == str.size()
            && 0 == Compare(data(), str.data(), size());
    }

    // Lexicographically compare this string with another string STR.
    bool less(std__string_view str) const noexcept
    {
        int c = Compare(data(), str.data(), Min(size(), str.size()));
        return c < 0 || (c == 0 && size() < str.size());
    }

    // Removes the first N characters from the string.
    void remove_prefix(size_t n) noexcept
    {
        assert(n <= size_);
        data_ += n;
        size_ -= n;
    }

    // Removes the last N characters from the string.
    void remove_suffix(size_t n) noexcept
    {
        assert(n <= size_);
        size_ -= n;
    }

    // Returns a reference to the first character of the string.
    /*constexpr*/ const_reference front() const noexcept
    {
        assert(size_ != 0);
        return data_[0];
    }

    // Returns a reference to the last character of the string.
    /*constexpr*/ const_reference back() const noexcept
    {
        assert(size_ != 0);
        return data_[size_ - 1];
    }

    // Returns the substring [first, +count)
    /*constexpr*/ std__string_view substr(size_t first = 0, size_t count = npos) const noexcept
    {
        size_t const f = Min(first, size_);
        size_t const n = Min(count, size_ - f);
        return { data_ + f, n };
    }

    // Search for the first character ch in the sub-string [from, end)
    size_t find(char ch, size_t from = 0) const noexcept;

    // Search for the first substring str in the sub-string [from, end)
    size_t find(std__string_view str, size_t from = 0) const noexcept;

    // Search for the first character ch in the sub-string [from, end)
    size_t find_first_of(char ch, size_t from = 0) const noexcept;

    // Search for the first character in the sub-string [from, end)
    // which matches any of the characters in chars.
    size_t find_first_of(std__string_view chars, size_t from = 0) const noexcept;

    // Search for the first character in the sub-string [from, end)
    // which does not match any of the characters in chars.
    size_t find_first_not_of(std__string_view chars, size_t from = 0) const noexcept;

    // Search for the last character ch in the sub-string [0, from)
    size_t rfind(char ch, size_t from = npos) const noexcept;

    // Search for the last character ch in the sub-string [0, from)
    size_t find_last_of(char ch, size_t from = npos) const noexcept;

    // Search for the last character in the sub-string [0, from)
    // which matches any of the characters in chars.
    size_t find_last_of(std__string_view chars, size_t from = npos) const noexcept;

    // Search for the last character in the sub-string [0, from)
    // which does not match any of the characters in chars.
    size_t find_last_not_of(std__string_view chars, size_t from = npos) const noexcept;
};

inline size_t std__string_view::find(char ch, size_t from) const noexcept
{
    if (from >= size())
        return npos;

    if (auto I = Find(data() + from, size() - from, ch))
        return static_cast<size_t>(I - data());

    return npos;
}

inline size_t std__string_view::find(std__string_view str, size_t from) const noexcept
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

inline size_t std__string_view::find_first_of(char ch, size_t from) const noexcept
{
    return find(ch, from);
}

inline size_t std__string_view::find_first_of(std__string_view chars, size_t from) const noexcept
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

inline size_t std__string_view::find_first_not_of(std__string_view chars, size_t from) const noexcept
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

inline size_t std__string_view::rfind(char ch, size_t from) const noexcept
{
    if (from < size())
        ++from;
    else
        from = size();

    for (auto I = from; I != 0; --I)
    {
        if (ch == data()[I - 1])
            return I - 1;
    }

    return npos;
}

inline size_t std__string_view::find_last_of(char ch, size_t from) const noexcept
{
    return rfind(ch, from);
}

inline size_t std__string_view::find_last_of(std__string_view chars, size_t from) const noexcept
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

inline size_t std__string_view::find_last_not_of(std__string_view chars, size_t from) const noexcept
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

inline bool operator==(std__string_view s1, std__string_view s2) noexcept
{
    return s1.equal(s2);
}

inline bool operator!=(std__string_view s1, std__string_view s2) noexcept
{
    return !(s1 == s2);
}

inline bool operator<(std__string_view s1, std__string_view s2) noexcept
{
    return s1.less(s2);
}

inline bool operator<=(std__string_view s1, std__string_view s2) noexcept
{
    return !(s2 < s1);
}

inline bool operator>(std__string_view s1, std__string_view s2) noexcept
{
    return s2 < s1;
}

inline bool operator>=(std__string_view s1, std__string_view s2) noexcept
{
    return !(s1 < s2);
}
