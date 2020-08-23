// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POW_H
#define BITCOIN_POW_H

#include "consensus/params.h"

#include <stdint.h>

class CBlockHeader;
class CBlockIndex;
class uint256;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params&, int algo);

unsigned int GetNextWorkRequiredV1(const CBlockIndex* pindexLast, const Consensus::Params&, int algo);

unsigned int GetNextWorkRequiredV2(const CBlockIndex* pindexLast, const Consensus::Params&, int algo);

unsigned int GetNextWorkRequiredV3(const CBlockIndex* pindexLast, const Consensus::Params& params, int algo);

/** Check whether a block hash satisfies the proof-of-work requirement specified by nBits */
bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params&);

const CBlockIndex* GetLastBlockIndexForAlgo(const CBlockIndex* pindex, const Consensus::Params&, int algo);

#endif // BITCOIN_POW_H
