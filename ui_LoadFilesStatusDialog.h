/********************************************************************************
** Form generated from reading UI file 'LoadFilesStatusDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_LOADFILESSTATUSDIALOG_H
#define UI_LOADFILESSTATUSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QPlainTextEdit>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_LoadFilesStatusDialog
{
public:
    QVBoxLayout *verticalLayout_3;
    QTabWidget *tabWidget;
    QWidget *loadedTab;
    QVBoxLayout *verticalLayout;
    QPlainTextEdit *loadedFiles;
    QWidget *failedTab;
    QVBoxLayout *verticalLayout_2;
    QPlainTextEdit *failedFiles;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *LoadFilesStatusDialog)
    {
        if (LoadFilesStatusDialog->objectName().isEmpty())
            LoadFilesStatusDialog->setObjectName(QString::fromUtf8("LoadFilesStatusDialog"));
        LoadFilesStatusDialog->resize(461, 312);
        verticalLayout_3 = new QVBoxLayout(LoadFilesStatusDialog);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        tabWidget = new QTabWidget(LoadFilesStatusDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setDocumentMode(true);
        loadedTab = new QWidget();
        loadedTab->setObjectName(QString::fromUtf8("loadedTab"));
        verticalLayout = new QVBoxLayout(loadedTab);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        loadedFiles = new QPlainTextEdit(loadedTab);
        loadedFiles->setObjectName(QString::fromUtf8("loadedFiles"));
        QPalette palette;
        QBrush brush(QColor(0, 161, 0, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::Text, brush);
        palette.setBrush(QPalette::Inactive, QPalette::Text, brush);
        QBrush brush1(QColor(120, 120, 120, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Disabled, QPalette::Text, brush1);
        loadedFiles->setPalette(palette);
        QFont font;
        font.setBold(true);
        font.setWeight(75);
        loadedFiles->setFont(font);
        loadedFiles->setLineWrapMode(QPlainTextEdit::NoWrap);
        loadedFiles->setReadOnly(true);

        verticalLayout->addWidget(loadedFiles);

        tabWidget->addTab(loadedTab, QString());
        failedTab = new QWidget();
        failedTab->setObjectName(QString::fromUtf8("failedTab"));
        verticalLayout_2 = new QVBoxLayout(failedTab);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        failedFiles = new QPlainTextEdit(failedTab);
        failedFiles->setObjectName(QString::fromUtf8("failedFiles"));
        QPalette palette1;
        QBrush brush2(QColor(198, 18, 18, 255));
        brush2.setStyle(Qt::SolidPattern);
        palette1.setBrush(QPalette::Active, QPalette::Text, brush2);
        palette1.setBrush(QPalette::Inactive, QPalette::Text, brush2);
        palette1.setBrush(QPalette::Disabled, QPalette::Text, brush1);
        failedFiles->setPalette(palette1);
        failedFiles->setFont(font);
        failedFiles->setLineWrapMode(QPlainTextEdit::NoWrap);
        failedFiles->setReadOnly(true);

        verticalLayout_2->addWidget(failedFiles);

        tabWidget->addTab(failedTab, QString());

        verticalLayout_3->addWidget(tabWidget);

        buttonBox = new QDialogButtonBox(LoadFilesStatusDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_3->addWidget(buttonBox);


        retranslateUi(LoadFilesStatusDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), LoadFilesStatusDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), LoadFilesStatusDialog, SLOT(reject()));

        tabWidget->setCurrentIndex(1);


        QMetaObject::connectSlotsByName(LoadFilesStatusDialog);
    } // setupUi

    void retranslateUi(QDialog *LoadFilesStatusDialog)
    {
        LoadFilesStatusDialog->setWindowTitle(QApplication::translate("LoadFilesStatusDialog", "Some files failed to load", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(loadedTab), QApplication::translate("LoadFilesStatusDialog", "Loaded successfully: %1", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(failedTab), QApplication::translate("LoadFilesStatusDialog", "Failed to load: %1", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class LoadFilesStatusDialog: public Ui_LoadFilesStatusDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_LOADFILESSTATUSDIALOG_H
