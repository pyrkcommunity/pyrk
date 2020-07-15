#include "tokenformauthmetaentry.h"
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

#include <nlohmann/json.hpp>

#include <QDoubleValidator>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormAuthMetaEntry::TokenFormAuthMetaEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addAuthMetaEntry();
}

TokenFormAuthMetaEntry::~TokenFormAuthMetaEntry()
{
    delete ui;
}

void TokenFormAuthMetaEntry::addAuthMetaEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    QLabel *labelAddress = new QLabel(this);
    labelAddress->setText("Authorize Address");

    QValidatedLineEdit* editAddress = new QValidatedLineEdit(this);
    editAddress->setObjectName("edit_auth_address");
    GUIUtil::setupAddressWidget(editAddress, this);
    editAddress->setMaxLength(34);
    editAddress->setFixedWidth(240);

    QPushButton* authButton = new QPushButton(this);
    authButton->setText("Authorize Meta");

    QSpacerItem *authSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    baseLayout->addWidget(labelAddress);
    baseLayout->addWidget(editAddress);
    baseLayout->addWidget(authButton);
    baseLayout->addSpacerItem(authSpacer);

    connect(authButton, SIGNAL(clicked()), this, SLOT(createAuthMetaClicked()));
}

void TokenFormAuthMetaEntry::createAuthMetaClicked()
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

    QLineEdit* addressObj = findChild<QLineEdit*>("edit_auth_address");
    QLabel* tokenidObj = parent->findChild<QLabel*>("label_token_id_value_2");

    std::string raddress, tokenid;

    if (tokenidObj && addressObj) {
        raddress = addressObj->text().toStdString();
        tokenid = tokenidObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    CBitcoinAddress addressRecipient(raddress);
    if (raddress.empty() || !addressRecipient.IsValid()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Address empty or invalid."));
        return;
    }

    std::string addresshex = string_to_hex(raddress);

    padTo(addresshex, 34);

    std::string opreturncommand = "34320108";
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

    // Return funds to sender
    CScript scriptPubKeySender = GetScriptForDestination(address.Get());
    CAmount valueout = inputvalue - valueoutdust - 1000; // 1000 Sat fee
    CTxOut outSender(valueout, scriptPubKeySender);
    rawTx.vout.push_back(outSender);

    // Token data
    CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
    rawTx.vout.push_back(out);

    // Dust to recipient
    CScript scriptPubKeyRecipient = GetScriptForDestination(addressRecipient.Get());
    CTxOut outRecipient(valueoutdust, scriptPubKeyRecipient);
    rawTx.vout.push_back(outRecipient);

    if (sendTokenTransaction(rawTx)) {
        clear();
    }
}

void TokenFormAuthMetaEntry::clear()
{
    QLineEdit* authObj = findChild<QLineEdit*>("edit_auth_address");

    if (authObj) {
        authObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
