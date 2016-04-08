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

#ifndef BEAST_HTTP_READ_IPP_H_INCLUDED
#define BEAST_HTTP_READ_IPP_H_INCLUDED

#include <beast/http/type_check.h>
#include <beast/asio/async_completion.h>
#include <beast/asio/bind_handler.h>
#include <beast/asio/handler_alloc.h>
#include <cassert>

namespace beast {
namespace http {

namespace detail {

template<class Stream, class Streambuf,
    bool isRequest, class Body, class Allocator,
        class Handler>
class read_op
{
    using alloc_type =
        handler_alloc<char, Handler>;

    using parser_type =
        parser<isRequest, Body, Allocator>;

    using message_type =
        parsed_message<isRequest, Body, Allocator>;

    struct data
    {
        Stream& s;
        Streambuf& sb;
        message_type& m;
        parser_type p;
        Handler h;
        int state = 0;

        template<class DeducedHandler>
        data(DeducedHandler&& h_, Stream& s_,
                Streambuf& sb_, message_type& m_)
            : s(s_)
            , sb(sb_)
            , m(m_)
            , h(std::forward<DeducedHandler>(h_))
        {
        }
    };

    std::shared_ptr<data> d_;

public:
    read_op(read_op&&) = default;
    read_op(read_op const&) = default;

    template<class DeducedHandler, class... Args>
    read_op(DeducedHandler&& h, Stream&s, Args&&... args)
        : d_(std::allocate_shared<data>(alloc_type{h},
            std::forward<DeducedHandler>(h), s,
                std::forward<Args>(args)...))
    {
        (*this)(error_code{}, 0);
    }

    void
    operator()(error_code ec,
        std::size_t bytes_transferred);

    friend
    auto asio_handler_allocate(
        std::size_t size, read_op* op)
    {
        return boost_asio_handler_alloc_helpers::
            allocate(size, op->d_->h);
    }

    friend
    auto asio_handler_deallocate(
        void* p, std::size_t size, read_op* op)
    {
        return boost_asio_handler_alloc_helpers::
            deallocate(p, size, op->d_->h);
    }

    friend
    auto asio_handler_is_continuation(read_op* op)
    {
        return op->d_->state >= 2 ||
            boost_asio_handler_cont_helpers::
                is_continuation(op->d_->h);
    }

    template <class Function>
    friend
    auto asio_handler_invoke(Function&& f, read_op* op)
    {
        return boost_asio_handler_invoke_helpers::
            invoke(f, op->d_->h);
    }
};

template<class Stream, class Streambuf,
    bool isRequest, class Body, class Allocator,
        class Handler>
void
read_op<Stream, Streambuf, isRequest, Body, Allocator, Handler>::
operator()(error_code ec, std::size_t bytes_transferred)
{
    auto& d = *d_;
    while(d.state != 99)
    {
        switch(d.state)
        {
        case 0:
        {
            auto const used =
                d.p.write(d.sb.data(), ec);
            if(ec)
            {
                d.s.get_io_service().post(
                    bind_handler(std::move(d.h), ec));
                return;
            }
            d.sb.consume(used);
            if(d.p.complete())
            {
                // VFALCO The intent is to move, is std::move needed here?
                d.m = d.p.release();
                d.s.get_io_service().post(
                    bind_handler(std::move(d.h), ec));
                return;
            }
            d.state = 1;
            break;
        }

        case 1:
        case 3:
            // read
            d.state = 2;
            d.s.async_read_some(d.sb.prepare(
                read_size_helper(d.sb, 65536)),
                    std::move(*this));
            return;

        // got data
        case 2:
        {
            if(ec == boost::asio::error::eof)
            {
                if(! d.p.started())
                {
                    // call handler
                    d.state = 99;
                    break;
                }
                // Caller will see eof on next read.
                ec = {};
                d.p.write_eof(ec);
                if(! ec)
                {
                    assert(d.p.complete());
                    // VFALCO The intent is to move, is std::move needed here?
                    d.m = d.p.release();
                }
                // call handler
                d.state = 99;
                break;
            }
            if(ec)
            {
                // call handler
                d.state = 99;
                break;
            }
            d.sb.commit(bytes_transferred);
            d.sb.consume(d.p.write(d.sb.data(), ec));
            if(ec)
            {
                // call handler
                d.state = 99;
                break;
            }
            if(d.p.complete())
            {
                // call handler
                d.state = 99;
                // VFALCO The intent is to move, is std::move needed here?
                d.m = d.p.release();
                break;
            }
            d.state = 3;
            break;
        }
        }
    }
    d.h(ec);
}

} // detail

//------------------------------------------------------------------------------

template<class SyncReadStream, class Streambuf,
    bool isRequest, class Body, class Allocator>
void
read(SyncReadStream& stream, Streambuf& streambuf,
    parsed_message<isRequest, Body, Allocator>& m,
        error_code& ec)
{
    static_assert(is_SyncReadStream<SyncReadStream>::value,
        "SyncReadStream requirements not met");
    static_assert(is_Streambuf<Streambuf>::value,
        "Streambuf requirements not met");
    parser<isRequest, Body, Allocator> p;
    for(;;)
    {
        auto used =
            p.write(streambuf.data(), ec);
        if(ec)
            return;
        streambuf.consume(used);
        if(p.complete())
        {
            // VFALCO The intent is to move, is std::move needed here?
            m = p.release();
            break;
        }
        streambuf.commit(stream.read_some(
            streambuf.prepare(read_size_helper(
                streambuf, 65536)), ec));
        if(ec && ec != boost::asio::error::eof)
            return;
        if(ec == boost::asio::error::eof)
        {
            if(! p.started())
                return;
            // Caller will see eof on next read.
            ec = {};
            p.write_eof(ec);
            if(ec)
                return;
            assert(p.complete());
            // VFALCO The intent is to move, is std::move needed here?
            m = p.release();
            break;
        }
    }
}

template<class AsyncReadStream, class Streambuf,
    bool isRequest, class Body, class Allocator,
        class CompletionToken>
auto
async_read(AsyncReadStream& stream, Streambuf& streambuf,
    parsed_message<isRequest, Body, Allocator>& m,
        CompletionToken&& token)
{
    static_assert(is_AsyncReadStream<AsyncReadStream>::value,
        "AsyncReadStream requirements not met");
    static_assert(is_Streambuf<Streambuf>::value,
        "Streambuf requirements not met");
    beast::async_completion<CompletionToken,
        void(error_code)> completion(token);
    detail::read_op<AsyncReadStream, Streambuf,
        isRequest, Body, Allocator, decltype(
            completion.handler)>{completion.handler,
                stream, streambuf, m};
    return completion.result.get();
}

} // http
} // beast

#endif
