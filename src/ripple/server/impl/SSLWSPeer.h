//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef RIPPLE_SERVER_SSLWSPEER_H_INCLUDED
#define RIPPLE_SERVER_SSLWSPEER_H_INCLUDED

#include <ripple/server/impl/BaseHTTPPeer.h>
#include <ripple/server/WSSession.h>
#include <beast/asio/ssl_bundle.h>
#include <beast/asio/placeholders.h>
#include <beast/wsproto/ssl.h>
#include <memory>

namespace ripple {

class SSLWSPeer
    : public BaseWSPeer<SSLWSPeer>
    , public std::enable_shared_from_this<SSLWSPeer>
{
private:
    friend class BasePeer<SSLWSPeer>;
    friend class BaseWSPeer<SSLWSPeer>;

    using clock_type = std::chrono::system_clock;
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using waitable_timer =
        boost::asio::basic_waitable_timer <clock_type>;

    std::unique_ptr<beast::asio::ssl_bundle> ssl_bundle_;
    beast::wsproto::socket<
        beast::asio::ssl_bundle::stream_type&> ws_;

public:
    template<class Body, class Headers>
    SSLWSPeer(
        Port const& port,
        Handler& handler,
        endpoint_type remote_endpoint,
        beast::http::message<true, Body, Headers>&& request,
        std::unique_ptr<
            beast::asio::ssl_bundle>&& ssl_bundle,
        beast::Journal journal);

private:
    void
    do_close() override;

    void
    on_shutdown(error_code ec);
};

//------------------------------------------------------------------------------

template<class Body, class Headers>
SSLWSPeer::SSLWSPeer(
    Port const& port,
    Handler& handler,
    endpoint_type remote_endpoint,
    beast::http::message<true, Body, Headers>&& request,
    std::unique_ptr<
        beast::asio::ssl_bundle>&& ssl_bundle,
    beast::Journal journal)
    : BaseWSPeer(port, handler, remote_endpoint, std::move(request),
        ssl_bundle->socket.get_io_service(), journal)
    , ssl_bundle_(std::move(ssl_bundle))
    , ws_(ssl_bundle_->stream)
{
}

void
SSLWSPeer::do_close()
{
    //start_timer();
    using namespace beast::asio;
    ws_.next_layer().async_shutdown(
        strand_.wrap(std::bind(&SSLWSPeer::on_shutdown,
            shared_from_this(), placeholders::error)));
}

void
SSLWSPeer::on_shutdown(error_code ec)
{
    //cancel_timer();
    ws_.lowest_layer().close(ec);
}

} // ripple

#endif
