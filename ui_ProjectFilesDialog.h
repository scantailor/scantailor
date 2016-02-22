/********************************************************************************
** Form generated from reading UI file 'ProjectFilesDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PROJECTFILESDIALOG_H
#define UI_PROJECTFILESDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLineEdit>
#include <QtGui/QListView>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_ProjectFilesDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *groupBox_2;
    QHBoxLayout *hboxLayout;
    QLineEdit *inpDirLine;
    QPushButton *inpDirBrowseBtn;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout1;
    QLineEdit *outDirLine;
    QPushButton *outDirBrowseBtn;
    QHBoxLayout *hboxLayout2;
    QGroupBox *groupBox_3;
    QVBoxLayout *vboxLayout;
    QListView *offProjectList;
    QPushButton *offProjectSelectAllBtn;
    QVBoxLayout *vboxLayout1;
    QSpacerItem *spacerItem;
    QToolButton *addToProjectBtn;
    QToolButton *removeFromProjectBtn;
    QSpacerItem *spacerItem1;
    QGroupBox *groupBox_4;
    QVBoxLayout *vboxLayout2;
    QListView *inProjectList;
    QPushButton *inProjectSelectAllBtn;
    QCheckBox *rtlLayoutCB;
    QCheckBox *forceFixDpi;
    QProgressBar *progressBar;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *ProjectFilesDialog)
    {
        if (ProjectFilesDialog->objectName().isEmpty())
            ProjectFilesDialog->setObjectName(QString::fromUtf8("ProjectFilesDialog"));
        ProjectFilesDialog->resize(482, 506);
        verticalLayout = new QVBoxLayout(ProjectFilesDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        groupBox_2 = new QGroupBox(ProjectFilesDialog);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        hboxLayout = new QHBoxLayout(groupBox_2);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        inpDirLine = new QLineEdit(groupBox_2);
        inpDirLine->setObjectName(QString::fromUtf8("inpDirLine"));
        inpDirLine->setEnabled(true);

        hboxLayout->addWidget(inpDirLine);

        inpDirBrowseBtn = new QPushButton(groupBox_2);
        inpDirBrowseBtn->setObjectName(QString::fromUtf8("inpDirBrowseBtn"));

        hboxLayout->addWidget(inpDirBrowseBtn);


        verticalLayout->addWidget(groupBox_2);

        groupBox = new QGroupBox(ProjectFilesDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        hboxLayout1 = new QHBoxLayout(groupBox);
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        outDirLine = new QLineEdit(groupBox);
        outDirLine->setObjectName(QString::fromUtf8("outDirLine"));

        hboxLayout1->addWidget(outDirLine);

        outDirBrowseBtn = new QPushButton(groupBox);
        outDirBrowseBtn->setObjectName(QString::fromUtf8("outDirBrowseBtn"));

        hboxLayout1->addWidget(outDirBrowseBtn);


        verticalLayout->addWidget(groupBox);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        groupBox_3 = new QGroupBox(ProjectFilesDialog);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        vboxLayout = new QVBoxLayout(groupBox_3);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        offProjectList = new QListView(groupBox_3);
        offProjectList->setObjectName(QString::fromUtf8("offProjectList"));
        offProjectList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        vboxLayout->addWidget(offProjectList);

        offProjectSelectAllBtn = new QPushButton(groupBox_3);
        offProjectSelectAllBtn->setObjectName(QString::fromUtf8("offProjectSelectAllBtn"));

        vboxLayout->addWidget(offProjectSelectAllBtn);


        hboxLayout2->addWidget(groupBox_3);

        vboxLayout1 = new QVBoxLayout();
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem);

        addToProjectBtn = new QToolButton(ProjectFilesDialog);
        addToProjectBtn->setObjectName(QString::fromUtf8("addToProjectBtn"));

        vboxLayout1->addWidget(addToProjectBtn);

        removeFromProjectBtn = new QToolButton(ProjectFilesDialog);
        removeFromProjectBtn->setObjectName(QString::fromUtf8("removeFromProjectBtn"));

        vboxLayout1->addWidget(removeFromProjectBtn);

        spacerItem1 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout1->addItem(spacerItem1);


        hboxLayout2->addLayout(vboxLayout1);

        groupBox_4 = new QGroupBox(ProjectFilesDialog);
        groupBox_4->setObjectName(QString::fromUtf8("groupBox_4"));
        vboxLayout2 = new QVBoxLayout(groupBox_4);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        inProjectList = new QListView(groupBox_4);
        inProjectList->setObjectName(QString::fromUtf8("inProjectList"));
        inProjectList->setEnabled(true);
        inProjectList->setSelectionMode(QAbstractItemView::ExtendedSelection);

        vboxLayout2->addWidget(inProjectList);

        inProjectSelectAllBtn = new QPushButton(groupBox_4);
        inProjectSelectAllBtn->setObjectName(QString::fromUtf8("inProjectSelectAllBtn"));

        vboxLayout2->addWidget(inProjectSelectAllBtn);


        hboxLayout2->addWidget(groupBox_4);


        verticalLayout->addLayout(hboxLayout2);

        rtlLayoutCB = new QCheckBox(ProjectFilesDialog);
        rtlLayoutCB->setObjectName(QString::fromUtf8("rtlLayoutCB"));

        verticalLayout->addWidget(rtlLayoutCB);

        forceFixDpi = new QCheckBox(ProjectFilesDialog);
        forceFixDpi->setObjectName(QString::fromUtf8("forceFixDpi"));

        verticalLayout->addWidget(forceFixDpi);

        progressBar = new QProgressBar(ProjectFilesDialog);
        progressBar->setObjectName(QString::fromUtf8("progressBar"));
        progressBar->setValue(0);
        progressBar->setTextVisible(false);

        verticalLayout->addWidget(progressBar);

        buttonBox = new QDialogButtonBox(ProjectFilesDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(ProjectFilesDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), ProjectFilesDialog, SLOT(reject()));
        QObject::connect(offProjectSelectAllBtn, SIGNAL(clicked()), offProjectList, SLOT(selectAll()));
        QObject::connect(inProjectSelectAllBtn, SIGNAL(clicked()), inProjectList, SLOT(selectAll()));

        QMetaObject::connectSlotsByName(ProjectFilesDialog);
    } // setupUi

    void retranslateUi(QDialog *ProjectFilesDialog)
    {
        ProjectFilesDialog->setWindowTitle(QApplication::translate("ProjectFilesDialog", "Project Files", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("ProjectFilesDialog", "Input Directory", 0, QApplication::UnicodeUTF8));
        inpDirBrowseBtn->setText(QApplication::translate("ProjectFilesDialog", "Browse", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("ProjectFilesDialog", "Output Directory", 0, QApplication::UnicodeUTF8));
        outDirBrowseBtn->setText(QApplication::translate("ProjectFilesDialog", "Browse", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("ProjectFilesDialog", "Files Not In Project", 0, QApplication::UnicodeUTF8));
        offProjectSelectAllBtn->setText(QApplication::translate("ProjectFilesDialog", "Select All", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        addToProjectBtn->setToolTip(QApplication::translate("ProjectFilesDialog", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Add selected files to project.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        addToProjectBtn->setText(QApplication::translate("ProjectFilesDialog", ">>", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        removeFromProjectBtn->setToolTip(QApplication::translate("ProjectFilesDialog", "<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'Sans Serif'; font-size:9pt; font-weight:400; font-style:normal;\">\n"
"<p style=\" margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px;\">Remove selected files from project.</p></body></html>", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        removeFromProjectBtn->setText(QApplication::translate("ProjectFilesDialog", "<<", 0, QApplication::UnicodeUTF8));
        groupBox_4->setTitle(QApplication::translate("ProjectFilesDialog", "Files In Project", 0, QApplication::UnicodeUTF8));
        inProjectSelectAllBtn->setText(QApplication::translate("ProjectFilesDialog", "Select All", 0, QApplication::UnicodeUTF8));
        rtlLayoutCB->setText(QApplication::translate("ProjectFilesDialog", "Right to left layout (for Hebrew and Arabic)", 0, QApplication::UnicodeUTF8));
        forceFixDpi->setText(QApplication::translate("ProjectFilesDialog", "Fix DPIs, even if they look OK", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class ProjectFilesDialog: public Ui_ProjectFilesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PROJECTFILESDIALOG_H
