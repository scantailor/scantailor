#ifndef STARTBATCHPROCESSINGDIALOG_H
#define STARTBATCHPROCESSINGDIALOG_H

#include <QDialog>

namespace Ui {
class StartBatchProcessingDialog;
}

class StartBatchProcessingDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit StartBatchProcessingDialog(QWidget *parent = 0);
    ~StartBatchProcessingDialog();
    
    bool isAllPagesChecked() const;
    
private:
    Ui::StartBatchProcessingDialog *ui;
};

#endif // STARTBATCHPROCESSINGDIALOG_H
