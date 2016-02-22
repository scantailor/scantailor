/********************************************************************************
** Form generated from reading UI file 'OrientationApplyDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ORIENTATIONAPPLYDIALOG_H
#define UI_ORIENTATIONAPPLYDIALOG_H

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

class Ui_OrientationApplyDialog
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout_5;
    QRadioButton *thisPageOnlyRB;
    QRadioButton *allPagesRB;
    QRadioButton *thisPageAndFollowersRB;
    QWidget *everyOtherPageWidget;
    QVBoxLayout *verticalLayout_3;
    QWidget *everyOtherWidget;
    QVBoxLayout *verticalLayout_2;
    QRadioButton *everyOtherRB;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_2;
    QWidget *selectedPagesWidget;
    QVBoxLayout *verticalLayout_4;
    QRadioButton *selectedPagesRB;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_6;
    QLabel *selectedPagesHint;
    QWidget *everyOtherSelectedWidget;
    QVBoxLayout *verticalLayout;
    QRadioButton *everyOtherSelectedRB;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_5;
    QLabel *everyOtherSelectedHint;
    QSpacerItem *spacerItem;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OrientationApplyDialog)
    {
        if (OrientationApplyDialog->objectName().isEmpty())
            OrientationApplyDialog->setObjectName(QString::fromUtf8("OrientationApplyDialog"));
        OrientationApplyDialog->setWindowModality(Qt::WindowModal);
        OrientationApplyDialog->resize(364, 316);
        vboxLayout = new QVBoxLayout(OrientationApplyDialog);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(OrientationApplyDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout_5 = new QVBoxLayout(groupBox);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        thisPageOnlyRB = new QRadioButton(groupBox);
        thisPageOnlyRB->setObjectName(QString::fromUtf8("thisPageOnlyRB"));
        thisPageOnlyRB->setChecked(true);

        verticalLayout_5->addWidget(thisPageOnlyRB);

        allPagesRB = new QRadioButton(groupBox);
        allPagesRB->setObjectName(QString::fromUtf8("allPagesRB"));

        verticalLayout_5->addWidget(allPagesRB);

        thisPageAndFollowersRB = new QRadioButton(groupBox);
        thisPageAndFollowersRB->setObjectName(QString::fromUtf8("thisPageAndFollowersRB"));

        verticalLayout_5->addWidget(thisPageAndFollowersRB);

        everyOtherPageWidget = new QWidget(groupBox);
        everyOtherPageWidget->setObjectName(QString::fromUtf8("everyOtherPageWidget"));
        verticalLayout_3 = new QVBoxLayout(everyOtherPageWidget);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        everyOtherWidget = new QWidget(everyOtherPageWidget);
        everyOtherWidget->setObjectName(QString::fromUtf8("everyOtherWidget"));
        verticalLayout_2 = new QVBoxLayout(everyOtherWidget);
        verticalLayout_2->setSpacing(0);
        verticalLayout_2->setContentsMargins(0, 0, 0, 0);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        everyOtherRB = new QRadioButton(everyOtherWidget);
        everyOtherRB->setObjectName(QString::fromUtf8("everyOtherRB"));

        verticalLayout_2->addWidget(everyOtherRB);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_2 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        label_2 = new QLabel(everyOtherWidget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        QFont font;
        font.setPointSize(7);
        label_2->setFont(font);

        horizontalLayout_2->addWidget(label_2);


        verticalLayout_2->addLayout(horizontalLayout_2);


        verticalLayout_3->addWidget(everyOtherWidget);


        verticalLayout_5->addWidget(everyOtherPageWidget);

        selectedPagesWidget = new QWidget(groupBox);
        selectedPagesWidget->setObjectName(QString::fromUtf8("selectedPagesWidget"));
        verticalLayout_4 = new QVBoxLayout(selectedPagesWidget);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        selectedPagesRB = new QRadioButton(selectedPagesWidget);
        selectedPagesRB->setObjectName(QString::fromUtf8("selectedPagesRB"));

        verticalLayout_4->addWidget(selectedPagesRB);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_6 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_6);

        selectedPagesHint = new QLabel(selectedPagesWidget);
        selectedPagesHint->setObjectName(QString::fromUtf8("selectedPagesHint"));
        selectedPagesHint->setFont(font);

        horizontalLayout_3->addWidget(selectedPagesHint);


        verticalLayout_4->addLayout(horizontalLayout_3);


        verticalLayout_5->addWidget(selectedPagesWidget);

        everyOtherSelectedWidget = new QWidget(groupBox);
        everyOtherSelectedWidget->setObjectName(QString::fromUtf8("everyOtherSelectedWidget"));
        verticalLayout = new QVBoxLayout(everyOtherSelectedWidget);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        everyOtherSelectedRB = new QRadioButton(everyOtherSelectedWidget);
        everyOtherSelectedRB->setObjectName(QString::fromUtf8("everyOtherSelectedRB"));

        verticalLayout->addWidget(everyOtherSelectedRB);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_5 = new QSpacerItem(30, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_5);

        everyOtherSelectedHint = new QLabel(everyOtherSelectedWidget);
        everyOtherSelectedHint->setObjectName(QString::fromUtf8("everyOtherSelectedHint"));
        everyOtherSelectedHint->setFont(font);

        horizontalLayout_5->addWidget(everyOtherSelectedHint);


        verticalLayout->addLayout(horizontalLayout_5);


        verticalLayout_5->addWidget(everyOtherSelectedWidget);


        vboxLayout->addWidget(groupBox);

        spacerItem = new QSpacerItem(17, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem);

        buttonBox = new QDialogButtonBox(OrientationApplyDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(OrientationApplyDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), OrientationApplyDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(OrientationApplyDialog);
    } // setupUi

    void retranslateUi(QDialog *OrientationApplyDialog)
    {
        OrientationApplyDialog->setWindowTitle(QApplication::translate("OrientationApplyDialog", "Fix Orientation", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("OrientationApplyDialog", "Apply to", 0, QApplication::UnicodeUTF8));
        thisPageOnlyRB->setText(QApplication::translate("OrientationApplyDialog", "This page only (already applied)", 0, QApplication::UnicodeUTF8));
        allPagesRB->setText(QApplication::translate("OrientationApplyDialog", "All pages", 0, QApplication::UnicodeUTF8));
        thisPageAndFollowersRB->setText(QApplication::translate("OrientationApplyDialog", "This page and the following ones", 0, QApplication::UnicodeUTF8));
        everyOtherRB->setText(QApplication::translate("OrientationApplyDialog", "Every other page", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("OrientationApplyDialog", "The current page will be included.", 0, QApplication::UnicodeUTF8));
        selectedPagesRB->setText(QApplication::translate("OrientationApplyDialog", "Selected pages", 0, QApplication::UnicodeUTF8));
        selectedPagesHint->setText(QApplication::translate("OrientationApplyDialog", "Use Ctrl+Click / Shift+Click to select multiple pages.", 0, QApplication::UnicodeUTF8));
        everyOtherSelectedRB->setText(QApplication::translate("OrientationApplyDialog", "Every other selected page", 0, QApplication::UnicodeUTF8));
        everyOtherSelectedHint->setText(QApplication::translate("OrientationApplyDialog", "The current page will be included.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OrientationApplyDialog: public Ui_OrientationApplyDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ORIENTATIONAPPLYDIALOG_H
