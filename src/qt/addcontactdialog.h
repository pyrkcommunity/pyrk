// Copyright (c) 2017-2020 Trezarcoin developers
// Copyright (c) 2020 Peter Bushnell
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_QT_ADDCONTACTDIALOG_H
#define BITCOIN_QT_ADDCONTACTDIALOG_H

#include <QDialog>

class PlatformStyle;
class SecureMessageGUI;
class WalletModel;

namespace Ui {
    class AddContactDialog;
}

/** Multifunctional dialog to add Contacts to SecureMessageGUI.
 */
class AddContactDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddContactDialog(const PlatformStyle *platformStyle, SecureMessageGUI *secureMessage, QWidget *parent = 0);
    ~AddContactDialog();

    void setModel(WalletModel *model);
    void accept();
    void reject();

private:
    Ui::AddContactDialog *ui;
    WalletModel *model;
    SecureMessageGUI *secureMessage;
    const PlatformStyle *platformStyle;

private Q_SLOTS:
    void textChanged();
    void clearFields();
};

#endif // BITCOIN_QT_ADDCONTACTDIALOG_H
