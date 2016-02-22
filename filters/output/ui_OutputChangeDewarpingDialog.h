/********************************************************************************
** Form generated from reading UI file 'OutputChangeDewarpingDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OUTPUTCHANGEDEWARPINGDIALOG_H
#define UI_OUTPUTCHANGEDEWARPINGDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OutputChangeDewarpingDialog
{
public:
    QVBoxLayout *verticalLayout;
    QGroupBox *dpiGroup;
    QGridLayout *gridLayout;
    QRadioButton *offRB;
    QRadioButton *autoRB;
    QRadioButton *manualRB;
    QRadioButton *marginalRB;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout_4;
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

    void setupUi(QDialog *OutputChangeDewarpingDialog)
    {
        if (OutputChangeDewarpingDialog->objectName().isEmpty())
            OutputChangeDewarpingDialog->setObjectName(QString::fromUtf8("OutputChangeDewarpingDialog"));
        OutputChangeDewarpingDialog->setWindowModality(Qt::WindowModal);
        OutputChangeDewarpingDialog->resize(320, 324);
        verticalLayout = new QVBoxLayout(OutputChangeDewarpingDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        dpiGroup = new QGroupBox(OutputChangeDewarpingDialog);
        dpiGroup->setObjectName(QString::fromUtf8("dpiGroup"));
        gridLayout = new QGridLayout(dpiGroup);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        offRB = new QRadioButton(dpiGroup);
        offRB->setObjectName(QString::fromUtf8("offRB"));
        offRB->setChecked(true);

        gridLayout->addWidget(offRB, 0, 0, 1, 1);

        autoRB = new QRadioButton(dpiGroup);
        autoRB->setObjectName(QString::fromUtf8("autoRB"));
        autoRB->setEnabled(true);

        gridLayout->addWidget(autoRB, 1, 0, 1, 1);

        manualRB = new QRadioButton(dpiGroup);
        manualRB->setObjectName(QString::fromUtf8("manualRB"));
        manualRB->setEnabled(true);

        gridLayout->addWidget(manualRB, 3, 0, 1, 1);

        marginalRB = new QRadioButton(dpiGroup);
        marginalRB->setObjectName(QString::fromUtf8("marginalRB"));
        marginalRB->setEnabled(true);

        gridLayout->addWidget(marginalRB, 2, 0, 1, 1);


        verticalLayout->addWidget(dpiGroup);

        groupBox_3 = new QGroupBox(OutputChangeDewarpingDialog);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout_4 = new QVBoxLayout(groupBox_3);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        thisPageRB = new QRadioButton(groupBox_3);
        thisPageRB->setObjectName(QString::fromUtf8("thisPageRB"));
        thisPageRB->setChecked(true);

        verticalLayout_4->addWidget(thisPageRB);

        allPagesRB = new QRadioButton(groupBox_3);
        allPagesRB->setObjectName(QString::fromUtf8("allPagesRB"));

        verticalLayout_4->addWidget(allPagesRB);

        thisPageAndFollowersRB = new QRadioButton(groupBox_3);
        thisPageAndFollowersRB->setObjectName(QString::fromUtf8("thisPageAndFollowersRB"));

        verticalLayout_4->addWidget(thisPageAndFollowersRB);

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


        verticalLayout_4->addWidget(selectedPagesWidget);


        verticalLayout->addWidget(groupBox_3);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(OutputChangeDewarpingDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(OutputChangeDewarpingDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), OutputChangeDewarpingDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(OutputChangeDewarpingDialog);
    } // setupUi

    void retranslateUi(QDialog *OutputChangeDewarpingDialog)
    {
        OutputChangeDewarpingDialog->setWindowTitle(QApplication::translate("OutputChangeDewarpingDialog", "Apply Dewarping Mode", 0, QApplication::UnicodeUTF8));
        dpiGroup->setTitle(QApplication::translate("OutputChangeDewarpingDialog", "Mode", 0, QApplication::UnicodeUTF8));
        offRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "Off", 0, QApplication::UnicodeUTF8));
        autoRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "Auto (experimental)", 0, QApplication::UnicodeUTF8));
        manualRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "Manual", 0, QApplication::UnicodeUTF8));
        marginalRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "Marginal (experimental)", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("OutputChangeDewarpingDialog", "Scope", 0, QApplication::UnicodeUTF8));
        thisPageRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "This page only", 0, QApplication::UnicodeUTF8));
        allPagesRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "All pages", 0, QApplication::UnicodeUTF8));
        thisPageAndFollowersRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "This page and the following ones", 0, QApplication::UnicodeUTF8));
        selectedPagesRB->setText(QApplication::translate("OutputChangeDewarpingDialog", "Selected pages", 0, QApplication::UnicodeUTF8));
        selectedPagesHint->setText(QApplication::translate("OutputChangeDewarpingDialog", "Use Ctrl+Click / Shift+Click to select multiple pages.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OutputChangeDewarpingDialog: public Ui_OutputChangeDewarpingDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OUTPUTCHANGEDEWARPINGDIALOG_H
