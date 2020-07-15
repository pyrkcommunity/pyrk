#ifndef TOKENUTIL_H
#define TOKENUTIL_H

#include "amount.h"

#include <set>

class CBitcoinAddress;
struct CMutableTransaction;
class uint256;

bool selectInput(std::set<CBitcoinAddress>& setAddress, uint256& inputtxid, int& inputvout, CAmount& inputvalue, CAmount selectvalue);
bool sendTokenTransaction(CMutableTransaction &rawTx);

#endif // TOKENUTIL_H
