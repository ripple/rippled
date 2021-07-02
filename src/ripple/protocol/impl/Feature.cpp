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

#include <ripple/basics/Slice.h>
#include <ripple/basics/contract.h>
#include <ripple/protocol/Feature.h>
#include <ripple/protocol/digest.h>

#include <cstring>

namespace ripple {

enum class Supported : bool { no = false, yes };

namespace detail {
// *NOTE*
//
// Features, or Amendments as they are called elsewhere, are enabled on the
// network at some specific time based on Validator voting.  Features are
// enabled using run-time conditionals based on the state of the amendment.
// There is value in retaining that conditional code for some time after
// the amendment is enabled to make it simple to replay old transactions.
// However, once an Amendment has been enabled for, say, more than two years
// then retaining that conditional code has less value since it is
// uncommon to replay such old transactions.
//
// Starting in January of 2020 Amendment conditionals from before January
// 2018 are being removed.  So replaying any ledger from before January
// 2018 needs to happen on an older version of the server code.  There's
// a log message in Application.cpp that warns about replaying old ledgers.
//
// At some point in the future someone may wish to remove Amendment
// conditional code for Amendments that were enabled after January 2018.
// When that happens then the log message in Application.cpp should be
// updated.

class FeatureCollections
{
    struct Feature
    {
        std::string name;
        uint256 feature;
        std::size_t index;

        explicit Feature() = default;
        explicit Feature(
            std::string const& name_,
            uint256 const& feature_,
            std::size_t index_)
            : name(name_), feature(feature_), index(index_)
        {
        }
    };
    std::vector<Feature> features;
    boost::container::flat_map<uint256, std::size_t> featureToIndex;
    boost::container::flat_map<std::string, uint256> nameToFeature;
    std::map<std::string, DefaultVote> supported;
    std::size_t upVotes = 0;
    std::size_t downVotes = 0;
    mutable std::atomic<bool> readOnly = false;

public:
    FeatureCollections();

    std::optional<uint256>
    getRegisteredFeature(std::string const& name) const;

    uint256
    registerFeature(
        std::string const& name,
        Supported support,
        DefaultVote vote);

    std::size_t
    featureToBitsetIndex(uint256 const& f) const;

    uint256 const&
    bitsetIndexToFeature(size_t i) const;

    std::string
    featureToName(uint256 const& f) const;

    /** Amendments that this server supports.
    Whether they are enabled depends on the Rules defined in the validated
    ledger */
    std::map<std::string, DefaultVote> const&
    supportedAmendments() const
    {
        return supported;
    }

    /** Amendments that this server WON'T vote for by default. */
    std::size_t
    numDownVotedAmendments() const
    {
        return downVotes;
    }

    /** Amendments that this server WILL vote for by default. */
    std::size_t
    numUpVotedAmendments() const
    {
        return upVotes;
    }
};

}  // namespace detail

//------------------------------------------------------------------------------

detail::FeatureCollections::FeatureCollections()
{
    features.reserve(ripple::detail::numFeatures);
    featureToIndex.reserve(ripple::detail::numFeatures);
    nameToFeature.reserve(ripple::detail::numFeatures);
}

std::optional<uint256>
detail::FeatureCollections::getRegisteredFeature(std::string const& name) const
{
    readOnly = true;
    auto const i = nameToFeature.find(name);
    if (i == nameToFeature.end())
        return std::nullopt;
    return i->second;
}

uint256
detail::FeatureCollections::registerFeature(
    std::string const& name,
    Supported support,
    DefaultVote vote)
{
    assert(!readOnly);
    assert(support != Supported::no || vote != DefaultVote::yes);
    auto const i = nameToFeature.find(name);
    // Each feature should only be registered once
    assert(i == nameToFeature.end());
    if (i == nameToFeature.end())
    {
        // If this assertion fails, and you just added a feature, increase the
        // numFeatures value in Feature.h
        assert(features.size() < numFeatures);

        auto const f = sha512Half(Slice(name.data(), name.size()));
#if DEBUG
        {
            // remove this before publishing
            sha512_half_hasher h;
            h(name.data(), name.size());
            auto const fOld = static_cast<uint256>(h);
            assert(f == fOld);
        }
#endif

        auto const& feature = features.emplace_back(name, f, features.size());
        featureToIndex[f] = feature.index;
        nameToFeature[name] = f;

        assert(features.size() == featureToIndex.size());
        assert(features.size() == nameToFeature.size());

        assert(features[featureToIndex[f]].name == name);
        assert(features[featureToIndex[f]].feature == f);

        if (support == Supported::yes)
        {
            supported.emplace(name, vote);

            if (vote == DefaultVote::yes)
                ++upVotes;
            else
                ++downVotes;
        }
        assert(upVotes + downVotes == supported.size());
        assert(supported.size() <= features.size());
        return f;
    }
    else
        return i->second;
}

size_t
detail::FeatureCollections::featureToBitsetIndex(uint256 const& f) const
{
    readOnly = true;
    auto const i = featureToIndex.find(f);
    if (i == featureToIndex.end())
        LogicError("Invalid Feature ID");
    return i->second;
}

uint256 const&
detail::FeatureCollections::bitsetIndexToFeature(size_t i) const
{
    readOnly = true;
    if (i >= features.size())
        LogicError("Invalid FeatureBitset index");
    return features[i].feature;
}

std::string
detail::FeatureCollections::featureToName(uint256 const& f) const
{
    readOnly = true;
    auto const i = featureToIndex.find(f);
    return i == featureToIndex.end() ? to_string(f) : features[i->second].name;
}

static detail::FeatureCollections featureCollections;

/** Amendments that this server supports.
   Whether they are enabled depends on the Rules defined in the validated
   ledger */
std::map<std::string, DefaultVote> const&
detail::supportedAmendments()
{
    return featureCollections.supportedAmendments();
}

/** Amendments that this server won't vote for by default. */
std::size_t
detail::numDownVotedAmendments()
{
    return featureCollections.numDownVotedAmendments();
}

/** Amendments that this server will vote for by default. */
std::size_t
detail::numUpVotedAmendments()
{
    return featureCollections.numUpVotedAmendments();
}

//------------------------------------------------------------------------------

std::optional<uint256>
getRegisteredFeature(std::string const& name)
{
    return featureCollections.getRegisteredFeature(name);
}

uint256
registerFeature(std::string const& name, Supported support, DefaultVote vote)
{
    return featureCollections.registerFeature(name, support, vote);
}

size_t
featureToBitsetIndex(uint256 const& f)
{
    return featureCollections.featureToBitsetIndex(f);
}

uint256
bitsetIndexToFeature(size_t i)
{
    return featureCollections.bitsetIndexToFeature(i);
}

std::string
featureToName(uint256 const& f)
{
    return featureCollections.featureToName(f);
}

// clang-format off

// All known amendments must be registered either here or below with the
// "retired" amendments
uint256 const
    featureOwnerPaysFee             = registerFeature("OwnerPaysFee", Supported::no, DefaultVote::abstain),
    featureFlow                     = registerFeature("Flow", Supported::yes, DefaultVote::yes),
    featureCompareTakerFlowCross    = registerFeature("CompareTakerFlowCross", Supported::no, DefaultVote::abstain),
    featureFlowCross                = registerFeature("FlowCross", Supported::yes, DefaultVote::yes),
    featureCryptoConditionsSuite    = registerFeature("CryptoConditionsSuite", Supported::yes, DefaultVote::abstain),
    fix1513                         = registerFeature("fix1513", Supported::yes, DefaultVote::yes),
    featureDepositAuth              = registerFeature("DepositAuth", Supported::yes, DefaultVote::yes),
    featureChecks                   = registerFeature("Checks", Supported::yes, DefaultVote::yes),
    fix1571                         = registerFeature("fix1571", Supported::yes, DefaultVote::yes),
    fix1543                         = registerFeature("fix1543", Supported::yes, DefaultVote::yes),
    fix1623                         = registerFeature("fix1623", Supported::yes, DefaultVote::yes),
    featureDepositPreauth           = registerFeature("DepositPreauth", Supported::yes, DefaultVote::yes),
    // Use liquidity from strands that consume max offers, but mark as dry
    fix1515                         = registerFeature("fix1515", Supported::yes, DefaultVote::yes),
    fix1578                         = registerFeature("fix1578", Supported::yes, DefaultVote::yes),
    featureMultiSignReserve         = registerFeature("MultiSignReserve", Supported::yes, DefaultVote::yes),
    fixTakerDryOfferRemoval         = registerFeature("fixTakerDryOfferRemoval", Supported::yes, DefaultVote::yes),
    fixMasterKeyAsRegularKey        = registerFeature("fixMasterKeyAsRegularKey", Supported::yes, DefaultVote::yes),
    fixCheckThreading               = registerFeature("fixCheckThreading", Supported::yes, DefaultVote::yes),
    fixPayChanRecipientOwnerDir     = registerFeature("fixPayChanRecipientOwnerDir", Supported::yes, DefaultVote::yes),
    featureDeletableAccounts        = registerFeature("DeletableAccounts", Supported::yes, DefaultVote::yes),
    // fixQualityUpperBound should be activated before FlowCross
    fixQualityUpperBound            = registerFeature("fixQualityUpperBound", Supported::yes, DefaultVote::yes),
    featureRequireFullyCanonicalSig = registerFeature("RequireFullyCanonicalSig", Supported::yes, DefaultVote::yes),
    // fix1781: XRPEndpointSteps should be included in the circular payment check
    fix1781                         = registerFeature("fix1781", Supported::yes, DefaultVote::yes),
    featureHardenedValidations      = registerFeature("HardenedValidations", Supported::yes, DefaultVote::yes),
    fixAmendmentMajorityCalc        = registerFeature("fixAmendmentMajorityCalc", Supported::yes, DefaultVote::yes),
    featureNegativeUNL              = registerFeature("NegativeUNL", Supported::no, DefaultVote::abstain),
    featureTicketBatch              = registerFeature("TicketBatch", Supported::yes, DefaultVote::yes),
    featureFlowSortStrands          = registerFeature("FlowSortStrands", Supported::yes, DefaultVote::yes),
    fixSTAmountCanonicalize         = registerFeature("fixSTAmountCanonicalize", Supported::yes, DefaultVote::yes),
    fixRmSmallIncreasedQOffers      = registerFeature("fixRmSmallIncreasedQOffers", Supported::yes, DefaultVote::yes);

// The following amendments have been active for at least two years. Their
// pre-amendment code has been removed and the identifiers are deprecated.
// All known amendments and amendments that may appear in a validated
// ledger must be registered either here or above with the "active" amendments
[[deprecated("The referenced amendment has been retired"), maybe_unused]]
uint256 const
    retiredMultiSign         = registerFeature("MultiSign", Supported::yes, DefaultVote::abstain),
    retiredTrustSetAuth      = registerFeature("TrustSetAuth", Supported::yes, DefaultVote::abstain),
    retiredFeeEscalation     = registerFeature("FeeEscalation", Supported::yes, DefaultVote::abstain),
    retiredPayChan           = registerFeature("PayChan", Supported::yes, DefaultVote::abstain),
    retiredCryptoConditions  = registerFeature("CryptoConditions", Supported::yes, DefaultVote::abstain),
    retiredTickSize          = registerFeature("TickSize", Supported::yes, DefaultVote::abstain),
    retiredFix1368           = registerFeature("fix1368", Supported::yes, DefaultVote::abstain),
    retiredEscrow            = registerFeature("Escrow", Supported::yes, DefaultVote::abstain),
    retiredFix1373           = registerFeature("fix1373", Supported::yes, DefaultVote::abstain),
    retiredEnforceInvariants = registerFeature("EnforceInvariants", Supported::yes, DefaultVote::abstain),
    retiredSortedDirectories = registerFeature("SortedDirectories", Supported::yes, DefaultVote::abstain),
    retiredFix1201           = registerFeature("fix1201", Supported::yes, DefaultVote::abstain),
    retiredFix1512           = registerFeature("fix1512", Supported::yes, DefaultVote::abstain),
    retiredFix1523           = registerFeature("fix1523", Supported::yes, DefaultVote::abstain),
    retiredFix1528           = registerFeature("fix1528", Supported::yes, DefaultVote::abstain);

// clang-format on

}  // namespace ripple
