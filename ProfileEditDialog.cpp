#include "ProfileEditDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

ProfileEditDialog::ProfileEditDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
    fillFromProfile(profile_);
}

void ProfileEditDialog::setupUi()
{
    setWindowTitle(QStringLiteral("Редактор профиля"));
    resize(620, 720);

    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setFormAlignment(Qt::AlignTop);

    schemeCombo_ = new QComboBox(this);
    schemeCombo_->addItems({QStringLiteral("vless"), QStringLiteral("vmess"), QStringLiteral("trojan"), QStringLiteral("shadowsocks")});

    nameEdit_ = new QLineEdit(this);
    hostEdit_ = new QLineEdit(this);
    portSpin_ = new QSpinBox(this);
    portSpin_->setRange(1, 65535);
    portSpin_->setValue(443);
    credentialEdit_ = new QLineEdit(this);
    methodEdit_ = new QLineEdit(this);
    encryptionEdit_ = new QLineEdit(this);
    securityEdit_ = new QLineEdit(this);
    transportEdit_ = new QLineEdit(this);
    pathEdit_ = new QLineEdit(this);
    hostHeaderEdit_ = new QLineEdit(this);
    serviceNameEdit_ = new QLineEdit(this);
    sniEdit_ = new QLineEdit(this);
    publicKeyEdit_ = new QLineEdit(this);
    shortIdEdit_ = new QLineEdit(this);
    flowEdit_ = new QLineEdit(this);
    uriEdit_ = new QPlainTextEdit(this);
    uriEdit_->setPlaceholderText(QStringLiteral("Исходная импортированная ссылка (опционально)"));
    uriEdit_->setMaximumBlockCount(10);
    uriEdit_->setFixedHeight(90);

    form->addRow(QStringLiteral("Тип"), schemeCombo_);
    form->addRow(QStringLiteral("Название"), nameEdit_);
    form->addRow(QStringLiteral("Сервер"), hostEdit_);
    form->addRow(QStringLiteral("Порт"), portSpin_);
    form->addRow(QStringLiteral("Credential / UUID / Password"), credentialEdit_);
    form->addRow(QStringLiteral("Method (SS)"), methodEdit_);
    form->addRow(QStringLiteral("Encryption"), encryptionEdit_);
    form->addRow(QStringLiteral("Security"), securityEdit_);
    form->addRow(QStringLiteral("Transport"), transportEdit_);
    form->addRow(QStringLiteral("Path"), pathEdit_);
    form->addRow(QStringLiteral("Host Header"), hostHeaderEdit_);
    form->addRow(QStringLiteral("gRPC Service"), serviceNameEdit_);
    form->addRow(QStringLiteral("SNI"), sniEdit_);
    form->addRow(QStringLiteral("Reality Public Key"), publicKeyEdit_);
    form->addRow(QStringLiteral("Reality Short ID"), shortIdEdit_);
    form->addRow(QStringLiteral("Flow"), flowEdit_);
    form->addRow(QStringLiteral("Raw URI"), uriEdit_);

    auto *note = new QLabel(QStringLiteral("Редактор сохраняет профиль в локальный JSON. Для подписочных профилей изменения могут быть перезаписаны следующим обновлением подписки."), this);
    note->setWordWrap(true);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(schemeCombo_, &QComboBox::currentTextChanged, this, [this]() { updateVisibility(); });

    layout->addLayout(form);
    layout->addWidget(note);
    layout->addWidget(buttons);
}

void ProfileEditDialog::setProfile(const Profile &profile)
{
    profile_ = profile;
    fillFromProfile(profile_);
}

void ProfileEditDialog::fillFromProfile(const Profile &profile)
{
    const int schemeIndex = schemeCombo_->findText(profile.scheme);
    if (schemeIndex >= 0) {
        schemeCombo_->setCurrentIndex(schemeIndex);
    }

    nameEdit_->setText(profile.name);
    hostEdit_->setText(profile.host);
    portSpin_->setValue(profile.port > 0 ? profile.port : 443);
    credentialEdit_->setText(profile.credential);
    methodEdit_->setText(profile.method);
    encryptionEdit_->setText(profile.encryption);
    securityEdit_->setText(profile.security);
    transportEdit_->setText(profile.transport);
    pathEdit_->setText(profile.path);
    hostHeaderEdit_->setText(profile.hostHeader);
    serviceNameEdit_->setText(profile.serviceName);
    sniEdit_->setText(profile.sni);
    publicKeyEdit_->setText(profile.publicKey);
    shortIdEdit_->setText(profile.shortId);
    flowEdit_->setText(profile.flow);
    uriEdit_->setPlainText(profile.uri);
    updateVisibility();
}

Profile ProfileEditDialog::profile() const
{
    Profile result = profile_;
    result.scheme = schemeCombo_->currentText().trimmed();
    result.name = nameEdit_->text().trimmed();
    result.host = hostEdit_->text().trimmed();
    result.port = portSpin_->value();
    result.credential = credentialEdit_->text().trimmed();
    result.method = methodEdit_->text().trimmed();
    result.encryption = encryptionEdit_->text().trimmed();
    result.security = securityEdit_->text().trimmed();
    result.transport = transportEdit_->text().trimmed();
    result.path = pathEdit_->text().trimmed();
    result.hostHeader = hostHeaderEdit_->text().trimmed();
    result.serviceName = serviceNameEdit_->text().trimmed();
    result.sni = sniEdit_->text().trimmed();
    result.publicKey = publicKeyEdit_->text().trimmed();
    result.shortId = shortIdEdit_->text().trimmed();
    result.flow = flowEdit_->text().trimmed();
    result.uri = uriEdit_->toPlainText().trimmed();

    if (result.scheme == QStringLiteral("shadowsocks")) {
        result.security = QStringLiteral("none");
        result.transport = QStringLiteral("tcp");
    }

    if (result.name.isEmpty()) {
        result.name = QStringLiteral("%1:%2").arg(result.host).arg(result.port);
    }

    return result;
}

void ProfileEditDialog::updateVisibility()
{
    const bool isShadowsocks = schemeCombo_->currentText() == QStringLiteral("shadowsocks");
    methodEdit_->setEnabled(isShadowsocks);
    encryptionEdit_->setEnabled(!isShadowsocks);
    publicKeyEdit_->setEnabled(!isShadowsocks);
    shortIdEdit_->setEnabled(!isShadowsocks);
    flowEdit_->setEnabled(!isShadowsocks);
}
