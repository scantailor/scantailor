/********************************************************************************
** Form generated from reading UI file 'FixDpiDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_FIXDPIDIALOG_H
#define UI_FIXDPIDIALOG_H

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
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QTreeView>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_FixDpiDialog
{
public:
    QVBoxLayout *vboxLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *vboxLayout1;
    QTreeView *undefinedDpiView;
    QWidget *tab_2;
    QVBoxLayout *vboxLayout2;
    QTreeView *allPagesView;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QComboBox *dpiCombo;
    QLineEdit *xDpi;
    QLabel *label_3;
    QLineEdit *yDpi;
    QPushButton *applyBtn;
    QSpacerItem *spacerItem1;
    QSpacerItem *spacerItem2;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *FixDpiDialog)
    {
        if (FixDpiDialog->objectName().isEmpty())
            FixDpiDialog->setObjectName(QString::fromUtf8("FixDpiDialog"));
        FixDpiDialog->resize(335, 354);
        vboxLayout = new QVBoxLayout(FixDpiDialog);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        tabWidget = new QTabWidget(FixDpiDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        vboxLayout1 = new QVBoxLayout(tab);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        undefinedDpiView = new QTreeView(tab);
        undefinedDpiView->setObjectName(QString::fromUtf8("undefinedDpiView"));

        vboxLayout1->addWidget(undefinedDpiView);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        vboxLayout2 = new QVBoxLayout(tab_2);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        allPagesView = new QTreeView(tab_2);
        allPagesView->setObjectName(QString::fromUtf8("allPagesView"));

        vboxLayout2->addWidget(allPagesView);

        tabWidget->addTab(tab_2, QString());

        vboxLayout->addWidget(tabWidget);

        groupBox = new QGroupBox(FixDpiDialog);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        hboxLayout = new QHBoxLayout(groupBox);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacerItem = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        dpiCombo = new QComboBox(groupBox);
        dpiCombo->setObjectName(QString::fromUtf8("dpiCombo"));
        dpiCombo->setEnabled(false);

        hboxLayout->addWidget(dpiCombo);

        xDpi = new QLineEdit(groupBox);
        xDpi->setObjectName(QString::fromUtf8("xDpi"));
        xDpi->setEnabled(false);
        xDpi->setMaximumSize(QSize(45, 16777215));

        hboxLayout->addWidget(xDpi);

        label_3 = new QLabel(groupBox);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        hboxLayout->addWidget(label_3);

        yDpi = new QLineEdit(groupBox);
        yDpi->setObjectName(QString::fromUtf8("yDpi"));
        yDpi->setEnabled(false);
        yDpi->setMaximumSize(QSize(45, 16777215));

        hboxLayout->addWidget(yDpi);

        applyBtn = new QPushButton(groupBox);
        applyBtn->setObjectName(QString::fromUtf8("applyBtn"));
        applyBtn->setEnabled(false);

        hboxLayout->addWidget(applyBtn);

        spacerItem1 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);


        vboxLayout->addWidget(groupBox);

        spacerItem2 = new QSpacerItem(317, 16, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem2);

        buttonBox = new QDialogButtonBox(FixDpiDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::NoButton|QDialogButtonBox::Ok);

        vboxLayout->addWidget(buttonBox);


        retranslateUi(FixDpiDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), FixDpiDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), FixDpiDialog, SLOT(reject()));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(FixDpiDialog);
    } // setupUi

    void retranslateUi(QDialog *FixDpiDialog)
    {
        FixDpiDialog->setWindowTitle(QApplication::translate("FixDpiDialog", "Fix DPI", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("FixDpiDialog", "Tab 1", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("FixDpiDialog", "Tab 2", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("FixDpiDialog", "DPI", 0, QApplication::UnicodeUTF8));
        dpiCombo->clear();
        dpiCombo->insertItems(0, QStringList()
         << QApplication::translate("FixDpiDialog", "Custom", 0, QApplication::UnicodeUTF8)
        );
        label_3->setText(QApplication::translate("FixDpiDialog", "x", 0, QApplication::UnicodeUTF8));
        applyBtn->setText(QApplication::translate("FixDpiDialog", "Apply", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class FixDpiDialog: public Ui_FixDpiDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_FIXDPIDIALOG_H
