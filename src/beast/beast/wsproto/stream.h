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

#ifndef BEAST_WSPROTO_STREAM_H_INCLUDED
#define BEAST_WSPROTO_STREAM_H_INCLUDED

#include <beast/wsproto/error.h>
#include <beast/wsproto/http.h>
#include <beast/wsproto/role.h>
#include <beast/wsproto/detail/frame.h>
#include <beast/wsproto/detail/mask.h>
#include <beast/http/message.h>
#include <beast/http/parser.h>
#include <beast/is_call_possible.h>
#include <boost/asio.hpp>
#include <memory>
#include <type_traits>

namespace beast {
namespace wsproto {

template<class T, std::size_t Size>
class small_object_ptr
{
    T* t_ = nullptr;
    std::unique_ptr<T> p_;
    std::array<std::uint8_t, Size> buf_;

public:
    small_object_ptr() = default;

    ~small_object_ptr()
    {
        clear();
    }

    template<class U, class... Args>
    void
    emplace(Args&&... args)
    {
        clear();
        if(sizeof(U) <= Size)
        {
            p_ = nullptr;
            t_ = new(buf_.data()) U(
                std::forward<Args>(args)...);
            return;
        }
        auto u = std::make_unique<U>(
            std::forward<Args>(args)...);
        t_ = u.get();
        p_ = std::move(u);
    }

    T* get()
    {
        return t_;
    }

    T const* get() const
    {
        return t_;
    }

    T* operator->()
    {
        return get();
    }

    T const* operator->() const
    {
        return get();
    }

    explicit
    operator bool() const
    {
        return get() != nullptr;
    }

private:
    void
    clear()
    {
        if(! t_)
            return;
        if(p_)
            p_ = nullptr;
        else
            t_->~T();
        t_ = nullptr;
    }
};

/// Provides message-oriented functionality using WebSockets.
/**
    The stream class template provides asynchronous and blocking
    message-oriented functionality necessary for clients and
    servers to utilize the WebSockets protocol.

    @par Thread Safety
    @e Distinct @e objects: Safe.@n
    @e Shared @e objects: Unsafe. The application must also
    ensure that all asynchronous operations are performed within
    the same implicit or explicit strand.

    @par Example
    To use the WebSockets stream template with an ip::tcp::socket,
    you would write:
    @code
    boost::asio::io_service io_service;
    wsproto::stream<boost::asio:ip::tcp::socket> ws(io_service);
    @endcode

    @par Concepts: AsyncReadStream, AsyncWriteStream, Decorator,
    Stream, SyncReadStream, SyncWriteStream.
*/
template<class Stream>
class stream
{
public:
    /// The type of the next layer.
    using next_layer_type =
        std::remove_reference_t<Stream>;

    /// The type of the lowest layer.
    using lowest_layer_type =
        typename next_layer_type::lowest_layer_type;

    /// The type of endpoint of the lowest layer.
    using endpoint_type =
        typename lowest_layer_type::endpoint_type;

    static_assert(sizeof(std::size_t) >= 8,
        "64-bit environment required");

    static_assert(
        std::is_class<next_layer_type>::value &&
        ! std::is_const<next_layer_type>::value &&
        ! std::is_volatile<next_layer_type>::value,
            "Stream does not meet the type requirements");

private:
    struct abstract_decorator
    {
        virtual ~abstract_decorator() = default;
        
        virtual
        void
        operator()(beast::http::message& m) const = 0;
    };

    template<class Decorator>
    struct decorator : abstract_decorator
    {
        Decorator d;

        template<class DeducedDecorator>
        decorator(DeducedDecorator&& d_)
            : d(std::forward<DeducedDecorator>(d_))
        {
        }

        void
        operator()(beast::http::message& m) const override
        {
            d(m);
        }
    };

    Stream stream_;
    detail::frame_state fs_;
#ifndef NDEBUG
    detail::frame_state fsd_;
#endif
    detail::maskgen maskgen_;
    std::unique_ptr<abstract_decorator> decorate_;

public:
    stream(stream&&) = default;
    stream(stream const&) = delete;
    stream& operator= (stream const&) = delete;
    stream& operator= (stream&&) = delete;

    /// Construct a stream.
    /**
        This constructor creates a stream and initialises the
        underlying stream object.

        @param args The arguments to be passed to initialise the
                    underlying stream.
    */
    template<class... Args>
    explicit
    stream(Args&&... args);

    /// Destructor.
    ~stream() = default;

    /// Get the io_service associated with the object.
    /**
        This function may be used to obtain the io_service
        object that the stream uses to dispatch handlers for
        asynchronous operations.
    
        @return A reference to the io_service object that
        stream will use to dispatch handlers. Ownership is
        not transferred to the caller.
    */
    boost::asio::io_service&
    get_io_service()
    {
        return stream_.lowest_layer().get_io_service();
    }

    /// Get a reference to the next layer.
    /**
        This function returns a reference to the next layer
        in a stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers. Ownership is not transferred to the caller.
    */
    next_layer_type&
    next_layer()
    {
        return stream_;
    }

    /// Get a reference to the next layer.
    /**
        This function returns a reference to the next layer in a
        stack of stream layers.

        @return A reference to the next layer in the stack of
        stream layers. Ownership is not transferred to the caller.
    */
    next_layer_type const&
    next_layer() const
    {
        return stream_;
    }

    /// Get a reference to the lowest layer.
    /**
        This function returns a reference to the lowest layer
        in a stack of stream layers.

        @return A reference to the lowest layer in the stack of
        stream layers. Ownership is not transferred to the caller.
    */
    lowest_layer_type&
    lowest_layer()
    {
        return stream_.lowest_layer();
    }

    /// Get a reference to the lowest layer.
    /**
        This function returns a reference to the lowest layer
        in a stack of stream layers.

        @return A reference to the lowest layer in the stack of
        stream layers. Ownership is not transferred to the caller.
    */
    lowest_layer_type const&
    lowest_layer() const
    {
        return stream_.lowest_layer();
    }

    /// Close the socket.
    /**
        This function is used to close the socket. Any asynchronous
        operations will be cancelled immediately, and will complete
        with the boost::asio::error::operation_aborted error.

        @throws boost::system::system_error Thrown on failure. Note
        that, even if the function indicates an error, the underlying
        descriptor is closed.

        @note For portable behaviour with respect to graceful closure
        of a connected socket, call shutdown() before closing the socket.
    */
    void
    close()
    {
        error_code ec;
        close(ec);
        maybe_throw(ec, "close");
    }

    /// Close the socket.
    /**
        This function is used to close the socket. Any asynchronous
        operations will be cancelled immediately, and will complete
        with the boost::asio::error::operation_aborted error.

        @param ec Set to indicate what error occurred, if any. Note
        that, even if the function indicates an error, the underlying
        descriptor is closed.
        
        @par Example
        @code
        wsproto::stream<ip::tcp::socket> ws(io_service);
        ...
        wsproto::error_code ec;
        if(ws.close(ec))
        {
            // An error occurred.
        }
        @endcode

        @note For portable behaviour with respect to graceful closure of a
        connected socket, call shutdown() before closing the socket.
    */
    error_code
    close(error_code& ec)
    {
        return lowest_layer().close(ec);
    }

    /// Cancel all asynchronous operations associated with the socket.
    /**
        This function causes all outstanding asynchronous operations to
        finish immediately, and the handlers for cancelled operations
        will be passed the boost::asio::error::operation_aborted error.
    
        @throws boost::system::system_error Thrown on failure.
    */
    void
    cancel()
    {
        error_code ec;
        cancel(ec);
        maybe_throw(ec, "cancel");
    }

    /// Cancel all asynchronous operations associated with the socket.
    /**
        This function causes all outstanding asynchronous operations to
        finish immediately, and the handlers for cancelled operations
        will be passed the boost::asio::error::operation_aborted error.
    
        @param ec Set to indicate what error occurred, if any.
    */
    error_code
    cancel(error_code& ec)
    {
        return lowest_layer().cancel(ec);
    }

    /// Get the local endpoint of the socket.
    /**
        This function is used to obtain the locally bound endpoint
        of the socket.
        
        @returns An object that represents the local endpoint of
        the socket.
        
        @throws boost::system::system_error Thrown on failure.
        
        @par Example
        @code
        wsproto::stream<ip::tcp::socket> ws(io_service);
        ...
        auto const endpoint = ws.local_endpoint();
        @endcode
    */
    endpoint_type
    local_endpoint() const
    {
        error_code ec;
        auto ep = local_endpoint(ec);
        maybe_throw(ec, "local_endpoint");
    }

    /// Get the local endpoint of the socket.
    /**
        This function is used to obtain the locally bound endpoint
        of the socket.
        
        @param ec Set to indicate what error occurred, if any.
        
        @returns An object that represents the local endpoint of
        the socket. Returns a default-constructed endpoint object
        if an error occurred.
        
        @par Example
        @code
        wsproto::stream<ip::tcp::socket> ws(io_service);
        ...
        wsproto::error_code ec;
        auto endpoint = ws.local_endpoint(ec);
        if(ec)
        {
            // An error occurred.
        }
        @endcode
    */
    endpoint_type
    local_endpoint(error_code& ec) const
    {
        return lowest_layer().local_endpoint(ec);
    }

    /// Get the remote endpoint of the socket.
    /**
        This function is used to obtain the remote endpoint
        of the socket.
        
        @returns An object that represents the remote endpoint
        of the socket.
        
        @throws boost::system::system_error Thrown on failure.
        
        @par Example
        @code
        wsproto::stream<ip::tcp::socket> ws(io_service);
        ...
        auto endpoint = ws.remote_endpoint();
        @endcode
    */
    endpoint_type
    remote_endpoint() const
    {
        error_code ec;
        auto ep = remote_endpoint(ec);
        maybe_throw(ec, "remote_endpoint");
        return ep;
    }

    /// Get the local endpoint of the socket.
    /**
        This function is used to obtain the remote endpoint
        of the socket.
        
        @param ec Set to indicate what error occurred, if any.
        
        @returns An object that represents the remote endpoint of
        the socket. Returns a default-constructed endpoint object
        if an error occurred.

        @par Example
        @code
        wsproto::stream<ip::tcp::socket> ws(io_service);
        ...
        wsproto::error_code ec;
        auto endpoint = ws.remote_endpoint(ec);
        if(ec)
        {
            // An error occurred.
        }
        @endcode
    */
    endpoint_type
    remote_endpoint(error_code& ec) const
    {
        return lowest_layer().remote_endpoint(ec);
    }

    /// Set the HTTP message decorator on this object.
    /*
        The decorator is used to add custom fields to outbound
        HTTP messages. This could be used, for example, to set
        the Server or other fields.
    */
    template<class Decorator>
    void
    decorate(Decorator&& d)
    {
        static_assert(beast::is_call_possible<Decorator,
            void(beast::http::message&)>::value,
                "Type does not meet the decorator requirements");
        decorate_ = std::make_unique<
            decorator<std::decay_t<Decorator>>>(
                std::forward<Decorator>(d));
    }

    /// Request a WebSockets upgrade.
    /*
        This function is used to synchronously send a WebSocket
        Upgrade request on the stream. This function call always
        returns immediately.
        
        @throws boost::system::system_error Thrown on failure.

        @param host The name of the remote host, required by
        the HTTP protocol. Copies will be made as required.

        @param resource The requesting URI, which may not be empty,
        required by the HTTP protocol. Copies will be made as
        required.
    */
    void
    upgrade(std::string const& host,
        std::string const& resource)
    {
        error_code ec;
        upgrade(host, resource, ec);
        maybe_throw(ec, "upgrade");
    }

    /// Request a WebSockets upgrade.
    /*
        This function is used to synchronously send a WebSocket
        Upgrade request on the stream. This function call always
        returns immediately.
        
        @param host The name of the remote host, required by
        the HTTP protocol. Copies will be made as required.

        @param resource The requesting URI, which may not be empty,
        required by the HTTP protocol. Copies will be made as
        required.

        @param ec Set to indicate what error occurred, if any.
        If the remote host refuses the upgrade request, then
        ec is set to <TBD>.
        
        @par Example
        @code
        stream<ip::tcp::socket> ws(io_service);
        ...
        error_code ec;
        if(ec)
        {
            // An error occurred.
        }
        @endcode
    */
    void
    upgrade(std::string const& host,
        std::string const& resource, error_code& ec);

    /// Asynchronously request a WebSockets upgrade.
    /*
        This function is used to asynchronously send a WebSocket
        Upgrade request on the stream. This function call always
        returns immediately.

        @param host The name of the remote host, required by
        the HTTP protocol. Copies will be made as required.

        @param resource The requesting URI, which may not be empty,
        required by the HTTP protocol. Copies will be made as
        required.

        @param h The handler to be called when the request completes.
        Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
            error_code const& error // result of operation
        ); @endcode
    */
    template<class Handler>
    void
    async_upgrade(std::string const& host,
        std::string const& resource, Handler&& h);

    /// Accept a client HTTP Upgrade request
    /*
    */
    void
    accept(beast::http::message const& m);

    /// Accept a client HTTP Upgrade request
    /*
    */
    void
    accept(beast::http::message const& m, error_code& ec);

    /// Asynchronously accept a client HTTP Upgrade request
    /*
    */
    template<class Handler>
    void
    async_accept(beast::http::message const& m,
        Handler&& h);

    /// Read a frame header
    /*
        This function is used to read a WebSocket frame header
        on the stream. The call will block until one of the
        following conditions is true:

        @li The frame header is completely read.

        @li An error occurs while reading from the stream.

        If the frame header is complete, but the frame contents
        are invalid, ec is set to an appropriate error code.

        @throws boost::system::system_error Thrown on failure.

        @param fh An object to hold the frame header information.
        This reference must remain valid until the asynchronous
        operation is complete.
    */
    void
    read_fh(frame_header& fh)
    {
        error_code ec;
        read_fh(fh, ec);
        maybe_throw(ec, "read_fh");
    }

    /// Read a frame header
    /*
        This function is used to read a WebSocket frame header
        on the stream. The call will block until one of the
        following conditions is true:

        @li The frame header is completely read.

        @li An error occurs while reading from the stream.

        If the frame header is complete, but the frame contents
        are invalid, ec is set to an appropriate error code.

        @param fh An object to hold the frame header information.
        This reference must remain valid until the asynchronous
        operation is complete.

        @param ec Set to indicate what error occurred, if any.

        @par See: error
    */
    void
    read_fh(frame_header& fh, error_code& ec);

    /// Start reading a frame header asynchronously
    /**
        This function is used to asynchronously read a WebSocket
        frame header on the stream. This function call always returns
        immediately.

        @param fh An object to hold the frame header information.
        This reference must remain valid until the asynchronous
        operation is complete.

        @param h The handler to be called when the read completes.
        Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
            error_code const& error // result of operation
        ); @endcode
    */
    template<class Handler>
    void
    async_read_fh(frame_header& fh, Handler&& h);

    /// Start reading a frame payload asynchronously
    /**
        This function is used to asynchronously read a WebSocket
        frame payload on the stream. This function call always
        returns immediately.

        @param fh The contents of the corresponding frame header.

        @param b A object meeting the requirements of MutableBufferSequence
        which will receive the payload data. Although the buffers object
        may be copied as necessary, ownership of the underlying buffers is
        retained by the caller, which must guarantee that they remain
        valid until the handler is called. Requires:
        @code boost::asio::buffer_size(b) == fh.len @endcode

        @param h The handler to be called when the read completes.
        Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
            boost::system::error_code const& error // result of operation
        ); @endcode
    */
    template<class MutableBuffers, class Handler>
    void
    async_read(frame_header const& fh,
        MutableBuffers&& b, Handler&& h);

    /// Write an entire frame to a stream before returning.
    /**
        This function is used to write a frame to a stream. The
        call will block until one of the following conditions is true:

        @li All of the data in the supplied buffers has been written.

        @li An error occurs.

        This operation is implemented in terms of one or more calls
        to the stream's write_some function. The actual payload sent
        may be transformed as per the WebSocket protocol settings.

        @param fin `true` if this is the last frame in the message.

        @param buffers One or more buffers containing the frame's
        payload data.

        @param ec Set to indicate what error occurred, if any.
    */
    template<class ConstBufferSequence>
    void
    write(opcode::value op, bool fin,
        ConstBufferSequence const& buffers, error_code& ec);

    /// Write an entire frame to a stream before returning.
    /**
        This function is used to write a frame to a stream. The
        call will block until one of the following conditions is true:

        @li All of the data in the supplied buffers has been written.

        @li An error occurs.

        This operation is implemented in terms of one or more calls
        to the stream's write_some function. The actual payload sent
        may be transformed as per the WebSocket protocol settings.

        @throws boost::system::system_error Thrown on failure.

        @param fin `true` if this is the last frame in the message.

        @param buffers One or more buffers containing the frame's
        payload data.
    */
    template<class ConstBufferSequence>
    void
    write(opcode::value op, bool fin, ConstBufferSequence const& buffers)
    {
        error_code ec;
        write(op, fin, buffers, ec);
        maybe_throw(ec, "write");
    }

    /// Start writing a frame asynchronously
    /**
        This function is used to asynchronously write a WebSocket
        frame on the stream. This function call always returns
        immediately.

        @param fin A bool indicating whether or not the frame is the
        last frame in the corresponding WebSockets message.

        @param b A object meeting the requirements of ConstBufferSequence
        which holds the payload data before any masking or compression.
        Although the buffers object may be copied as necessary, ownership
        of the underlying buffers is retained by the caller, which must
        guarantee that they remain valid until the handler is called. 

        @param h The handler to be called when the write completes.
        Copies will be made of the handler as required. The equivalent
        function signature of the handler must be:
        @code void handler(
            boost::system::error_code const& error // result of operation
        ); @endcode
    */
    template<class ConstBuffers, class WriteHandler>
    void
    async_write(opcode::value op, bool fin,
        ConstBuffers const& b, WriteHandler&& h);

private:
    template<class String>
    static
    void
    maybe_throw(error_code const& ec, String const& what)
    {
        if(ec)
            throw boost::system::system_error{ec};
    }

    beast::asio::streambuf
    write_error_response(error_code const& ec);

    beast::http::message
    make_upgrade(std::string const& host,
        std::string const& resource);

    beast::asio::streambuf
    make_response(beast::http::message const& r);

    error_code
    do_accept(beast::http::message const& r);
};

} // wsproto
} // beast

#include <beast/wsproto/impl/stream.ipp>

#endif
