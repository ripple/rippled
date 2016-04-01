//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef BEAST_ASIO_APPEND_BUFFERS_H_INLUDED
#define BEAST_ASIO_APPEND_BUFFERS_H_INLUDED

#include <boost/asio/buffer.hpp>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace beast {
namespace asio {

namespace detail {

template<class ValueType, class... Bs>
class append_buffers_helper
{
    std::tuple<Bs...> bs_;

public:
    using value_type = ValueType;

    class const_iterator;

    append_buffers_helper(append_buffers_helper&&) = default;
    append_buffers_helper(append_buffers_helper const&) = default;
    append_buffers_helper& operator=(append_buffers_helper&&) = default;
    append_buffers_helper& operator=(append_buffers_helper const&) = default;

    explicit
    append_buffers_helper(Bs const&... bs)
        : bs_(bs...)
    {
    }

    const_iterator
    begin() const;

    const_iterator
    end() const;
};

template<class ValueType, class... Bs>
class append_buffers_helper<
    ValueType, Bs...>::const_iterator
{
    template<class U>
    static
    inline
    std::size_t constexpr
    max_sizeof()
    {
        return sizeof(U);
    }

    template<class U0, class U1, class... Us>
    static
    inline
    std::size_t constexpr
    max_sizeof()
    {
        return
            max_sizeof<U0>() > max_sizeof<U1, Us...>() ?
            max_sizeof<U0>() : max_sizeof<U1, Us...>();
    }

    std::size_t n_;
    std::tuple<Bs...> const* bs_;
    std::array<std::uint8_t,
        max_sizeof<typename Bs::const_iterator...>()> buf_;

    friend class append_buffers_helper<ValueType, Bs...>;

    template<std::size_t I>
    using C = std::integral_constant<std::size_t, I>;

    template<std::size_t I>
    using iter_t = typename std::tuple_element_t<
        I, std::tuple<Bs...>>::const_iterator;

    template<std::size_t I>
    inline
    iter_t<I>&
    iter()
    {
        return *reinterpret_cast<
            iter_t<I>*>(buf_.data());
    }

    template<std::size_t I>
    inline
    iter_t<I> const&
    iter() const
    {
        return *reinterpret_cast<
            iter_t<I> const*>(buf_.data());
    }

public:
    using value_type = ValueType;
    using pointer = value_type const*;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category =
        std::bidirectional_iterator_tag;

    ~const_iterator();
    const_iterator();
    const_iterator(const_iterator&& other);
    const_iterator(const_iterator const& other);
    const_iterator& operator=(const_iterator&& other);
    const_iterator& operator=(const_iterator const& other);

    bool
    operator==(const_iterator const& other) const;

    bool
    operator!=(const_iterator const& other) const
    {
        return !(*this == other);
    }

    reference
    operator*() const;

    // VFALCO Unsupported since we return by value
    /*
    pointer
    operator->() const
    {
        return &**this;
    }
    */

    const_iterator&
    operator++();

    const_iterator
    operator++(int)
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    const_iterator&
    operator--();

    const_iterator
    operator--(int)
    {
        auto temp = *this;
        --(*this);
        return temp;
    }

private:
    const_iterator(
        std::tuple<Bs...> const& bs, bool at_end);

    inline
    void
    construct(C<sizeof...(Bs)>)
    {
        auto constexpr I = sizeof...(Bs);
        n_ = I;
    }

    template<std::size_t I>
    inline
    void
    construct(C<I>)
    {
        if(std::get<I>(*bs_).begin() !=
            std::get<I>(*bs_).end())
        {
            n_ = I;
            new(&iter<I>()) iter_t<I>(
                std::get<I>(*bs_).begin());
            return;
        }
        construct(C<I+1>{});
    }

    inline
    void
    destroy(C<sizeof...(Bs)>)
    {
        return;
    }

    template<std::size_t I>
    inline
    void
    destroy(C<I>)
    {
        if(n_ == I)
        {
            using Iter = iter_t<I>;
            iter<I>().~Iter();
            return;
        }
        destroy(C<I+1>{});
    }

    inline
    void
    move(C<sizeof...(Bs)>, const_iterator&&)
    {
        return;
    }

    template<std::size_t I>
    inline
    void
    move(C<I>, const_iterator&& other)
    {
        if(n_ == I)
        {
            new(&iter<I>()) iter_t<I>(
                std::move(other.iter<I>()));
            return;
        }
        move(C<I+1>{}, std::move(other));
    }

    inline
    void
    copy(C<sizeof...(Bs)>, const_iterator const&)
    {
        return;
    }

    template<std::size_t I>
    inline
    void
    copy(C<I>, const_iterator const& other)
    {
        if(n_ == I)
        {
            new(&iter<I>()) iter_t<I>(
                other.iter<I>());
            return;
        }
        copy(C<I+1>{}, other);
    }

    inline
    bool
    equal(C<sizeof...(Bs)>,
        const_iterator const&) const
    {
        return true;
    }

    template<std::size_t I>
    inline
    bool
    equal(C<I>, const_iterator const& other) const
    {
        if(n_ == I)
            return iter<I>() == other.iter<I>();
        return equal(C<I+1>{}, other);
    }

    [[noreturn]]
    inline
    reference
    dereference(C<sizeof...(Bs)>) const
    {
        throw std::logic_error("invalid iterator");
    }

    template<std::size_t I>
    inline
    reference
    dereference(C<I>) const
    {
        if(n_ == I)
            return *iter<I>();
        return dereference(C<I+1>{});
    }

    [[noreturn]]
    inline
    void
    increment(C<sizeof...(Bs)>)
    {
        throw std::logic_error("invalid iterator");
    }

    template<std::size_t I>
    inline
    void
    increment(C<I>)
    {
        if(n_ == I)
        {
            if(++iter<I>() !=
                    std::get<I>(*bs_).end())
                return;
            using Iter = iter_t<I>;
            iter<I>().~Iter();
            return construct(C<I+1>{});
        }
        increment(C<I+1>{});
    }

    inline
    void
    decrement(C<sizeof...(Bs)>)
    {
        auto constexpr I = sizeof...(Bs);
        if(n_ == I)
        {
            --n_;
            new(&iter<I-1>()) iter_t<I-1>(
                std::get<I-1>(*bs_).end());
        }
        decrement(C<I-1>{});
    }

    inline
    void
    decrement(C<0>)
    {
        auto constexpr I = 0;
        if(iter<I>() != std::get<I>(*bs_).begin())
        {
            --iter<I>();
            return;
        }
        throw std::logic_error("invalid iterator");
    }

    template<std::size_t I>
    inline
    void
    decrement(C<I>)
    {
        if(n_ == I)
        {
            if(iter<I>() != std::get<I>(*bs_).begin())
            {
                --iter<I>();
                return;
            }
            --n_;
            using Iter = iter_t<I>;
            iter<I>().~Iter();
            new(&iter<I-1>()) iter_t<I-1>(
                std::get<I-1>(*bs_).end());
        }
        decrement(C<I-1>{});
    }
};

//------------------------------------------------------------------------------

template<class ValueType, class... Bs>
append_buffers_helper<ValueType, Bs...>::
const_iterator::~const_iterator()
{
    destroy(C<0>{});
}

template<class ValueType, class... Bs>
append_buffers_helper<ValueType, Bs...>::
const_iterator::const_iterator()
    : n_(sizeof...(Bs))
{
}

template<class ValueType, class... Bs>
append_buffers_helper<ValueType, Bs...>::
const_iterator::const_iterator(
    std::tuple<Bs...> const& bs, bool at_end)
    : bs_(&bs)
{
    if(at_end)
        n_ = sizeof...(Bs);
    else
        construct(C<0>{});
}

template<class ValueType, class... Bs>
append_buffers_helper<ValueType, Bs...>::
const_iterator::const_iterator(const_iterator&& other)
    : n_(other.n_)
    , bs_(other.bs_)
{
    move(C<0>{}, std::move(other));
}

template<class ValueType, class... Bs>
append_buffers_helper<ValueType, Bs...>::
const_iterator::const_iterator(const_iterator const& other)
    : n_(other.n_)
    , bs_(other.bs_)
{
    copy(C<0>{}, other);
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator=(const_iterator&& other) ->
    const_iterator&
{
    destroy(C<0>{});
    n_ = other.n_;
    bs_ = other.bs_;
    move(C<0>{}, std::move(other));
    return *this;
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator=(const_iterator const& other) ->
const_iterator&
{
    destroy(C<0>{});
    n_ = other.n_;
    bs_ = other.bs_;
    copy(C<0>{}, other);
    return *this;
}

template<class ValueType, class... Bs>
bool
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator==(const_iterator const& other) const
{
    if(bs_ != other.bs_)
        return false;
    if(n_ != other.n_)
        return false;
    return equal(C<0>{}, other);
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator*() const ->
    reference
{
    return dereference(C<0>{});
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator++() ->
    const_iterator&
{
    increment(C<0>{});
    return *this;
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::
const_iterator::operator--() ->
    const_iterator&
{
    decrement(C<sizeof...(Bs)>{});
    return *this;
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::begin() const ->
    const_iterator
{
    return const_iterator(bs_, false);
}

template<class ValueType, class... Bs>
auto
append_buffers_helper<ValueType, Bs...>::end() const ->
    const_iterator
{
    return const_iterator(bs_, true);
}

template<class... O>
struct are_same;

template<class T, class... O>
struct are_same<T, T, O...>: public are_same<T, O...> { };

template<class T, class U, class... O>
struct are_same<T, U, O...>: public std::is_same<T, U> { };

template<class T>
struct are_same<T, T>: public std::is_same<T, T> { };

template<class T, class U>
struct are_same<T, U>: public std::is_same<T, U> { };

//template<class... O>
//bool constexpr are_same_v = are_same<O...>::value;

} // detail

//------------------------------------------------------------------------------

/// Concatenate 2 or more BufferSequence to form a ConstBufferSequence
template<class B1, class B2, class... Bn>
auto
append_buffers(B1 const& b1, B2 const& b2, Bn const&... bn)
{
    using namespace boost::asio;
    return detail::append_buffers_helper<const_buffer,
        B1, B2, Bn...>(b1, b2, bn...);
}

} // asio
} // beast

#endif
