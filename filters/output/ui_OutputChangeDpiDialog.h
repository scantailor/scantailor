/********************************************************************************
** Form generated from reading UI file 'OutputChangeDpiDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OUTPUTCHANGEDPIDIALOG_H
#define UI_OUTPUTCHANGEDPIDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OutputChangeDpiDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *dpiGroup;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_3;
    QComboBox *dpiSelector;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *thisPageRB;
    QRadioButton *allPagesRB;
    QRadioButton *thisPageAndFollowersRB;
    QWidget *selectedPagesWidget;
    QVBoxLayout *verticalLayout_3;
    QRadioButton *selectedPagesRB;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_5;
    QLabel *selectedPagesHint;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OutputChangeDpiDialog)
    {
        if (OutputChangeDpiDialog->objectName().isEmpty())
            OutputChangeDpiDialog->setObjectName(QString::fromUtf8("OutputChangeDpiDialog"));
        OutputChangeDpiDialog->setWindowModality(Qt::WindowModal);
        OutputChangeDpiDialog->resize(320, 266);
        verticalLayout = new QVBoxLayout(OutputChangeDpiDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        dpiGroup = new QGroupBox(OutputChangeDpiDialog);
        dpiGroup->setObjectName(QString::fromUtf8("dpiGroup"));
        horizontalLayout = new QHBoxLayout(dpiGroup);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_3 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);

        dpiSelector = new QComboBox(dpiGroup);
        dpiSelector->setObjectName(QString::fromUtf8("dpiSelector"));
        dpiSelector->setInsertPolicy(QComboBox::NoInsert);
        dpiSelector->setDuplicatesEnabled(true);

        horizontalLayout->addWidget(dpiSelector);

        horizontalSpacer_4 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);


        verticalLayout->addWidget(dpiGroup);

        groupBox_3 = new QGroupBox(OutputChangeDpiDialog);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_2 = new QVBoxLayout(groupBox_3);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        thisPageRB = new QRadioButton(groupBox_3);
        thisPageRB->setObjectName(QString::fromUtf8("thisPageRB"));
        thisPageRB->setChecked(true);

        verticalLayout_2->addWidget(thisPageRB);

        allPagesRB = new QRadioButton(groupBox_3);
        allPagesRB->setObjectName(QString::fromUtf8("allPagesRB"));

        verticalLayout_2->addWidget(allPagesRB);

        thisPageAndFollowersRB = new QRadioButton(groupBox_3);
        thisPageAndFollowersRB->setObjectName(QString::fromUtf8("thisPageAndFollowersRB"));

        verticalLayout_2->addWidget(thisPageAndFollowersRB);

        selectedPagesWidget = new QWidget(groupBox_3);
        selectedPagesWidget->setObjectName(QString::fromUtf8("selectedPagesWidget"));
        verticalLayout_3 = new QVBoxLayout(selectedPagesWidget);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        selectedPagesRB = new QRadioButton(selectedPagesWidget);
        selectedPagesRB->setObjectName(QString::fromUtf8("selectedPagesRB"));

        verticalLayout_3->addWidget(selectedPagesRB);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setSpacing(0);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_5 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_5);

        selectedPagesHint = new QLabel(selectedPagesWidget);
        selectedPagesHint->setObjectName(QString::fromUtf8("selectedPagesHint"));
        QFont font;
        font.setPointSize(7);
        selectedPagesHint->setFont(font);

        horizontalLayout_2->addWidget(selectedPagesHint);


        verticalLayout_3->addLayout(horizontalLayout_2);


        verticalLayout_2->addWidget(selectedPagesWidget);


        verticalLayout->addWidget(groupBox_3);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(OutputChangeDpiDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(OutputChangeDpiDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), OutputChangeDpiDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(OutputChangeDpiDialog);
    } // setupUi

    void retranslateUi(QDialog *OutputChangeDpiDialog)
    {
        OutputChangeDpiDialog->setWindowTitle(QApplication::translate("OutputChangeDpiDialog", "Apply Output Resolution", 0, QApplication::UnicodeUTF8));
        dpiGroup->setTitle(QApplication::translate("OutputChangeDpiDialog", "DPI", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("OutputChangeDpiDialog", "Scope", 0, QApplication::UnicodeUTF8));
        thisPageRB->setText(QApplication::translate("OutputChangeDpiDialog", "This page only", 0, QApplication::UnicodeUTF8));
        allPagesRB->setText(QApplication::translate("OutputChangeDpiDialog", "All pages", 0, QApplication::UnicodeUTF8));
        thisPageAndFollowersRB->setText(QApplication::translate("OutputChangeDpiDialog", "This page and the following ones", 0, QApplication::UnicodeUTF8));
        selectedPagesRB->setText(QApplication::translate("OutputChangeDpiDialog", "Selected pages", 0, QApplication::UnicodeUTF8));
        selectedPagesHint->setText(QApplication::translate("OutputChangeDpiDialog", "Use Ctrl+Click / Shift+Click to select multiple pages.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OutputChangeDpiDialog: public Ui_OutputChangeDpiDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OUTPUTCHANGEDPIDIALOG_H
