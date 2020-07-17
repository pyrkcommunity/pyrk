#ifndef TOKENFORMSENDENTRY_H
#define TOKENFORMSENDENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormSendEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormSendEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormSendEntry();

private:
    void addSendEntry();
    void clear();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createSendClicked();
};

#endif // TOKENFORMSENDENTRY_H
