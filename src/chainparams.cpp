// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "Cointelegraph 26/03/2020 Mt. Gox Trustee Reveals Draft Rehabilitation Plan";
    const CScript genesisOutputScript = CScript() << ParseHex("04ecc8faa3bb4fef51fd57145c25cd6d492e69412ad7c1722fadbcabbc1497617d32d1f49557634bdc4286293c479332426effbe2b040da1b7a569ce469d213ff1") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 100000;
        consensus.nMasternodePaymentsStartBlock = 10100;
        consensus.nInstantSendConfirmationsRequired = 6;
        consensus.nInstantSendKeepLock = 24;
        consensus.nSuperblockStartBlock = 25000;
        consensus.nSuperblockCycle = 29200;
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.nCollateralChangeHeight = 100000;
        consensus.BIP34Height = 16;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.DIP0001Height = 2;
        consensus.CSVHeight = 1;
        consensus.BIP147Height = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetSpacing = 90; // 1.5 minutes
        consensus.nAveragingInterval = 10; // 10 blocks
        consensus.multiAlgoTargetSpacing = 90 * 3; // NUM_ALGOS * 90 seconds
        consensus.multiAlgoTargetSpacingV2 = 90 * 4; // add yespower
        consensus.nAveragingTargetTimespan = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacing; // 10 * NUM_ALGOS * 90
        consensus.nAveragingTargetTimespanV2 = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacingV2; // 10 * NUM_ALGOS * 90
        consensus.nMaxAdjustDown = 16; // 16% adjustment down
        consensus.nMaxAdjustUp = 8; // 8% adjustment up
        consensus.nMaxAdjustDownV2 = 12; // 12% adjustment down -- lower a little slower
        consensus.nMaxAdjustUpV2 = 10; // 10% adjustment up  -- rise a little faster
        consensus.nMinActualTimespan = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUp) / 100;
        consensus.nMaxActualTimespan = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDown) / 100;
        consensus.nMinActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUpV2) / 100;
        consensus.nMaxActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDownV2) / 100;
        consensus.v2DiffChangeHeight = 100000;
        consensus.nLocalTargetAdjustment = 4; //target adjustment per algo
        consensus.nLocalDifficultyAdjustment = 4; //difficulty adjustment per algo
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 17920;
        consensus.nMinerConfirmationWindow = 13440;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xb0;
        pchMessageStart[1] = 0x0d;
        pchMessageStart[2] = 0x6c;
        pchMessageStart[3] = 0xbe;
        nDefaultPort = 8118;
        nPruneAfterHeight = 100000;

        genesis = CreateGenesisBlock(1585180800, 2875050, 0x1e0ffff0, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x973814a07c1ae4f3af90372952c9b9709901a95df1d0ea54bd1b3bd6feff5b89"));
        assert(genesis.hashMerkleRoot == uint256S("0xfaa5e3a6b332d17e8d9532839ff8cf89732e9a52e1a75ab3c4a3eaccf35e7251"));

        vSeeds.push_back(CDNSSeedData("peer1.pyrk.net", "peer1.pyrk.net"));
        vSeeds.push_back(CDNSSeedData("peer2.pyrk.net", "peer2.pyrk.net"));

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,55);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,16);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,183);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // BIP44 coin type is '5'
        nExtCoinType = 5;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        strSporkAddress = "PXhXTyumkiWqRoJ4cT7vUJxGWh1RwkULDU";

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (  0, uint256S("0x973814a07c1ae4f3af90372952c9b9709901a95df1d0ea54bd1b3bd6feff5b89"))
            (  5000, uint256S("0x00000000000000028b9635facde03c52f86280340ddb7de23ec2d70847602a23"))
            (  10000, uint256S("0xd378053edc554b14166acfa36bfaaf71c57f014d7698585d7a5c2f3c0c2624b9"))
            (  15000, uint256S("0x2df0a91058d731b42025004658e36648b24745794eab4de0bae547770a4eb2d2"))
            (  25000, uint256S("0x000000000000000feb054686168fafb67897a5766c1ffeafed713889e0f18728"))
            (  40000, uint256S("0x0000000000000011d127f197d185b5771933ea345e83ebb9ddd3e2cd68ac25df"))
            (  47000, uint256S("0xdad5bf16bbc6e980d7cb54762f57a2e445485dbd030e084fe27d9c1c35846918"))
            (  48000, uint256S("0x1c4f8bf00540c230d4f7b69f0a2b0e0c3dc8d8811e30f54948fc3436705a1878"))
            (  48250, uint256S("0x00000000000000045ab0543cad7ffef7aa884938f1b511d18c4a7415fd53bf47"))
            (  48322, uint256S("0x00000000000000012e4b46b6b404a6dd77dac362697042766efd69bdc074f3ac"))

        };

        chainTxData = ChainTxData{
            1585180800, // * UNIX timestamp of last known number of transactions
            1,    // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.01         // * estimated number of transactions per second after that timestamp
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 100000;
        consensus.nMasternodePaymentsStartBlock = 4010;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nSuperblockStartBlock = 4010;
        consensus.nSuperblockCycle = 40; // Superblocks can be issued hourly on testnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nCollateralChangeHeight = 100000;
        consensus.BIP34Height = 76;
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1;
        consensus.BIP66Height = 1;
        consensus.DIP0001Height = 2;
        consensus.CSVHeight = 1;
        consensus.BIP147Height = 1;
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 20
        consensus.nPowTargetSpacing = 90; // 1.5 minutes
        consensus.nAveragingInterval = 10; // 10 blocks
        consensus.multiAlgoTargetSpacing = 90 * 4; //3; // NUM_ALGOS * 90 seconds
        consensus.multiAlgoTargetSpacingV2 = 90 * 4; // add yespower
        consensus.nAveragingTargetTimespan = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacing; // 10 * NUM_ALGOS * 90
        consensus.nAveragingTargetTimespanV2 = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacingV2; // 10 * NUM_ALGOS * 90
        consensus.nMaxAdjustDown = 16; // 16% adjustment down
        consensus.nMaxAdjustUp = 8; // 8% adjustment up
        consensus.nMaxAdjustDownV2 = 12; // 12% adjustment down -- lower a little slower
        consensus.nMaxAdjustUpV2 = 10; // 10% adjustment up  -- rise a little faster
        consensus.nMinActualTimespan = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUp) / 100;
        consensus.nMaxActualTimespan = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDown) / 100;
        consensus.nMinActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUpV2) / 100;
        consensus.nMaxActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDownV2) / 100;
        consensus.v2DiffChangeHeight = 100000;
        consensus.nLocalTargetAdjustment = 4; //target adjustment per algo
        consensus.nLocalDifficultyAdjustment = 4; //difficulty adjustment per algo
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 17920; // 75% for testchains
        consensus.nMinerConfirmationWindow = 13440;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008
        
        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xcf;
        pchMessageStart[1] = 0xe3;
        pchMessageStart[2] = 0xcb;
        pchMessageStart[3] = 0xf0;
        nDefaultPort = 18117;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1585180810, 988216, 0x1e0ffff0, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x6015f9422d514ebd62d28c48b7faf86738680091d38ecb2c9bc2381b9325c13a"));
        assert(genesis.hashMerkleRoot == uint256S("0xfaa5e3a6b332d17e8d9532839ff8cf89732e9a52e1a75ab3c4a3eaccf35e7251"));

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        strSporkAddress = "yUZqqyH9zT5MJtxRm7dumbc9E3QwuE4haA";

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (    0, uint256S("0x6015f9422d514ebd62d28c48b7faf86738680091d38ecb2c9bc2381b9325c13a"))
        };

        chainTxData = ChainTxData{
            1585180810, // * UNIX timestamp of last known number of transactions
            1,          // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.01        // * estimated number of transactions per second after that timestamp
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";
        consensus.nSubsidyHalvingInterval = 100000;
        consensus.nMasternodePaymentsStartBlock = 4010;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nSuperblockStartBlock = 4010;
        consensus.nSuperblockCycle = 40; // Superblocks can be issued hourly on devnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nCollateralChangeHeight = 100000;
        consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.CSVHeight = 1; // CSV activated immediately on devnet
        consensus.BIP147Height = 1; // BIP147 activated immediately on devnet
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetSpacing = 90; // 1.5 minutes
        consensus.nAveragingInterval = 10; // 10 blocks
        consensus.multiAlgoTargetSpacing = 90 * 3; //3; // NUM_ALGOS * 90 seconds
        consensus.multiAlgoTargetSpacingV2 = 90 * 4; // add yespower
        consensus.nAveragingTargetTimespan = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacing; // 10 * NUM_ALGOS * 90
        consensus.nAveragingTargetTimespanV2 = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacingV2; // 10 * NUM_ALGOS * 90
        consensus.nMaxAdjustDown = 16; // 16% adjustment down
        consensus.nMaxAdjustUp = 8; // 8% adjustment up
        consensus.nMaxAdjustDownV2 = 12; // 12% adjustment down -- lower a little slower
        consensus.nMaxAdjustUpV2 = 10; // 10% adjustment up  -- rise a little faster
        consensus.nMinActualTimespan = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUp) / 100;
        consensus.nMaxActualTimespan = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDown) / 100;
        consensus.nMinActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUpV2) / 100;
        consensus.nMaxActualTimespanV2 = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDownV2) / 100;
        consensus.v2DiffChangeHeight = 100000;
        consensus.nLocalTargetAdjustment = 4; //target adjustment per algo
        consensus.nLocalDifficultyAdjustment = 4; //difficulty adjustment per algo
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 17920; // 75% for testchains
        consensus.nMinerConfirmationWindow = 13440;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xe3;
        pchMessageStart[1] = 0xcb;
        pchMessageStart[2] = 0xf0;
        pchMessageStart[3] = 0xcf;
        nDefaultPort = 18118;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1585180820, 0, 0x207fffff, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x9ee0e8d7f651e2f2870d96b73f9e6176951ea1c3daebf8fd72bd96beb4225127"));
        assert(genesis.hashMerkleRoot == uint256S("0xfaa5e3a6b332d17e8d9532839ff8cf89732e9a52e1a75ab3c4a3eaccf35e7251"));

        devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 100 * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        strSporkAddress = "yUZqqyH9zT5MJtxRm7dumbc9E3QwuE4haA";

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (      0, uint256S("0x9ee0e8d7f651e2f2870d96b73f9e6176951ea1c3daebf8fd72bd96beb4225127"))
            (      1, devnetGenesis.GetHash())
        };

        chainTxData = ChainTxData{
            devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
            2,                            // * we only have 2 coinbase transactions when a devnet is started up
            0.01                          // * estimated number of transactions per second
        };
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 2000;
        consensus.nMasternodePaymentsStartBlock = 240;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nSuperblockStartBlock = 340;
        consensus.nSuperblockCycle = 40;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.nCollateralChangeHeight = 1000;
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP34Hash = uint256();
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.DIP0001Height = 2000;
        consensus.CSVHeight = 1;
        consensus.BIP147Height = 1;
        consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.nPowTargetSpacing = 90; // 1.5 minutes
        consensus.nAveragingInterval = 10; // 10 blocks
        consensus.multiAlgoTargetSpacing = 90 * 3; // NUM_ALGOS * 90 seconds
        consensus.nAveragingTargetTimespan = consensus.nAveragingInterval * consensus.multiAlgoTargetSpacing; // 10 * NUM_ALGOS * 90
        consensus.nMaxAdjustDown = 16; // 16% adjustment down
        consensus.nMaxAdjustUp = 8; // 8% adjustment up
        consensus.nMinActualTimespan = consensus.nAveragingTargetTimespan * (100 - consensus.nMaxAdjustUp) / 100;
        consensus.nMaxActualTimespan = consensus.nAveragingTargetTimespan * (100 + consensus.nMaxAdjustDown) / 100;
        consensus.nLocalTargetAdjustment = 4; // target adjustment per algo
        consensus.nLocalDifficultyAdjustment = 4; // difficulty adjustment per algo
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;
        


        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfd;
        pchMessageStart[1] = 0xc2;
        pchMessageStart[2] = 0xb8;
        pchMessageStart[3] = 0xdd;
        nDefaultPort = 19994;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1585180830, 2, 0x207fffff, 1, 100 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x14d20f12aae2274086bc6076d143324c959d9db8dff2f1d804150b8299ea2361"));
        assert(genesis.hashMerkleRoot == uint256S("0xfaa5e3a6b332d17e8d9532839ff8cf89732e9a52e1a75ab3c4a3eaccf35e7251"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
        strSporkAddress = "yUZqqyH9zT5MJtxRm7dumbc9E3QwuE4haA";

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x14d20f12aae2274086bc6076d143324c959d9db8dff2f1d804150b8299ea2361"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Regtest BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;
   }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::DEVNET) {
            assert(devNetParams);
            return *devNetParams;
    } else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    if (network == CBaseChainParams::DEVNET) {
        devNetParams = new CDevNetParams();
    }

    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
