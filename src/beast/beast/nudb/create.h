//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2014, Vinnie Falco <vinnie.falco@gmail.com>

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

#ifndef BEAST_NUDB_CREATE_H_INCLUDED
#define BEAST_NUDB_CREATE_H_INCLUDED

#include <beast/nudb/file.h>
#include <beast/nudb/detail/bucket.h>
#include <beast/nudb/detail/config.h>
#include <beast/nudb/detail/format.h>
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace beast {
namespace nudb {

/** Create a new database.
    Preconditions:
        The files must not exist
    Throws:

    @param args Arguments passed to File constructors
    @return `false` if any file could not be created.
*/
template <class Hasher = default_hash>
bool
create (
    path_type const& dat_path,
    path_type const& key_path,
    path_type const& log_path,
    std::uint64_t appnum,
    std::uint64_t salt,
    std::size_t key_size,
    std::size_t block_size,
    float load_factor)
{
    using namespace detail;
    using File = native_file;
    if (key_size < 1)
        throw std::domain_error(
            "invalid key size");
    if (block_size > field<std::uint16_t>::max)
        throw std::domain_error(
            "nudb: block size too large");
    if (load_factor <= 0.f)
        throw std::domain_error(
            "nudb: load factor too small");
    if (load_factor >= 1.f)
        throw std::domain_error(
            "nudb: load factor too large");
    auto const capacity =
        bucket_capacity(key_size, block_size);
    if (capacity < 1)
        throw std::domain_error(
            "nudb: block size too small");
    File df;
    File kf;
    File lf;
    for(;;)
    {
        if (df.create(
            file_mode_append, dat_path))
        {
            if (kf.create (
                file_mode_append, key_path))
            {
                if (lf.create(
                        file_mode_append, log_path))
                    break;
                File::erase (dat_path);
            }
            File::erase (key_path);
        }
        return false;
    }

    dat_file_header dh;
    dh.version = currentVersion;
    dh.appnum = appnum;
    dh.salt = salt;
    dh.key_size = key_size;

    key_file_header kh;
    kh.version = currentVersion;
    kh.appnum = appnum;
    kh.salt = salt;
    kh.pepper = pepper<Hasher>(salt);
    kh.key_size = key_size;
    kh.block_size = block_size;
    // VFALCO Should it be 65536?
    //        How do we set the min?
    kh.load_factor = std::min<std::size_t>(
        65536.0 * load_factor, 65535);
    write (df, dh);
    write (kf, kh);
    buffer buf(block_size);
    std::memset(buf.get(), 0, block_size);
    bucket b (key_size, block_size,
        buf.get(), empty);
    b.write (kf, block_size);
    // VFALCO Leave log file empty?
    df.sync();
    kf.sync();
    lf.sync();
    return true;
}

} // nudb
} // beast

#endif
