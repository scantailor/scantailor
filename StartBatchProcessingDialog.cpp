#include "StartBatchProcessingDialog.h"
#include "StartBatchProcessingDialog.h.moc"
#include "ui_StartBatchProcessingDialog.h"


#include <QSettings>

StartBatchProcessingDialog::StartBatchProcessingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartBatchProcessingDialog)
{
    ui->setupUi(this);
    
    QSettings settings;
    if (settings.value("StartBatchProcessing/pages").toBool()) {
        ui->allPages->setChecked(true);
        ui->fromSelected->setChecked(false);
    } else {
        ui->allPages->setChecked(false);
        ui->fromSelected->setChecked(true);
    }
}

StartBatchProcessingDialog::~StartBatchProcessingDialog()
{
    QSettings settings;
    settings.setValue("StartBatchProcessing/pages", ui->allPages->isChecked());
    delete ui;
}

bool StartBatchProcessingDialog::isAllPagesChecked() const
{
	return ui->allPages->isChecked();
}
