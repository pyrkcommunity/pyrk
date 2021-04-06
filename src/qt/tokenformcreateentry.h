#ifndef TOKENFORMCREATEENTRY_H
#define TOKENFORMCREATEENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormCreateEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormCreateEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormCreateEntry();

private:
    void addCreateEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createTokenClicked();
};

#endif // TOKENFORMCREATEENTRY_H
