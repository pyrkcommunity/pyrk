// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"

#include <math.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, int algo) {

    if (!pindexLast || pindexLast->nHeight < params.v2DiffChangeHeight)
        return GetNextWorkRequiredV1(pindexLast, params, algo);
    else if (pindexLast->nHeight < params.v3DiffChangeHeight)
        return GetNextWorkRequiredV2(pindexLast, params, algo);
    else
        return GetNextWorkRequiredV3(pindexLast, params, algo);

}


unsigned int GetNextWorkRequiredV1(const CBlockIndex* pindexLast, const Consensus::Params& params, int algo) {
    unsigned int npowWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == nullptr)
        return npowWorkLimit;

    // find first block in averaging interval
    // Go back by what we want to be nAveragingInterval blocks per algo
    const CBlockIndex* pindexFirst = pindexLast;
    
    for (int i = 0; pindexFirst && i < NUM_ALGOS * params.nAveragingInterval; i++)
    {
        pindexFirst = pindexFirst->pprev;
    }

    const CBlockIndex* pindexPrevAlgo = GetLastBlockIndexForAlgo(pindexLast, params, algo);
    if (pindexPrevAlgo == nullptr || pindexFirst == nullptr || params.fPowNoRetargeting)
    {
        return npowWorkLimit;
    }

    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = pindexLast->GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    nActualTimespan = params.nAveragingTargetTimespan + (nActualTimespan - params.nAveragingTargetTimespan)/4;

    if (nActualTimespan < params.nMinActualTimespan)
        nActualTimespan = params.nMinActualTimespan;
    if (nActualTimespan > params.nMaxActualTimespan)
        nActualTimespan = params.nMaxActualTimespan;

    //Global retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrevAlgo->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.nAveragingTargetTimespan;

    //Per-algo retarget
    int nAdjustments = pindexPrevAlgo->nHeight + NUM_ALGOS - 1 - pindexLast->nHeight;
    if (nAdjustments > 0)
    {
        for (int i = 0; i < nAdjustments; i++)
        {
            bnNew *= 100;
            bnNew /= (100 + params.nLocalTargetAdjustment);
        }
    }
    else if (nAdjustments < 0)
    {
        for (int i = 0; i < -nAdjustments; i++)
        {
            bnNew *= (100 + params.nLocalTargetAdjustment);
            bnNew /= 100;
        }
    }

    if (bnNew > UintToArith256(params.powLimit))
    {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredV2(const CBlockIndex* pindexLast, const Consensus::Params& params, int algo) {
    unsigned int npowWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == nullptr)
        return npowWorkLimit;

    // find first block in averaging interval
    // Go back by what we want to be nAveragingInterval blocks per algo
    const CBlockIndex* pindexFirst = pindexLast;
    for (int i = 0; pindexFirst && i < NUM_ALGOSV2 * params.nAveragingInterval; i++)
    {
        pindexFirst = pindexFirst->pprev;
    }

    const CBlockIndex* pindexPrevAlgo = GetLastBlockIndexForAlgo(pindexLast, params, algo);
    if (pindexPrevAlgo == nullptr || pindexFirst == nullptr || params.fPowNoRetargeting)
    {
        return npowWorkLimit;
    }

    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = pindexLast->GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    nActualTimespan = params.nAveragingTargetTimespanV2 + (nActualTimespan - params.nAveragingTargetTimespanV2)/4;

    if (nActualTimespan < params.nMinActualTimespanV2)
        nActualTimespan = params.nMinActualTimespanV2;
    if (nActualTimespan > params.nMaxActualTimespanV2)
        nActualTimespan = params.nMaxActualTimespanV2;

    //Global retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrevAlgo->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.nAveragingTargetTimespanV2;

    //Per-algo retarget
    int nAdjustments = pindexPrevAlgo->nHeight + NUM_ALGOSV2 - 1 - pindexLast->nHeight;
    if (nAdjustments > 0)
    {
        for (int i = 0; i < nAdjustments; i++)
        {
            bnNew *= 100;
            bnNew /= (100 + params.nLocalTargetAdjustment);
        }
    }
    else if (nAdjustments < 0)
    {
        for (int i = 0; i < -nAdjustments; i++)
        {
            bnNew *= (100 + params.nLocalTargetAdjustment);
            bnNew /= 100;
        }
    }

    if (bnNew > UintToArith256(params.powLimit))
    {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}

unsigned int GetNextWorkRequiredV3(const CBlockIndex* pindexLast, const Consensus::Params& params, int algo) {
    unsigned int npowWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == nullptr)
        return npowWorkLimit;

    // find first block in averaging interval
    // Go back by what we want to be nAveragingInterval blocks per algo
    const CBlockIndex* pindexFirst = pindexLast;
    if (pindexLast->nHeight < params.AlgoChangeHeight) {
        for (int i = 0; pindexFirst && i < NUM_ALGOSV2 * params.nAveragingInterval; i++)
        {
            pindexFirst = pindexFirst->pprev;
        }
    } else {
        for (int i = 0; pindexFirst && i < NUM_ALGOSV3 * params.nAveragingInterval; i++)
        {
            pindexFirst = pindexFirst->pprev;
        }
    }

    const CBlockIndex* pindexPrevAlgo = GetLastBlockIndexForAlgo(pindexLast, params, algo);
    if (pindexPrevAlgo == nullptr || pindexFirst == nullptr || params.fPowNoRetargeting)
    {
        return npowWorkLimit;
    }

    // Limit adjustment step
    // Use medians to prevent time-warp attacks
    int64_t nActualTimespan = pindexLast->GetMedianTimePast() - pindexFirst->GetMedianTimePast();
    nActualTimespan = params.nAveragingTargetTimespanV2 + (nActualTimespan - params.nAveragingTargetTimespanV2)/4;

    if (nActualTimespan < params.nMinActualTimespanV3)
        nActualTimespan = params.nMinActualTimespanV3;
    if (nActualTimespan > params.nMaxActualTimespanV3)
        nActualTimespan = params.nMaxActualTimespanV3;

    //Global retarget
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrevAlgo->nBits);

    bnNew *= nActualTimespan;
    bnNew /= params.nAveragingTargetTimespanV2;

    //Per-algo retarget
    int nAdjustments{0};
    if (pindexLast->nHeight < params.AlgoChangeHeight) {
        nAdjustments = pindexPrevAlgo->nHeight + NUM_ALGOSV2 - 1 - pindexLast->nHeight;
    } else {
        nAdjustments = pindexPrevAlgo->nHeight + NUM_ALGOSV3 - 1 - pindexLast->nHeight;
    }

    if (nAdjustments > 0)
    {
        for (int i = 0; i < nAdjustments; i++)
        {
            bnNew *= 100;
            bnNew /= (100 + params.nLocalTargetAdjustment);
        }
    }
    else if (nAdjustments < 0)
    {
        for (int i = 0; i < -nAdjustments; i++)
        {
            bnNew *= (100 + params.nLocalTargetAdjustment);
            bnNew /= 100;
        }
    }

    if (bnNew > UintToArith256(params.powLimit))
    {
        bnNew = UintToArith256(params.powLimit);
    }

    return bnNew.GetCompact();
}



bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, const Consensus::Params& params, int algo)
{
    for (; pindex; pindex = pindex->pprev)
    {
        if (pindex->GetAlgo() != algo)
            continue;
        // ignore special min-difficulty testnet blocks
        if (params.fPowAllowMinDifficultyBlocks &&
            pindex->pprev &&
            pindex->nTime > pindex->pprev->nTime + params.nPowTargetSpacing*6)
        {
            continue;
        }
        return pindex;
    }
    return nullptr;
}
