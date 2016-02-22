/********************************************************************************
** Form generated from reading UI file 'DeskewApplyDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DESKEWAPPLYDIALOG_H
#define UI_DESKEWAPPLYDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
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

class Ui_DeskewApplyDialog
{
public:
    QVBoxLayout *verticalLayout_3;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *thisPageRB;
    QRadioButton *allPagesRB;
    QRadioButton *thisPageAndFollowersRB;
    QWidget *selectedPagesWidget;
    QVBoxLayout *verticalLayout;
    QRadioButton *selectedPagesRB;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_5;
    QLabel *selectedPagesHint;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *DeskewApplyDialog)
    {
        if (DeskewApplyDialog->objectName().isEmpty())
            DeskewApplyDialog->setObjectName(QString::fromUtf8("DeskewApplyDialog"));
        DeskewApplyDialog->setWindowModality(Qt::WindowModal);
        DeskewApplyDialog->resize(320, 195);
        verticalLayout_3 = new QVBoxLayout(DeskewApplyDialog);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        groupBox_2 = new QGroupBox(DeskewApplyDialog);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        verticalLayout_2 = new QVBoxLayout(groupBox_2);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        thisPageRB = new QRadioButton(groupBox_2);
        thisPageRB->setObjectName(QString::fromUtf8("thisPageRB"));
        thisPageRB->setChecked(true);

        verticalLayout_2->addWidget(thisPageRB);

        allPagesRB = new QRadioButton(groupBox_2);
        allPagesRB->setObjectName(QString::fromUtf8("allPagesRB"));

        verticalLayout_2->addWidget(allPagesRB);

        thisPageAndFollowersRB = new QRadioButton(groupBox_2);
        thisPageAndFollowersRB->setObjectName(QString::fromUtf8("thisPageAndFollowersRB"));

        verticalLayout_2->addWidget(thisPageAndFollowersRB);

        selectedPagesWidget = new QWidget(groupBox_2);
        selectedPagesWidget->setObjectName(QString::fromUtf8("selectedPagesWidget"));
        verticalLayout = new QVBoxLayout(selectedPagesWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        selectedPagesRB = new QRadioButton(selectedPagesWidget);
        selectedPagesRB->setObjectName(QString::fromUtf8("selectedPagesRB"));

        verticalLayout->addWidget(selectedPagesRB);

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


        verticalLayout->addLayout(horizontalLayout_2);


        verticalLayout_2->addWidget(selectedPagesWidget);


        verticalLayout_3->addWidget(groupBox_2);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(DeskewApplyDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_3->addWidget(buttonBox);


        retranslateUi(DeskewApplyDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), DeskewApplyDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(DeskewApplyDialog);
    } // setupUi

    void retranslateUi(QDialog *DeskewApplyDialog)
    {
        groupBox_2->setTitle(QApplication::translate("DeskewApplyDialog", "Apply to", 0, QApplication::UnicodeUTF8));
        thisPageRB->setText(QApplication::translate("DeskewApplyDialog", "This page only (already applied)", 0, QApplication::UnicodeUTF8));
        allPagesRB->setText(QApplication::translate("DeskewApplyDialog", "All pages", 0, QApplication::UnicodeUTF8));
        thisPageAndFollowersRB->setText(QApplication::translate("DeskewApplyDialog", "This page and the following ones", 0, QApplication::UnicodeUTF8));
        selectedPagesRB->setText(QApplication::translate("DeskewApplyDialog", "Selected pages", 0, QApplication::UnicodeUTF8));
        selectedPagesHint->setText(QApplication::translate("DeskewApplyDialog", "Use Ctrl+Click / Shift+Click to select multiple pages.", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(DeskewApplyDialog);
    } // retranslateUi

};

namespace Ui {
    class DeskewApplyDialog: public Ui_DeskewApplyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DESKEWAPPLYDIALOG_H
