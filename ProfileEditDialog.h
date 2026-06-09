#pragma once

#include "Profile.h"

#include <QDialog>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;

class ProfileEditDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProfileEditDialog(QWidget *parent = nullptr);

    void setProfile(const Profile &profile);
    Profile profile() const;

private:
    void setupUi();
    void fillFromProfile(const Profile &profile);
    void updateVisibility();

    QComboBox *schemeCombo_ = nullptr;
    QLineEdit *nameEdit_ = nullptr;
    QLineEdit *hostEdit_ = nullptr;
    QSpinBox *portSpin_ = nullptr;
    QLineEdit *credentialEdit_ = nullptr;
    QLineEdit *methodEdit_ = nullptr;
    QLineEdit *encryptionEdit_ = nullptr;
    QLineEdit *securityEdit_ = nullptr;
    QLineEdit *transportEdit_ = nullptr;
    QLineEdit *pathEdit_ = nullptr;
    QLineEdit *hostHeaderEdit_ = nullptr;
    QLineEdit *serviceNameEdit_ = nullptr;
    QLineEdit *sniEdit_ = nullptr;
    QLineEdit *publicKeyEdit_ = nullptr;
    QLineEdit *shortIdEdit_ = nullptr;
    QLineEdit *flowEdit_ = nullptr;
    QPlainTextEdit *uriEdit_ = nullptr;

    Profile profile_;
};
