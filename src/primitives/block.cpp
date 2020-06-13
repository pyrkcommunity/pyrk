// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "arith_uint256.h"
#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"
#include "crypto/scrypt.h"
#include "crypto/algos/yespower/yespower.h"

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

int CBlockHeader::GetAlgo() const
{
    switch (nVersion & BLOCK_VERSION_ALGO)
    {
        case BLOCK_VERSION_SCRYPT:
            return ALGO_SCRYPT;
        case BLOCK_VERSION_SHA256D:
            return ALGO_SHA256D;
        case BLOCK_VERSION_X11:
            return ALGO_X11;
        case BLOCK_VERSION_YESPOWER:
            return ALGO_YESPOWER;
            
    }
    return ALGO_UNKNOWN;
}

uint256 CBlockHeader::GetPoWAlgoHash() const
{
    switch (GetAlgo())
    {
        case ALGO_SHA256D:
        {
            return GetHash();
        }
        case ALGO_SCRYPT:
        {
            uint256 thash;
            scrypt_1024_1_1_256(BEGIN(nVersion), BEGIN(thash));
            return thash;
        }
        case ALGO_X11:
        {
            return HashX11(BEGIN(nVersion), END(nNonce));
        }
        case ALGO_YESPOWER:
        {
            uint256 thash;
            yespower_hash(BEGIN(nVersion), BEGIN(thash));
            return thash;
        }
        case ALGO_UNKNOWN:
            return ArithToUint256(~arith_uint256(0));
    }
    return GetHash();
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, pow_algo=%d, pow_hash=%s, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        GetAlgo(),
        GetPoWAlgoHash().ToString(),
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i]->ToString() << "\n";
    }
    return s.str();
}

std::string GetAlgoName(int Algo)
{
    switch (Algo)
    {
        case ALGO_SHA256D:
            return std::string("sha256d");
        case ALGO_SCRYPT:
            return std::string("scrypt");
        case ALGO_X11:
            return std::string("x11");
        case ALGO_YESPOWER:
            return std::string("yespower");
    }
    return std::string("unknown");
}
