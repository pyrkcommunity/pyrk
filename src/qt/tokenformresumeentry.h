#ifndef TOKENFORMRESUMEENTRY_H
#define TOKENFORMRESUMEENTRY_H

#include <QWidget>

class PyrkTokens;
class WalletModel;

namespace Ui {
class TokenFormEntry;
}

class TokenFormResumeEntry : public QWidget
{
    Q_OBJECT

public:
    explicit TokenFormResumeEntry(PyrkTokens *parent, WalletModel *model);
    ~TokenFormResumeEntry();

private:
    void addResumeEntry();

    PyrkTokens *parent;
    Ui::TokenFormEntry *ui;
    WalletModel *walletModel;

private Q_SLOTS:
    void createResumeClicked();
};

#endif // TOKENFORMRESUMEENTRY_H
