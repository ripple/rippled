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

#ifndef RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED
#define RIPPLE_PROTOCOL_BUILDINFO_H_INCLUDED

#include <cstdint>
#include <string>

namespace ripple {

/** Versioning information for this build. */
// VFALCO The namespace is deprecated
namespace BuildInfo {

/** Server version.
    Follows the Semantic Versioning Specification:
    http://semver.org/
*/
std::string const&
getVersionString();

/** Full server version string.
    This includes the name of the server. It is used in the peer
    protocol hello message and also the headers of some HTTP replies.
*/
std::string const&
getFullVersionString();

/** Returns the server version packed in a 64-bit integer.

    The general format is:

    ........-........-........-........-........-........-........-........
    XXXXXXXX-XXXXXXXX-YYYYYYYY-YYYYYYYY-YYYYYYYY-YYYYYYYY-YYYYYYYY-YYYYYYYY

    X: 16 bits identifying the particular implementation
    Y: 48 bits of data specific to the implementation

    The rippled-specific format (implementation ID is: 0x18 0x3B) is:

    00011000-00111011-MMMMMMMM-mmmmmmmm-pppppppp-TTNNNNNN-00000000-00000000

        M: 8-bit major version (0-255)
        m: 8-bit minor version (0-255)
        p: 8-bit patch version (0-255)
        T: 11 if neither an RC nor a beta
           10 if an RC
           01 if a beta
        N: 6-bit rc/beta number (1-63)
*/
std::uint64_t
getEncodedVersion();

/** Compare the local node's rippled server software version with the given one.

    @note We only compare the major version, minor version, and patch version.
    @param othersVersion another node's rippled server software version
    @return true if local node's rippled server software version is lower,
            false otherwise.
*/
bool
localVersionLow(std::uint64_t othersVersion);

static constexpr float versionUpgradeWarningThreshold = 0.6;

}  // namespace BuildInfo

}  // namespace ripple

#endif
