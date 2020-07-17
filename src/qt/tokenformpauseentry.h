#ifndef TOKENFORMPAUSEENTRY_H
#define TOKENFORMPAUSEENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormPauseEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormPauseEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormPauseEntry();

private:
    void addPauseEntry();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createPauseClicked();
};

#endif // TOKENFORMPAUSEENTRY_H
