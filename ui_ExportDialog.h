/********************************************************************************
** Form generated from reading UI file 'ExportDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_EXPORTDIALOG_H
#define UI_EXPORTDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QTabWidget>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ExportDialog
{
public:
    QTabWidget *tabWidget;
    QWidget *tab;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QProgressBar *progressBar;
    QCheckBox *SplitMixed;
    QCheckBox *DefaultOutputFolder;
    QGroupBox *groupBoxExport;
    QHBoxLayout *ExportLayout;
    QLineEdit *outExportDirLine;
    QPushButton *outExportDirBrowseBtn;
    QPushButton *ExportButton;
    QPushButton *OkButton;
    QLabel *labelFilesProcessed;
    QWidget *tab_2;
    QCheckBox *GenerateBlankBackSubscans;
    QCheckBox *KeepOriginalColorIllumForeSubscans;

    void setupUi(QDialog *ExportDialog)
    {
        if (ExportDialog->objectName().isEmpty())
            ExportDialog->setObjectName(QString::fromUtf8("ExportDialog"));
        ExportDialog->resize(486, 231);
        tabWidget = new QTabWidget(ExportDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setGeometry(QRect(0, 1, 486, 230));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        scrollArea = new QScrollArea(tab);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setGeometry(QRect(-1, -3, 489, 205));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Plain);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 489, 205));
        progressBar = new QProgressBar(scrollAreaWidgetContents);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setGeometry(QRect(8, 150, 469, 21));
        progressBar->setValue(0);
        progressBar->setTextVisible(false);
        SplitMixed = new QCheckBox(scrollAreaWidgetContents);
        SplitMixed->setObjectName(QString::fromUtf8("SplitMixed"));
        SplitMixed->setGeometry(QRect(8, 18, 470, 18));
        DefaultOutputFolder = new QCheckBox(scrollAreaWidgetContents);
        DefaultOutputFolder->setObjectName(QString::fromUtf8("DefaultOutputFolder"));
        DefaultOutputFolder->setGeometry(QRect(8, 45, 470, 18));
        groupBoxExport = new QGroupBox(scrollAreaWidgetContents);
        groupBoxExport->setObjectName(QString::fromUtf8("groupBoxExport"));
        groupBoxExport->setGeometry(QRect(8, 80, 468, 60));
        ExportLayout = new QHBoxLayout(groupBoxExport);
        ExportLayout->setObjectName(QString::fromUtf8("ExportLayout"));
        outExportDirLine = new QLineEdit(groupBoxExport);
        outExportDirLine->setObjectName(QString::fromUtf8("outExportDirLine"));

        ExportLayout->addWidget(outExportDirLine);

        outExportDirBrowseBtn = new QPushButton(groupBoxExport);
        outExportDirBrowseBtn->setObjectName(QString::fromUtf8("outExportDirBrowseBtn"));

        ExportLayout->addWidget(outExportDirBrowseBtn);

        ExportButton = new QPushButton(scrollAreaWidgetContents);
        ExportButton->setObjectName(QString::fromUtf8("ExportButton"));
        ExportButton->setGeometry(QRect(290, 178, 105, 25));
        OkButton = new QPushButton(scrollAreaWidgetContents);
        OkButton->setObjectName(QString::fromUtf8("OkButton"));
        OkButton->setGeometry(QRect(397, 178, 81, 25));
        labelFilesProcessed = new QLabel(scrollAreaWidgetContents);
        labelFilesProcessed->setObjectName(QString::fromUtf8("labelFilesProcessed"));
        labelFilesProcessed->setGeometry(QRect(7, 183, 279, 16));
        scrollArea->setWidget(scrollAreaWidgetContents);
        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        GenerateBlankBackSubscans = new QCheckBox(tab_2);
        GenerateBlankBackSubscans->setObjectName(QString::fromUtf8("GenerateBlankBackSubscans"));
        GenerateBlankBackSubscans->setGeometry(QRect(7, 15, 469, 18));
        KeepOriginalColorIllumForeSubscans = new QCheckBox(tab_2);
        KeepOriginalColorIllumForeSubscans->setObjectName(QString::fromUtf8("KeepOriginalColorIllumForeSubscans"));
        KeepOriginalColorIllumForeSubscans->setGeometry(QRect(7, 42, 468, 18));
        tabWidget->addTab(tab_2, QString());

        retranslateUi(ExportDialog);
        QObject::connect(OkButton, SIGNAL(clicked()), ExportDialog, SLOT(accept()));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(ExportDialog);
    } // setupUi

    void retranslateUi(QDialog *ExportDialog)
    {
        ExportDialog->setWindowTitle(QApplication::translate("ExportDialog", "Export", 0, QApplication::UnicodeUTF8));
        SplitMixed->setText(QApplication::translate("ExportDialog", "Split mixed output", 0, QApplication::UnicodeUTF8));
        DefaultOutputFolder->setText(QApplication::translate("ExportDialog", "Default export folder", 0, QApplication::UnicodeUTF8));
        groupBoxExport->setTitle(QApplication::translate("ExportDialog", "Output Directory", 0, QApplication::UnicodeUTF8));
        outExportDirBrowseBtn->setText(QApplication::translate("ExportDialog", "Browse", 0, QApplication::UnicodeUTF8));
        ExportButton->setText(QApplication::translate("ExportDialog", "Export", 0, QApplication::UnicodeUTF8));
        OkButton->setText(QApplication::translate("ExportDialog", "Close", 0, QApplication::UnicodeUTF8));
        labelFilesProcessed->setText(QApplication::translate("ExportDialog", "TextLabel", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("ExportDialog", "Tab 1", 0, QApplication::UnicodeUTF8));
        GenerateBlankBackSubscans->setText(QApplication::translate("ExportDialog", "Generate blank background subscans", 0, QApplication::UnicodeUTF8));
        KeepOriginalColorIllumForeSubscans->setText(QApplication::translate("ExportDialog", "Keep the original color and illumination in foreground subscans (lengthy)", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("ExportDialog", "Tab 2", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ExportDialog: public Ui_ExportDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_EXPORTDIALOG_H
