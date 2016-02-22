/********************************************************************************
** Form generated from reading UI file 'PageLayoutApplyDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PAGELAYOUTAPPLYDIALOG_H
#define UI_PAGELAYOUTAPPLYDIALOG_H

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

class Ui_PageLayoutApplyDialog
{
public:
    QVBoxLayout *verticalLayout_5;
    QGroupBox *groupBox_2;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *thisPageRB;
    QRadioButton *allPagesRB;
    QRadioButton *thisPageAndFollowersRB;
    QWidget *everyOtherWidget;
    QVBoxLayout *verticalLayout_3;
    QRadioButton *everyOtherRB;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_2;
    QWidget *selectedPagesWidget;
    QVBoxLayout *verticalLayout;
    QRadioButton *selectedPagesRB;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_5;
    QLabel *selectedPagesHint;
    QWidget *everyOtherSelectedWidget;
    QVBoxLayout *verticalLayout_4;
    QRadioButton *everyOtherSelectedRB;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_6;
    QLabel *everyOtherSelectedHint;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *PageLayoutApplyDialog)
    {
        if (PageLayoutApplyDialog->objectName().isEmpty())
            PageLayoutApplyDialog->setObjectName(QString::fromUtf8("PageLayoutApplyDialog"));
        PageLayoutApplyDialog->setWindowModality(Qt::WindowModal);
        PageLayoutApplyDialog->resize(363, 316);
        verticalLayout_5 = new QVBoxLayout(PageLayoutApplyDialog);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        groupBox_2 = new QGroupBox(PageLayoutApplyDialog);
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

        everyOtherWidget = new QWidget(groupBox_2);
        everyOtherWidget->setObjectName(QString::fromUtf8("everyOtherWidget"));
        verticalLayout_3 = new QVBoxLayout(everyOtherWidget);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        everyOtherRB = new QRadioButton(everyOtherWidget);
        everyOtherRB->setObjectName(QString::fromUtf8("everyOtherRB"));

        verticalLayout_3->addWidget(everyOtherRB);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_2 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_2);

        label_2 = new QLabel(everyOtherWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QFont font;
        font.setPointSize(7);
        label_2->setFont(font);

        horizontalLayout_3->addWidget(label_2);


        verticalLayout_3->addLayout(horizontalLayout_3);


        verticalLayout_2->addWidget(everyOtherWidget);

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
        horizontalLayout_2->setSpacing(6);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_5 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_5);

        selectedPagesHint = new QLabel(selectedPagesWidget);
        selectedPagesHint->setObjectName(QString::fromUtf8("selectedPagesHint"));
        selectedPagesHint->setFont(font);

        horizontalLayout_2->addWidget(selectedPagesHint);


        verticalLayout->addLayout(horizontalLayout_2);


        verticalLayout_2->addWidget(selectedPagesWidget);

        everyOtherSelectedWidget = new QWidget(groupBox_2);
        everyOtherSelectedWidget->setObjectName(QString::fromUtf8("everyOtherSelectedWidget"));
        verticalLayout_4 = new QVBoxLayout(everyOtherSelectedWidget);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        everyOtherSelectedRB = new QRadioButton(everyOtherSelectedWidget);
        everyOtherSelectedRB->setObjectName(QString::fromUtf8("everyOtherSelectedRB"));

        verticalLayout_4->addWidget(everyOtherSelectedRB);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_6 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_6);

        everyOtherSelectedHint = new QLabel(everyOtherSelectedWidget);
        everyOtherSelectedHint->setObjectName(QString::fromUtf8("everyOtherSelectedHint"));
        everyOtherSelectedHint->setFont(font);

        horizontalLayout_5->addWidget(everyOtherSelectedHint);


        verticalLayout_4->addLayout(horizontalLayout_5);


        verticalLayout_2->addWidget(everyOtherSelectedWidget);


        verticalLayout_5->addWidget(groupBox_2);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(PageLayoutApplyDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout_5->addWidget(buttonBox);


        retranslateUi(PageLayoutApplyDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), PageLayoutApplyDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(PageLayoutApplyDialog);
    } // setupUi

    void retranslateUi(QDialog *PageLayoutApplyDialog)
    {
        groupBox_2->setTitle(QApplication::translate("PageLayoutApplyDialog", "Apply to", 0, QApplication::UnicodeUTF8));
        thisPageRB->setText(QApplication::translate("PageLayoutApplyDialog", "This page only (already applied)", 0, QApplication::UnicodeUTF8));
        allPagesRB->setText(QApplication::translate("PageLayoutApplyDialog", "All pages", 0, QApplication::UnicodeUTF8));
        thisPageAndFollowersRB->setText(QApplication::translate("PageLayoutApplyDialog", "This page and the following ones", 0, QApplication::UnicodeUTF8));
        everyOtherRB->setText(QApplication::translate("PageLayoutApplyDialog", "Every other page", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("PageLayoutApplyDialog", "The current page will be included.", 0, QApplication::UnicodeUTF8));
        selectedPagesRB->setText(QApplication::translate("PageLayoutApplyDialog", "Selected pages", 0, QApplication::UnicodeUTF8));
        selectedPagesHint->setText(QApplication::translate("PageLayoutApplyDialog", "Use Ctrl+Click / Shift+Click to select multiple pages.", 0, QApplication::UnicodeUTF8));
        everyOtherSelectedRB->setText(QApplication::translate("PageLayoutApplyDialog", "Every other selected page", 0, QApplication::UnicodeUTF8));
        everyOtherSelectedHint->setText(QApplication::translate("PageLayoutApplyDialog", "The current page will be included.", 0, QApplication::UnicodeUTF8));
        Q_UNUSED(PageLayoutApplyDialog);
    } // retranslateUi

};

namespace Ui {
    class PageLayoutApplyDialog: public Ui_PageLayoutApplyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PAGELAYOUTAPPLYDIALOG_H
