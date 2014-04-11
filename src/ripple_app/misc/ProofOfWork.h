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

#ifndef RIPPLE_PROOFOFWORK_H
#define RIPPLE_PROOFOFWORK_H

#include "PowResult.h"

namespace ripple {

class ProofOfWork : LeakChecked <ProofOfWork>
{
public:
    enum
    {
        kMaxIterations = (1 << 23)
    };

    typedef boost::shared_ptr <ProofOfWork> pointer;

    ProofOfWork (const std::string& token,
                 int iterations,
                 uint256 const& challenge,
                 uint256 const& target);

    explicit ProofOfWork (const std::string& token);

    bool isValid () const;

    uint256 solve (int maxIterations = 2 * kMaxIterations) const;
    bool checkSolution (uint256 const& solution) const;

    const std::string& getToken () const
    {
        return mToken;
    }
    uint256 const& getChallenge () const
    {
        return mChallenge;
    }

    uint64 getDifficulty () const
    {
        return getDifficulty (mTarget, mIterations);
    }

    // approximate number of hashes needed to solve
    static uint64 getDifficulty (uint256 const& target, int iterations);

    static bool validateToken (const std::string& strToken);

    static bool calcResultInfo (PowResult powCode, std::string& strToken, std::string& strHuman);

private:
    std::string     mToken;
    uint256         mChallenge;
    uint256         mTarget;
    int             mIterations;

    static const uint256 sMinTarget;
    static const int maxIterations;
};

}

#endif

