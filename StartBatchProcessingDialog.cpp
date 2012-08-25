#include "StartBatchProcessingDialog.h"
#include "StartBatchProcessingDialog.h.moc"
#include "ui_StartBatchProcessingDialog.h"


#include <QSettings>

StartBatchProcessingDialog::StartBatchProcessingDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartBatchProcessingDialog)
{
    ui->setupUi(this);
    
    ui->allPages->setChecked(true);
    ui->fromSelected->setChecked(false);
}

StartBatchProcessingDialog::~StartBatchProcessingDialog()
{
    delete ui;
}

bool StartBatchProcessingDialog::isAllPagesChecked() const
{
	return ui->allPages->isChecked();
}
