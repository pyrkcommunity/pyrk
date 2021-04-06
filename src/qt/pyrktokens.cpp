#include "pyrktokens.h"
#include "ui_pyrktokens.h"

#include "clientmodel.h"
#include "guiutil.h"
#include "tokenformaddmetaentry.h"
#include "tokenformauthmetaentry.h"
#include "tokenformburnentry.h"
#include "tokenformcreateentry.h"
#include "tokenformnewownerentry.h"
#include "tokenformpauseentry.h"
#include "tokenformresumeentry.h"
#include "tokenformrevokemetaentry.h"
#include "tokenformsendentry.h"
#include "tokenselectaddressdialog.h"
#include "util.h"
#include "utiltime.h"
#include "walletmodel.h"

#include <QAction>
#include <QDesktopServices>
#include <QEvent>
#include <QHeaderView>
#include <QPushButton>
#include <QSettings>
#include <QSignalMapper>
#include <QTableWidgetItem>
#include <QUrl>

#include <univalue.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

#include <iostream>
#include <fstream>

PyrkTokens::PyrkTokens(const PlatformStyle *platformStyle, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PyrkTokens),
    platformStyle(platformStyle),
    walletModel(0)
{
    ui->setupUi(this);

    // Transaction table
    ui->table_transaction_list->setColumnWidth(0, 150);
    ui->table_transaction_list->setColumnWidth(1, 60);
    ui->table_transaction_list->setColumnWidth(2, 120);
    ui->table_transaction_list->setColumnWidth(3, 280);
    ui->table_transaction_list->setColumnWidth(4, 180);
    ui->table_transaction_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table_transaction_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table_transaction_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Token list table
    ui->table_token_list->setColumnWidth(0, 60);
    ui->table_token_list->setColumnWidth(1, 180);
    ui->table_token_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->table_token_list->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->table_token_list->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Disable all buttons
    ui->button_add_meta->setDisabled(true);
    ui->button_auth_meta->setDisabled(true);
    ui->button_burn->setDisabled(true);
    ui->button_create_token->setDisabled(true);
    ui->button_new_owner->setDisabled(true);
    ui->button_pause->setDisabled(true);
    ui->button_resume->setDisabled(true);
    ui->button_revoke_meta->setDisabled(true);
    ui->button_set_address->setDisabled(true);
    ui->button_send->setDisabled(true);
    ui->button_view_on_explorer->setDisabled(true);

    // Allow some QLabels to be selected
    ui->label_token_address->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->label_token_id_value_2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->label_name_value_2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->label_symbol_value_2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->label_owner_value_2->setTextInteractionFlags(Qt::TextSelectableByMouse);
    ui->label_uri_value_2->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // Connect buttons
    connect(ui->button_auth_meta, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_add_meta, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_burn, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_create_token, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_new_owner, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_pause, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_resume, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_revoke_meta, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_send, SIGNAL(clicked()), this, SLOT(createNewForm()));
    connect(ui->button_set_address, SIGNAL(clicked()), this, SLOT(setAddressClicked()));
    connect(ui->button_view_on_explorer, SIGNAL(clicked()), this, SLOT(viewOnExplorerClicked()));


    // Right click transaction / token list context menus
    QAction *copyAddressTokenAction = new QAction(tr("&Copy"), this);
    QAction *copyAddressTXAction = new QAction(tr("&Copy"), this);

    contextTokenMenu = new QMenu(this);
    contextTokenMenu->addAction(copyAddressTokenAction);

    contextTXMenu = new QMenu(this);
    contextTXMenu->addAction(copyAddressTXAction);

    connect(copyAddressTokenAction, SIGNAL(triggered()), this, SLOT(copyAddressTokenTable()));
    connect(copyAddressTXAction, SIGNAL(triggered()), this, SLOT(copyAddressTXTable()));

    connect(ui->table_token_list, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextualTokenMenu(const QPoint&)));
    connect(ui->table_transaction_list, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextualTXMenu(const QPoint&)));

    // Row in token list selected
    connect(ui->table_token_list, SIGNAL(clicked(const QModelIndex &)), this, SLOT(populateTokenInfo()));

    // Get signals for TX and encrypted wallet change
    subscribeToCoreSignals();
}

PyrkTokens::~PyrkTokens()
{
    delete ui;
}

void PyrkTokens::contextualTokenMenu(const QPoint &point)
{
    QModelIndex index = ui->table_transaction_list->indexAt(point);
    if(index.isValid())
    {
        contextTokenMenu->exec(QCursor::pos());
    }
}

void PyrkTokens::contextualTXMenu(const QPoint &point)
{
    QModelIndex index = ui->table_transaction_list->indexAt(point);
    if(index.isValid())
    {
        contextTXMenu->exec(QCursor::pos());
    }
}

void PyrkTokens::copyAddressTokenTable()
{
    if(!ui->table_token_list->selectionModel())
        return;

    QModelIndex index = ui->table_token_list->selectionModel()->currentIndex();
    GUIUtil::setClipboard(ui->table_token_list->model()->index(index.row(), index.column()).data().toString());
}

void PyrkTokens::copyAddressTXTable()
{
    if(!ui->table_transaction_list->selectionModel())
        return;

    QModelIndex index = ui->table_transaction_list->selectionModel()->currentIndex();
    GUIUtil::setClipboard(ui->table_transaction_list->model()->index(index.row(), index.column()).data().toString());
}

void PyrkTokens::getTokenAddress()
{
    std::string tokenAddress;

    boost::filesystem::path tokenaddresspath = GetDataDir() / "tokenaddress.txt";
    std::string tokenaddressfile = tokenaddresspath.string();
    std::fstream newfile;
    newfile.open(tokenaddressfile, std::ios::in); //open a file to perform read operation using file object
    if (newfile.is_open()) {   //checking whether the file is open
        getline(newfile, tokenAddress);
        newfile.close();   //close the file object.
    }

    if (tokenAddress.empty()) {
        std::string strAccount = AccountFromValue("PYRKTOKENS");
        std::string ret = GetAccountAddress(strAccount).ToString();

        newfile.open(tokenaddressfile,std::ios::out);  // open a file to perform write operation using file object
        if(newfile.is_open())     //checking whether the file is open
        {
            newfile << ret;
            newfile.close(); //close the file object
        }

        tokenAddress = ret;
    }

    ui->label_token_address->setText(QString::fromStdString(tokenAddress));
}

bool PyrkTokens::tokenAddress(std::string& currentTokenAddress)
{
    currentTokenAddress = ui->label_token_address->text().toStdString();


    if (currentTokenAddress.empty())
    {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token Address empty."));
        return false;
    }

    CBitcoinAddress address(currentTokenAddress);

    if (!address.IsValid()) {
        QMessageBox::critical(nullptr, tr("Error"),
            tr("Token Address not valid, please select another address."));
        return false;
    }

    return true;
}

void PyrkTokens::populateTokenInfo()
{
    QModelIndexList selectedHexID = ui->table_token_list->selectionModel()->selectedRows(1);
    std::string tickerHexID = selectedHexID.at(0).data().toString().toStdString();

    // Token info already displayed
    if (ui->label_token_id_value_2->text().toStdString() == tickerHexID) {
        return;
    }

    deleteForm();

    std::ostringstream oss;
    if(CURLE_OK == curl_read(gArgs.GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "token/" + tickerHexID, oss))
    {
        auto json = nlohmann::json::parse(oss.str());

        if (json.is_null() || json.empty()) {
            return;
        }

        if (!json.contains("tokenDetails") || !json.contains("tokenStats")) {
            return;
        }

        auto tokenDetails = json["tokenDetails"];
        auto tokenStats = json["tokenStats"];

        if (tokenDetails.contains("name") && tokenDetails.contains("symbol") && tokenStats.contains("qty_token_circulating_supply") &&
                tokenDetails.contains("ownerAddress") && tokenDetails.contains("documentUri") && json.contains("paused") &&
                tokenStats.contains("qty_valid_meta_since_genesis"))
        {
            std::string name = tokenDetails["name"];
            std::string symbol = tokenDetails["symbol"];
            std::string supply = tokenStats["qty_token_circulating_supply"];

            // Make sure ownerAddress not null due to bug in transfer owner
            // https://github.com/pyrkcommunity/pyrk/issues/3
            std::string owner;
            if (!tokenDetails["ownerAddress"].is_null()) {
                owner = tokenDetails["ownerAddress"];
            }

            std::string documentURI = tokenDetails["documentUri"];
            bool paused = json["paused"];
            int metaItems = tokenStats["qty_valid_meta_since_genesis"];

            ui->label_token_id_value_2->setText(QString::fromStdString(tickerHexID));
            ui->label_name_value_2->setText(QString::fromStdString(name));
            ui->label_symbol_value_2->setText(QString::fromStdString(symbol));
            ui->label_circulation_value_2->setText(QString::fromStdString(supply));
            ui->label_owner_value_2->setText(QString::fromStdString(owner));
            ui->label_uri_value_2->setText(QString::fromStdString(documentURI));
            ui->label_paused_value_2->setText(QString::fromStdString(paused ? "Yes" : "No"));
            ui->label_meta_items_value_2->setText(QString::number(metaItems));

            ui->button_send->setDisabled(false);
            ui->button_view_on_explorer->setDisabled(false);

            // Now let's find out if we have add meta auth
            ui->button_add_meta->setDisabled(true); // Disable to start with

            // Reuse stream
            oss.str("");
            oss.clear();

            // Get token address
            CBitcoinAddress tokenAddress(ui->label_token_address->text().toStdString());
            if (!tokenAddress.IsValid()) {
                return;
            }

            // Get info for address
            if (CURLE_OK == curl_read(gArgs.GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "ismetaauth/" + tickerHexID + "/" + tokenAddress.ToString(), oss))
            {
                json = nlohmann::json::parse(oss.str());

                if (json.is_null() || json.empty()) {
                    return;
                }

                // Do we have a matching ID and is meta auth true
                if (json.contains("isMetaAuth") && json["isMetaAuth"] == true)
                {
                    // We have meta auth access, enable the button!
                    ui->button_add_meta->setDisabled(false);
                }
            }

            // The rest of the buttons are determined by token ownership
            if (ui->label_token_address->text().toStdString() == owner) {
                ui->button_auth_meta->setDisabled(false);
                ui->button_burn->setDisabled(false);
                ui->button_new_owner->setDisabled(false);
                ui->button_pause->setDisabled(false);
                ui->button_resume->setDisabled(false);
                ui->button_revoke_meta->setDisabled(false);
            } else {
                ui->button_auth_meta->setDisabled(true);
                ui->button_burn->setDisabled(true);
                ui->button_new_owner->setDisabled(true);
                ui->button_pause->setDisabled(true);
                ui->button_resume->setDisabled(true);
                ui->button_revoke_meta->setDisabled(true);
            }
        }
    }
}

void PyrkTokens::populateTransactionList()
{
    CBitcoinAddress tokenAddress(ui->label_token_address->text().toStdString());
    if (!tokenAddress.IsValid()) {
        return;
    }

    ui->table_transaction_list->setRowCount(0);

    std::ostringstream oss;
    if(CURLE_OK == curl_read(gArgs.GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "addresstransactions/" + tokenAddress.ToString() + "?limit=" + "0" /* infinite number? */, oss))
    {
        auto json = nlohmann::json::parse(oss.str());

        if (json.is_null() || json.empty()) {
            return;
        }

        if (json.is_array()) {
            // Handle QTBUG-75479
            ui->table_transaction_list->setSortingEnabled(false);

            for (auto line : json) {
                if (!line.contains("transactionDetails")) {
                    continue;
                }

                auto transactionDetails = line["transactionDetails"];

                if (!transactionDetails.contains("sendOutput")) {
                    continue;
                }

                auto sendOutput = transactionDetails["sendOutput"];

                if (transactionDetails.contains("timestamp_unix") && transactionDetails.contains("symbol") &&
                        transactionDetails.contains("transactionType") && transactionDetails.contains("tokenIdHex") &&
                        sendOutput.contains("address") && sendOutput.contains("amount"))
                {
                    int newRow = ui->table_transaction_list->rowCount();
                    ui->table_transaction_list->insertRow(newRow);

                    std::string timestanp_unix = DateTimeStrFormat("%Y-%m-%d %H:%M:%S", transactionDetails["timestamp_unix"]);

                    QTableWidgetItem *timestamp = new QTableWidgetItem(QString::fromStdString(timestanp_unix));
                    QTableWidgetItem *symbol = new QTableWidgetItem(QString::fromStdString(transactionDetails["symbol"]));
                    QTableWidgetItem *type = new QTableWidgetItem(QString::fromStdString(transactionDetails["transactionType"]));
                    QTableWidgetItem *address = new QTableWidgetItem(QString::fromStdString(sendOutput["address"]));
                    QTableWidgetItem *tokenIdHex = new QTableWidgetItem(QString::fromStdString(transactionDetails["tokenIdHex"]));
                    QTableWidgetItem *amount = new QTableWidgetItem(QString::fromStdString(sendOutput["amount"]));

                    ui->table_transaction_list->setItem(newRow, 0, timestamp);
                    ui->table_transaction_list->setItem(newRow, 1, symbol);
                    ui->table_transaction_list->setItem(newRow, 2, type);
                    ui->table_transaction_list->setItem(newRow, 3, address);
                    ui->table_transaction_list->setItem(newRow, 4, tokenIdHex);
                    ui->table_transaction_list->setItem(newRow, 5, amount);
                }
            }

            // Handle QTBUG-75479
            ui->table_transaction_list->setSortingEnabled(true);
        }
    }
}

void PyrkTokens::populateTokenList()
{
    CBitcoinAddress tokenAddress(ui->label_token_address->text().toStdString());
    if (!tokenAddress.IsValid()) {
        return;
    }

    ui->table_token_list->setRowCount(0);

    std::ostringstream oss;
    if(CURLE_OK == curl_read(gArgs.GetArg("-tokenapiurl", "https://tokenapi.pyrk.org/api/") + "address/" + tokenAddress.ToString(), oss))
    {
        auto json = nlohmann::json::parse(oss.str());

        if (json.is_null() || json.empty()) {
            return;
        }

        if (json.is_array()) {
            // Handle QTBUG-75479
            ui->table_token_list->setSortingEnabled(false);
            for (auto line : json) {
                if (line.contains("symbol") && line.contains("tokenIdHex") &&
                        line.contains("tokenBalance") && line.contains("isOwner"))
                {
                    int newRow = ui->table_token_list->rowCount();
                    ui->table_token_list->insertRow(newRow);

                    std::string isOwner = line["isOwner"] == true ? "*" : "";
                    std::string symbol = line["symbol"];

                    QTableWidgetItem *symbolItem = new QTableWidgetItem(QString::fromStdString(symbol + isOwner));
                    QTableWidgetItem *tokenIdHexItem = new QTableWidgetItem(QString::fromStdString(line["tokenIdHex"]));
                    QTableWidgetItem *tokenBalanceItem = new QTableWidgetItem(QString::fromStdString(line["tokenBalance"]));

                    ui->table_token_list->setItem(newRow, 0, symbolItem);
                    ui->table_token_list->setItem(newRow, 1, tokenIdHexItem);
                    ui->table_token_list->setItem(newRow, 2, tokenBalanceItem);


                }
            }
            // Handle QTBUG-75479
            ui->table_token_list->setSortingEnabled(true);

            ui->table_token_list->sortByColumn(0, Qt::AscendingOrder);
        }
    }
}

void PyrkTokens::updateTables()
{
    // Populate token and transaction list based on above Pyrk token address
    populateTokenList();
    populateTransactionList();
}

void PyrkTokens::setWalletModel(WalletModel *model)
{
    this->walletModel = model;

    if (model) {
        ui->button_set_address->setDisabled(false);
        ui->button_create_token->setDisabled(false);

        // Get Pyrk token address
        getTokenAddress();

        // Populate tables
        updateTables();

        // Balances changed, update tables in case our tokens are effected
        connect(model, SIGNAL(balanceChanged(CAmount,CAmount,CAmount,CAmount,CAmount,CAmount,CAmount)), this, SLOT(updateTables()));
    }
}

bool PyrkTokens::event(QEvent* pEvent)
{
    if (pEvent->type() == QEvent::Show) {
        // Prevent slight resizing when choosing new token address
        ui->widget_token_overview->setFixedWidth(ui->widget_token_overview->width());
    }

    return QWidget::event(pEvent);
}

void PyrkTokens::resetTokenInfo()
{
    ui->label_token_id_value_2->clear();
    ui->label_name_value_2->clear();
    ui->label_symbol_value_2->clear();
    ui->label_circulation_value_2->clear();
    ui->label_owner_value_2->clear();
    ui->label_uri_value_2->clear();
    ui->label_paused_value_2->clear();
    ui->label_meta_items_value_2->clear();

    ui->button_add_meta->setDisabled(true);
    ui->button_auth_meta->setDisabled(true);
    ui->button_burn->setDisabled(true);
    ui->button_new_owner->setDisabled(true);
    ui->button_pause->setDisabled(true);
    ui->button_resume->setDisabled(true);
    ui->button_revoke_meta->setDisabled(true);
    ui->button_send->setDisabled(true);
    ui->button_view_on_explorer->setDisabled(true);
}

void PyrkTokens::deleteForm()
{
    QList<QWidget*> widgets = ui->widget_token_form->findChildren<QWidget*>();
    if (widgets.size()) {
        for (auto it = widgets.rbegin(); it != widgets.rend(); ++it) {
            ui->layout_token_form->removeWidget(*it);
            delete *it;
        }
    }
}

void PyrkTokens::createNewForm()
{
    deleteForm();

    // Get type from button
    QPushButton* pButton = qobject_cast<QPushButton*>(sender());
    if (pButton) {

        QWidget* entry;
        std::string callingButton = pButton->objectName().toStdString();

        // Add meta form
        if (callingButton == "button_add_meta") {
            entry = new TokenFormAddMetaEntry(this, walletModel);
        }

        // Auth meta form
        if (callingButton == "button_auth_meta") {
            entry = new TokenFormAuthMetaEntry(this, walletModel);
        }

        // Burn token form
        if (callingButton == "button_burn") {
            entry = new TokenFormBurnEntry(this, walletModel);
        }

        // Create token form
        if (callingButton == "button_create_token") {
            resetTokenInfo();

            entry = new TokenFormCreateEntry(this, walletModel);
        }

        // Resume form
        if (callingButton == "button_new_owner") {
            entry = new TokenFormNewOwnerEntry(this, walletModel);
        }

        // Pause form
        if (callingButton == "button_pause") {
            entry = new TokenFormPauseEntry(this, walletModel);
        }

        // Resume form
        if (callingButton == "button_resume") {
            entry = new TokenFormResumeEntry(this, walletModel);
        }

        // Revoke meta form
        if (callingButton == "button_revoke_meta") {
            entry = new TokenFormRevokeMetaEntry(this, walletModel);
        }

        // Send token form
        if (callingButton == "button_send") {
            entry = new TokenFormSendEntry(this, walletModel);
        }

        ui->layout_token_form->addWidget(entry);
    }
}

void PyrkTokens::viewOnExplorerClicked()
{
    QModelIndexList selectedHexID = ui->table_token_list->selectionModel()->selectedRows(1);
    std::string tickerHexID = selectedHexID.at(0).data().toString().toStdString();

    if (!tickerHexID.empty()) {
        std::string url = "https://explorer.pyrk.org/token/" + tickerHexID;
        QDesktopServices::openUrl(QUrl(url.c_str()));
    }
}

void PyrkTokens::setAddressClicked()
{
    TokenSelectAddressDialog::selectedAddress = ui->label_token_address->text();

    TokenSelectAddressDialog dlg(platformStyle);
    dlg.setModel(walletModel);
    dlg.exec();

    if (ui->label_token_address->text() != TokenSelectAddressDialog::selectedAddress) {
        std::string strAccount = AccountFromValue("PYRKTOKENS");

        CBitcoinAddress address(TokenSelectAddressDialog::selectedAddress.toStdString());

        if (vpwallets[0]->mapAddressBook.count(address.Get()))
        {
            std::string strOldAccount = vpwallets[0]->mapAddressBook[address.Get()].name;
            if (address == GetAccountAddress(strOldAccount))
                GetAccountAddress(strOldAccount, true);
        }
        vpwallets[0]->SetAddressBook(address.Get(), strAccount, "receive");

        boost::filesystem::path tokenaddresspath = GetDataDir() / "tokenaddress.txt";
        std::string tokenaddressfile = tokenaddresspath.string().c_str();
        std::fstream newfile;
        newfile.open(tokenaddressfile,std::ios::out);  // open a file to perform write operation using file object
        if(newfile.is_open())     //checking whether the file is open
        {
            newfile << TokenSelectAddressDialog::selectedAddress.toStdString();
            newfile.close(); //close the file object
        }

        ui->label_token_address->setText(TokenSelectAddressDialog::selectedAddress);

        // Repopulate token and transaction list based on new Pyrk token address
        populateTokenList();
        populateTransactionList();

        // Clear previous token info and forms
        resetTokenInfo();
        deleteForm();
    }
}

static void NotifyTransactionChanged(PyrkTokens *pyrkTokens, CWallet *wallet, const uint256 &hash, ChangeType status)
{
    Q_UNUSED(wallet);
    Q_UNUSED(hash);
    Q_UNUSED(status);
    QMetaObject::invokeMethod(pyrkTokens, "updateTables", Qt::QueuedConnection);
}

void PyrkTokens::subscribeToCoreSignals()
{
    vpwallets[0]->NotifyTransactionChanged.connect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
}

void PyrkTokens::unsubscribeFromCoreSignals()
{
    vpwallets[0]->NotifyTransactionChanged.disconnect(boost::bind(NotifyTransactionChanged, this, _1, _2, _3));
}
