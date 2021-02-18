// Copyright (c) 2021 barrystyle
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PYRK_COLLATERAL_H
#define PYRK_COLLATERAL_H

#include <chainparams.h>
#include <util.h>
#include <txmempool.h>
#include <validation.h>

bool validateCollateral(COutPoint& collateral);
bool validateCollateral(COutPoint& collateral, const CBlockIndex* pindex);

#endif // PYRK_COLLATERAL_H
