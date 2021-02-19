// Copyright (c) 2021 barrystyle
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <collateral.h>

bool validateCollateral(COutPoint& collateral)
{
    const CBlockIndex* pindex = chainActive.Tip();
    return validateCollateral(collateral, pindex);
}

bool validateCollateral(COutPoint& collateral, const CBlockIndex* pindex)
{
    LOCK(cs_main);
    LOCK(mempool.cs);

    {
        Coin coin;
        const int height = pindex->nHeight;
        CCoinsViewMemPool viewMemPool(pcoinsTip, mempool);

        // invalid
        if (!viewMemPool.GetCoin(collateral, coin)) {
            LogPrintf("%s - outpoint %s invalid\n", __func__, collateral.ToString());
            return false;
        }

        // spent
        if (coin.IsSpent()) {
            LogPrintf("%s - outpoint %s has already been spent\n", __func__, collateral.ToString());
            return false;
        }

        // value
        CAmount collatValue = coin.out.nValue;
        if (collatValue != 1000 * COIN && collatValue != 2500 * COIN && collatValue != 5000 * COIN) {
            LogPrintf("%s - outpoint %s isnt any valid collateral amount (was %llu)\n", __func__, collateral.ToString(), collatValue/COIN);
            return false;
        }

        // height
        if (height < 100000) {
            if (collatValue == 1000 * COIN) {
                LogPrintf("under height 100000, collat 1000 is ok\n");
                return true;
            }
        } else if (height >= 100000 && height < 265000) {
            if (collatValue == 2500 * COIN) {
                LogPrintf("above height 100000 but under height 265000, collat 2500 is ok\n");
                return true;
            }
        } else if (height >= 265000 && height < 285000) {
            if (collatValue == 2500 * COIN || collatValue == 5000 * COIN) {
                LogPrintf("above height 265000 but under height 285000, collat 2500 or 5000 is ok\n");
                return true;
            }
        } else if (height >= 285000) {
            if (collatValue == 5000 * COIN) {
                LogPrintf("above 285000 only 5000 collat is ok\n");
                return true;
            }
        }

        // if we got here, grab a beer
        LogPrintf("got to the end (debug: collateral(%s) amount(%llu) spent(%d))\n", collateral.ToString(), collatValue, coin.IsSpent());
        return false;
    }
}
