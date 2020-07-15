#ifndef TOKENFORMBURNENTRY_H
#define TOKENFORMBURNENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormBurnEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormBurnEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormBurnEntry();

private:
    void addCreateEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void burnTokenClicked();
};

#endif // TOKENFORMBURNENTRY_H
