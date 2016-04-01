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

#include <BeastConfig.h>
#include <ripple/test/impl/async_echo_peer.h>
#include <ripple/test/impl/sync_echo_peer.h>
#include <beast/unit_test/suite.h>
#include <beast/unit_test/thread.h>
#include <boost/asio/spawn.hpp>

namespace beast {
namespace wsproto {

class ws_test : public unit_test::suite
{
public:
    using error_code = boost::system::error_code;
    using endpoint_type = boost::asio::ip::tcp::endpoint;
    using address_type = boost::asio::ip::address;
    using socket_type = boost::asio::ip::tcp::socket;
    using yield_context = boost::asio::yield_context;

    endpoint_type ep_;

    //--------------------------------------------------------------------------

    // opcodes for creating the test plans

    // concurrent read and write
    struct case_1{};

    // write a bad frame and shut down
    struct case_2{};

    //--------------------------------------------------------------------------

    class coro_peer
    {
        error_code ec_;
        boost::asio::io_service ios_;
        boost::asio::ip::tcp::acceptor acceptor_;
        socket_type sock_;
        socket<socket_type&> ws_;
        opcode::value op_;
        beast::asio::streambuf rb_;
        beast::asio::streambuf wb_;
        yield_context* yield_;
        int state_ = 0;
        //unit_test::suite& test_;

    public:
        coro_peer(coro_peer&&) = default;
        coro_peer(coro_peer const&) = delete;
        coro_peer& operator=(coro_peer&&) = delete;
        coro_peer& operator=(coro_peer const&) = delete;

        template<class... Ops>
        coro_peer(bool server, endpoint_type ep,
                unit_test::suite& test, Ops const&... ops)
            : acceptor_(ios_)
            , sock_(ios_)
            , ws_(sock_)
            //, test_(test)
        {
            if(server)
            {
                acceptor_.open(ep.protocol());
                acceptor_.bind(ep);
                acceptor_.listen(
                    boost::asio::socket_base::max_connections);
                boost::asio::spawn(ios_,
                    [=](auto yield)
                    {
                        yield_ = &yield;
                        state_ = 10;
                        acceptor_.async_accept(sock_, (*yield_)[ec_]);
                        if(ec_)
                            return this->fail("accept");
                        state_ = 20;
                        ws_.async_accept((*yield_)[ec_]);
                        if(ec_)
                            return this->fail("ws.accept");
                        this->invoke(ops...);
                        state_ = -1;
                    });
            }
            else
            {
                boost::asio::spawn(ios_,
                    [=](auto yield)
                    {
                        yield_ = &yield;
                        state_ = 30;
                        sock_.async_connect(ep, (*yield_)[ec_]);
                        if(ec_)
                            return this->fail("connect");
                        state_ = 40;
                        ws_.async_handshake(ep.address().to_string() +
                            std::to_string(ep.port()), "/", (*yield_)[ec_]);
                        if(ec_)
                            return this->fail("handshake");
                        this->invoke(ops...);
                        state_ = -1;
                    });
            }
        }

        ~coro_peer()
        {
        }

        int
        state() const
        {
            return state_;
        }

        void
        run_one()
        {
            ios_.run_one();
        }

        void
        step_to(int to = 0)
        {
            while(state_ != to)
                ios_.run_one();
        }

    private:
        template<class String>
        void fail(String const& s)
        {
        }

        void invoke_1(case_1)
        {
            async_read(ws_, op_, rb_,
                [&](auto ec)
                {
                    if(ec)
                        return this->fail(ec);
                    rb_.consume(rb_.size());
                });
            state_ = 100;
            async_write(ws_, opcode::text,
                boost::asio::null_buffers{}, (*yield_)[ec_]);
            if(ec_)
                return fail("write");
        }

        void invoke_1(case_2)
        {
            detail::frame_header fh;
            fh.op = opcode::rsv5; // bad opcode
            fh.fin = true;
            fh.mask = true;
            fh.rsv1 = false;
            fh.rsv2 = false;
            fh.rsv3 = false;
            fh.len = 0;
            fh.key = 0;
            detail::write(wb_, fh);
            state_ = 200;
            boost::asio::async_write(
                ws_.next_layer(), wb_.data(),
                    (*yield_)[ec_]);
            if(ec_)
                return fail("write");
            ws_.next_layer().shutdown(
                socket_type::shutdown_both, ec_);
            if(ec_)
                return fail("shutdown");
        }

        inline
        void
        invoke()
        {
        }

        template<class Op, class... Ops>
        inline
        void
        invoke(Op op, Ops const&... ops)
        {
            invoke_1(op);
            invoke(ops...);
        }
    };

    void
    testInvokable()
    {   
        endpoint_type const ep{
            address_type::from_string(
                "127.0.0.1"), 6000};
        coro_peer server(true, ep, *this, case_1{});
        coro_peer client(false, ep, *this, case_2{});
        server.step_to(10);     // async_accept
        client.step_to(30);     //                          async_connect
        server.step_to(20);     // async_accept(ws)
        client.step_to(40);     //                          async_handshake
        server.step_to(100);    // case_1
        client.step_to(200);    //                          case_2
        client.step_to(-1);
        server.step_to(-1);
    }

    //--------------------------------------------------------------------------

    template<class Buffers>
    static
    std::string
    buffers_to_string(Buffers const& bs)
    {
        using namespace boost::asio;
        std::string s;
        s.reserve(buffer_size(bs));
        for(auto const& b : bs)
            s.append(buffer_cast<char const*>(b),
                buffer_size(b));
        return s;
    }

    void
    check(int status, std::string const& s)
    {
        expect(request(s) == status);
    }

    void
    maybe_fail(error_code const& ec, std::string const& what)
    {
        expect(! ec, what + ": " + ec.message());
    }

    void
    maybe_throw(error_code ec, std::string what)
    {
        if(ec)
        {
            maybe_fail(ec, what);
            throw ec;
        }
    }

    int
    request(std::string const& s)
    {
        using namespace boost::asio;
        io_service ios;
        ip::tcp::socket sock(ios);
        sock.connect(ep_);
        write(sock, asio::append_buffers(
            buffer(s), buffer("\r\n")));
        http::body b;
        http::message m;
        http::parser p(m, b, false);
        streambuf sb;
        read_until(sock, sb, "\r\n\r\n");
        auto const result = p.write(sb.data());
        sock.shutdown(socket_type::shutdown_both);
        sock.close();
        if(! p.complete() || result.first)
            return -1;
        return m.status();
    }

    void
    testHandshake(endpoint_type ep)
    {
#if 0
        ep_ = ep;

        check(40, "GET / HTTP/1.0\r\n");
#endif
    }

    void
    syncEchoClient(endpoint_type ep)
    {
        using namespace boost::asio;
        error_code ec;
        io_service ios;
        wsproto::socket<socket_type> ws(ios);
        ws.next_layer().connect(ep, ec);
        maybe_fail(ec, "connect");
        ws.handshake(ep.address().to_string(), "/", ec);
        maybe_fail(ec, "upgrade");
        std::string const s = "Hello, world!";
        ws.write(wsproto::opcode::text, true, buffer(s), ec);
        maybe_fail(ec, "write");
        streambuf sb;
        wsproto::opcode::value op;
        read(ws, op, sb, ec);
        maybe_fail(ec, "read");
        if(! ec)
            expect(op == wsproto::opcode::text);
        expect(buffers_to_string(sb.data()) == s);
        sb.consume(sb.size());
        ws.close({}, ec);
        maybe_fail(ec, "close");
        while(! ec)
        {
            read(ws, op, sb, ec);
            if(! ec)
                sb.consume(sb.size());
        }
        if(ec != error::closed)
            maybe_fail(ec, "teardown");
    }

    void
    run() override
    {
        testInvokable();

        {
            endpoint_type ep{
                address_type::from_string("127.0.0.1"), 6000};
            testcase("Echo Server");
            test::sync_echo_peer s(true, ep, *this);
            testHandshake(ep);
            syncEchoClient(ep);
        }

        {
            endpoint_type ep{
                address_type::from_string("127.0.0.1"), 6001};
            testcase("Async Echo Server");
            test::async_echo_peer s(true, ep, *this);
            testHandshake(ep);
            syncEchoClient(ep);
        }
    }
};

BEAST_DEFINE_TESTSUITE(ws,asio,beast);

} // wsproto
} // beast
