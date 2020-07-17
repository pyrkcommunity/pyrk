#include "tokenformnewownerentry.h"
#include "ui_tokenformentry.h"

#include "base58.h"
#include "guiutil.h"
#include "net.h"
#include "pyrktokens.h"
#include "primitives/transaction.h"
#include "qvalidatedlineedit.h"
#include "tokenutil.h"
#include "utilstrencodings.h"
#include "wallet/rpcwallet.h"
#include "walletmodel.h"

#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormNewOwnerEntry::TokenFormNewOwnerEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addNewOwnerEntry();
}

TokenFormNewOwnerEntry::~TokenFormNewOwnerEntry()
{
    delete ui;
}

void TokenFormNewOwnerEntry::addNewOwnerEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    QLabel *labelNewOwner = new QLabel(this);
    labelNewOwner->setText("New Owner Address");

    QValidatedLineEdit* editAddress = new QValidatedLineEdit(this);
    editAddress->setObjectName("edit_owner_address");
    GUIUtil::setupAddressWidget(editAddress, this);
    editAddress->setMaxLength(34);
    editAddress->setFixedWidth(240);

    QPushButton* newOwnerButton = new QPushButton(this);
    newOwnerButton->setText("Reassign Ownership");

    QSpacerItem *newOwnerSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    baseLayout->addWidget(labelNewOwner);
    baseLayout->addWidget(editAddress);
    baseLayout->addWidget(newOwnerButton);
    baseLayout->addSpacerItem(newOwnerSpacer);

    connect(newOwnerButton, SIGNAL(clicked()), this, SLOT(createNewOwnerClicked()));
}

void TokenFormNewOwnerEntry::createNewOwnerClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    if (!g_connman) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Peer-to-peer functionality missing or disabled."));
        return;
    }

    QLabel* tokenidObj = parent->findChild<QLabel*>("label_token_id_value_2");
    QLineEdit* addressObj = findChild<QLineEdit*>("edit_owner_address");

    std::string tokenid, raddress;

    if (tokenidObj && addressObj) {
        tokenid = tokenidObj->text().toStdString();
        raddress = addressObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (tokenid.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token ID empty, select token from token list."));
        return;
    }

    CBitcoinAddress addressRecipient(raddress);
    if (raddress.empty() || !addressRecipient.IsValid()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Address empty or invalid."));
        return;
    }

    std::string addresshex = string_to_hex(raddress);
    std::string opreturncommand = "34320107";

    padTo(addresshex, 34);

    std::string opreturndata = opreturncommand + tokenid + addresshex;

    std::string currenttokenaddress;
    if (!parent->tokenAddress(currenttokenaddress)) {
        // tokenAddress shows errors to user on failure
        return;
    }

    if (currenttokenaddress == addressRecipient.ToString()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Destination token address cannot be the same as the source token address."));
        return;
    }

    std::set<CBitcoinAddress> setAddress;
    CBitcoinAddress address(currenttokenaddress);
    setAddress.insert(address);

    uint256 inputtxid;
    int inputvout;
    CAmount inputvalue;
    CAmount valueoutdust = 1000;

    // Require input to cover 1000 fee and 1000 as min to return to sender or we could end up
    // with a less than dust or negative output resulting in a non-standard TX that will be rejected.
    if (!selectInput(setAddress, inputtxid, inputvout, inputvalue, valueoutdust + 1000 + 1000)) {
        // Error message sent to user inside selectInput call
        return;
    }

    CMutableTransaction rawTx;
    CTxIn in(COutPoint(inputtxid, inputvout), CScript(), std::numeric_limits<uint32_t>::max());
    rawTx.vin.push_back(in);

    // Token data
    CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
    rawTx.vout.push_back(out);

    // Return funds to sender
    CScript scriptPubKeySender = GetScriptForDestination(address.Get());
    CAmount valueout = inputvalue - valueoutdust - 1000; // 1000 Sat fee
    CTxOut outSender(valueout, scriptPubKeySender);
    rawTx.vout.push_back(outSender);

    // Dust to recipient
    CScript scriptPubKeyRecipient = GetScriptForDestination(addressRecipient.Get());
    CTxOut outRecipient(valueoutdust, scriptPubKeyRecipient);
    rawTx.vout.push_back(outRecipient);

    if (sendTokenTransaction(rawTx)) {
        clear();
    }
}

void TokenFormNewOwnerEntry::clear()
{
    QLineEdit* ownerObj = findChild<QLineEdit*>("edit_owner_address");

    if (ownerObj) {
        ownerObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
