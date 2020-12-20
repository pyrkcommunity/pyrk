#include "tokenformaddmetaentry.h"
#include "ui_tokenformentry.h"

#include "base58.h"
#include "net.h"
#include "pyrktokens.h"
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
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>

TokenFormAddMetaEntry::TokenFormAddMetaEntry(PyrkTokens *parent, WalletModel *model) :
    QWidget(nullptr),
    parent(parent),
    ui(new Ui::TokenFormEntry),
    walletModel(model)
{
    ui->setupUi(this);

    addAddMetaEntry();
}

TokenFormAddMetaEntry::~TokenFormAddMetaEntry()
{
    delete ui;
}

void TokenFormAddMetaEntry::addAddMetaEntry()
{
    QHBoxLayout* baseLayout = new QHBoxLayout();
    setLayout(baseLayout);

    // Add titles for first line of each row, looks tidier!
    QVBoxLayout* titlesLayout = new QVBoxLayout();

    QLabel *labelCode = new QLabel(this);
    labelCode->setText("Meta Code");

    QLabel *labelSpacer = new QLabel(this); // Spacer
    labelSpacer->setText("");

    titlesLayout->addWidget(labelCode);
    titlesLayout->addWidget(labelSpacer);

    // Rest of the form goes here
    QVBoxLayout* formLayout = new QVBoxLayout();

    // Meta Code / Meta Data
    QHBoxLayout* codeLayout = new QHBoxLayout();
    codeLayout->setSpacing(5);

    QValidatedLineEdit* editCode = new QValidatedLineEdit(this);
    editCode->setObjectName("edit_meta_code");
    editCode->setMaxLength(8);
    editCode->setFixedWidth(70);
    QDoubleValidator* validator = new QDoubleValidator(1, 4294967295, 0, this);
    validator->setNotation(QDoubleValidator::StandardNotation);
    editCode->setCheckValidator(validator);

    QLabel *labelData = new QLabel(this);
    labelData->setText("Meta Data");

    QLineEdit* editData = new QLineEdit(this);
    editData->setObjectName("edit_meta_data");
    editData->setMaxLength(130);
    //editData->setFixedWidth(200);

    codeLayout->addWidget(editCode);
    codeLayout->addWidget(labelData);
    codeLayout->addWidget(editData);

    // Add Meta button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(5);

    QSpacerItem *sendSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);

    QPushButton* addMetaButton = new QPushButton(this);
    addMetaButton->setText("Post Meta");

    addMetaButton->setStyleSheet(QString::fromUtf8("QPushButton{border:1px solid rgb(255,255,255); border-radius:4px;}"));

    buttonLayout->addSpacerItem(sendSpacer);
    buttonLayout->addWidget(addMetaButton);

    formLayout->addLayout(codeLayout);
    formLayout->addLayout(buttonLayout);

    baseLayout->addLayout(titlesLayout);
    baseLayout->addLayout(formLayout);

    connect(addMetaButton, SIGNAL(clicked()), this, SLOT(createAddMetaClicked()));
}

void TokenFormAddMetaEntry::createAddMetaClicked()
{
    WalletModel::UnlockContext ctx(walletModel->requestUnlock());
    if (!ctx.isValid())
    {
        return;
    }

    QValidatedLineEdit* edit_meta_code = findChild<QValidatedLineEdit*>("edit_meta_code");
    if (edit_meta_code) {
        if (!edit_meta_code->isValid()) {
            QMessageBox::critical(nullptr, tr("Error"),
                tr("Amount cannot be more than 4,294,967,295."));
            return;
        }
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (!g_connman) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Peer-to-peer functionality missing or disabled."));
        return;
    }

    QLabel* tokenidObj = parent->findChild<QLabel*>("label_token_id_value_2");
    QLineEdit* metaCodeObj = findChild<QLineEdit*>("edit_meta_code");
    QLineEdit* metaDataObj = findChild<QLineEdit*>("edit_meta_data");

    std::string metaCode, metaData, tokenid;

    if (tokenidObj && metaCodeObj && metaDataObj) {
        metaCode = metaCodeObj->text().toStdString();
        metaData = metaDataObj->text().toStdString();
        tokenid = tokenidObj->text().toStdString();
    } else {
        return; // Could not find Qt Object, should not happen, maybe a form item was renamed?
    }

    if (metaCode.empty() || std::stold(metaCode) <= 0) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Meta code cannot be empty or less than 1."));
        return;
    }

    if (metaData.empty()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Meta data cannot be empty."));
        return;
    }

    std::string metacodehex = llint_to_hex((uint64_t)std::stold(metaCode));
    std::string metadatahex = string_to_hex(metaData);
    std::string opreturncommand = "34320102";

    padTo(metacodehex, 8);

    std::string opreturndata = opreturncommand + metacodehex + tokenid + metadatahex;

    std::string currenttokenaddress;
    if (!parent->tokenAddress(currenttokenaddress)) {
        // tokenAddress shows errors to user on failure
        return;
    }

    std::ostringstream oss;
    if(CURLE_OK == curl_read(GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "ismetaauth/" + tokenid + "/" + currenttokenaddress, oss))
    {
        auto json = nlohmann::json::parse(oss.str());

        bool isMetaAuth = false;

        if (json.contains("isMetaAuth")) {
            isMetaAuth = json["isMetaAuth"];
        }

        if (isMetaAuth) {
            std::set<CBitcoinAddress> setAddress;
            CBitcoinAddress address(currenttokenaddress);
            setAddress.insert(address);

            uint256 inputtxid;
            int inputvout;
            CAmount inputvalue;

            // Require input to cover 1000 fee and 1000 as min to return to sender or we could end up
            // with a less than dust or negative output resulting in a non-standard TX that will be rejected.
            if (!selectInput(setAddress, inputtxid, inputvout, inputvalue, 1000 + 1000)) {
                // Error message sent to user inside selectInput call
                return;
            }

            CMutableTransaction rawTx;
            CTxIn in(COutPoint(inputtxid, inputvout), CScript(), std::numeric_limits<uint32_t>::max());
            rawTx.vin.push_back(in);

            // Return funds to sender
            CScript scriptPubKeySender = GetScriptForDestination(address.Get());
            CAmount valueout = inputvalue - 1000;
            CTxOut outSender(valueout, scriptPubKeySender);
            rawTx.vout.push_back(outSender);

            // Token data
            CTxOut out(0, CScript() << OP_RETURN << ParseHex(opreturndata));
            rawTx.vout.push_back(out);

            if (sendTokenTransaction(rawTx)) {
                clear();
            }
        }
    }
}

void TokenFormAddMetaEntry::clear()
{
    QLineEdit* metaCodeObj = findChild<QLineEdit*>("edit_meta_code");
    QLineEdit* metaDataObj = findChild<QLineEdit*>("edit_meta_data");

    if (metaCodeObj && metaDataObj) {
        metaCodeObj->setText("");
        metaDataObj->setText("");
    } else {
        return; // Could not find named Qt Object, should not happen, have form elements been renamed?
    }
}
