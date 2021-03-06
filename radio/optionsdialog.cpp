#include "optionsdialog.h"
#include "ui_optionsDialog.h"
#include "options.h"
#include <QMessageBox>


OptionsDialog::OptionsDialog():
    QDialog(),
    ui(new Ui::OptionsDialog())
{
    ui->setupUi(this);
}


void showWarning(QWidget * parent, const QString &title, const QString &text);


void OptionsDialog::on_OptionsDialog_accepted()
{
    double actualBand = ui->actualBandEdit->value();
    double startFrequency = ui->frequencyEdit->value();
    double endFrequency = ui->endFrequencyEdit->value();
    double extraTicks = ui->extraTicksBox->value();
    double signalSpeed = ui->signalSpeedBox->value();

    auto options = Options::getInstance();

    options->setActualBand(actualBand * 1e3);
    options->setStartFrequency(std::min<double>(startFrequency, endFrequency) * 1e6);
    options->setEndFrequency(std::max<double>(startFrequency, endFrequency) * 1e6);
    options->setExtraTicks((size_t)(extraTicks * 1e3));
    options->setSignalSpeed(signalSpeed * 1e3);

    options->save();
}

void OptionsDialog::onRecalculateBand()
{
    double actualBand = ui->actualBandEdit->value();
    double signalSpeed = ui->signalSpeedBox->value();

    auto band = Options::getInstance()->getBand();
    size_t window = band * actualBand / signalSpeed;
    size_t i = 2;
    while ((i *= 2) < window) ;
    window = i;
    double newActualBand = i * signalSpeed / band;

    ui->calculatedBandLabel->setText(QString("Расчетная полоса: %1 КГц").arg(newActualBand, 0, 'f', 3));
}

int OptionsDialog::exec()
{
    Options* options = Options::getInstance();
    ui->actualBandEdit->setValue(options->getActualBand() / 1e3);
    ui->frequencyEdit->setValue(options->getStartFrequency() / 1e6);
    ui->endFrequencyEdit->setValue(options->getEndFrequency() / 1e6);
    ui->extraTicksBox->setValue(options->getExtraTicks() / 1e3);
    ui->signalSpeedBox->setValue(options->getSignalSpeed() / 1e3);
    return QDialog::exec();
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void showWarning(QWidget * parent, const QString &title, const QString &text)
{
    QMessageBox box(parent);
    box.setModal(true);
    box.setIcon(QMessageBox::Icon::Warning);
    box.setWindowTitle(title);
    box.setText(text);
    QSpacerItem* spacer = new QSpacerItem(300, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    QGridLayout* layout = (QGridLayout*)box.layout();
    layout->addItem(spacer, layout->rowCount(), 0, 1, layout->columnCount());
    box.exec();
}

#undef CHECK_VALID
