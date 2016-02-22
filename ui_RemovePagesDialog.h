/********************************************************************************
** Form generated from reading UI file 'RemovePagesDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_REMOVEPAGESDIALOG_H
#define UI_REMOVEPAGESDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_RemovePagesDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *icon;
    QSpacerItem *horizontalSpacer;
    QLabel *text;
    QLabel *multiPageWarning;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *RemovePagesDialog)
    {
        if (RemovePagesDialog->objectName().isEmpty())
            RemovePagesDialog->setObjectName(QString::fromUtf8("RemovePagesDialog"));
        RemovePagesDialog->resize(352, 111);
        verticalLayout = new QVBoxLayout(RemovePagesDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        icon = new QLabel(RemovePagesDialog);
        icon->setObjectName(QString::fromUtf8("icon"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(icon->sizePolicy().hasHeightForWidth());
        icon->setSizePolicy(sizePolicy);
        icon->setText(QString::fromUtf8("ICON"));

        horizontalLayout->addWidget(icon);

        horizontalSpacer = new QSpacerItem(10, 20, QSizePolicy::Preferred, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        text = new QLabel(RemovePagesDialog);
        text->setObjectName(QString::fromUtf8("text"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(text->sizePolicy().hasHeightForWidth());
        text->setSizePolicy(sizePolicy1);
        text->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

        horizontalLayout->addWidget(text);


        verticalLayout->addLayout(horizontalLayout);

        multiPageWarning = new QLabel(RemovePagesDialog);
        multiPageWarning->setObjectName(QString::fromUtf8("multiPageWarning"));
        multiPageWarning->setEnabled(true);
        multiPageWarning->setAlignment(Qt::AlignCenter);

        verticalLayout->addWidget(multiPageWarning);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(RemovePagesDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(RemovePagesDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), RemovePagesDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), RemovePagesDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(RemovePagesDialog);
    } // setupUi

    void retranslateUi(QDialog *RemovePagesDialog)
    {
        RemovePagesDialog->setWindowTitle(QApplication::translate("RemovePagesDialog", "Remove Pages", 0, QApplication::UnicodeUTF8));
        text->setText(QApplication::translate("RemovePagesDialog", "Remove %1 page(s) from project?", 0, QApplication::UnicodeUTF8));
        multiPageWarning->setText(QApplication::translate("RemovePagesDialog", "Corresponding output files will be deleted, while input files will remain.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class RemovePagesDialog: public Ui_RemovePagesDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_REMOVEPAGESDIALOG_H
