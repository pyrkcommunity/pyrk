#ifndef TOKENSELECTADDRESSDIALOG_H
#define TOKENSELECTADDRESSDIALOG_H

#include "platformstyle.h"

#include <QMenu>
#include <QDialog>
#include <QPoint>

namespace Ui {
    class TokenSelectAddressDialog;
}

class WalletModel;

/** Payrk Tokens page widget */
class TokenSelectAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TokenSelectAddressDialog(const PlatformStyle *platformStyle, QWidget *parent = 0);
    ~TokenSelectAddressDialog();

    void setModel(WalletModel *model);

    static QString selectedAddress;

private:
    QMenu *contextMenu;
    const PlatformStyle *platformStyle;

private:
    Ui::TokenSelectAddressDialog *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void buttonBoxClicked();
    void contextualMenu(const QPoint &point);
    void selectionChanged();
    void on_copyAddress_clicked();
};

#endif // TOKENSELECTADDRESSDIALOG_H
