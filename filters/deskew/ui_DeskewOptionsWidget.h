/********************************************************************************
** Form generated from reading UI file 'DeskewOptionsWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DESKEWOPTIONSWIDGET_H
#define UI_DESKEWOPTIONSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_DeskewOptionsWidget
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QPushButton *autoBtn;
    QPushButton *manualBtn;
    QSpacerItem *spacerItem1;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem2;
    QDoubleSpinBox *angleSpinBox;
    QSpacerItem *spacerItem3;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_9;
    QPushButton *applyDeskewBtn;
    QSpacerItem *horizontalSpacer_10;
    QSpacerItem *spacerItem4;

    void setupUi(QWidget *DeskewOptionsWidget)
    {
        if (DeskewOptionsWidget->objectName().isEmpty())
            DeskewOptionsWidget->setObjectName(QString::fromUtf8("DeskewOptionsWidget"));
        DeskewOptionsWidget->resize(220, 238);
        vboxLayout = new QVBoxLayout(DeskewOptionsWidget);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(DeskewOptionsWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        verticalLayout = new QVBoxLayout(groupBox);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacerItem = new QSpacerItem(1, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        autoBtn = new QPushButton(groupBox);
        autoBtn->setObjectName(QString::fromUtf8("autoBtn"));
        autoBtn->setCheckable(true);
        autoBtn->setChecked(true);
        autoBtn->setAutoExclusive(true);

        hboxLayout->addWidget(autoBtn);

        manualBtn = new QPushButton(groupBox);
        manualBtn->setObjectName(QString::fromUtf8("manualBtn"));
        manualBtn->setCheckable(true);
        manualBtn->setAutoExclusive(true);

        hboxLayout->addWidget(manualBtn);

        spacerItem1 = new QSpacerItem(1, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);


        verticalLayout->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem2);

        angleSpinBox = new QDoubleSpinBox(groupBox);
        angleSpinBox->setObjectName(QString::fromUtf8("angleSpinBox"));
        angleSpinBox->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        angleSpinBox->setSingleStep(0.01);

        hboxLayout1->addWidget(angleSpinBox);

        spacerItem3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem3);


        verticalLayout->addLayout(hboxLayout1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_9 = new QSpacerItem(38, 18, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_9);

        applyDeskewBtn = new QPushButton(groupBox);
        applyDeskewBtn->setObjectName(QString::fromUtf8("applyDeskewBtn"));

        horizontalLayout->addWidget(applyDeskewBtn);

        horizontalSpacer_10 = new QSpacerItem(28, 18, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_10);


        verticalLayout->addLayout(horizontalLayout);


        vboxLayout->addWidget(groupBox);

        spacerItem4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem4);


        retranslateUi(DeskewOptionsWidget);

        QMetaObject::connectSlotsByName(DeskewOptionsWidget);
    } // setupUi

    void retranslateUi(QWidget *DeskewOptionsWidget)
    {
        DeskewOptionsWidget->setWindowTitle(QApplication::translate("DeskewOptionsWidget", "Form", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("DeskewOptionsWidget", "Deskew", 0, QApplication::UnicodeUTF8));
        autoBtn->setText(QApplication::translate("DeskewOptionsWidget", "Auto", 0, QApplication::UnicodeUTF8));
        manualBtn->setText(QApplication::translate("DeskewOptionsWidget", "Manual", 0, QApplication::UnicodeUTF8));
        applyDeskewBtn->setText(QApplication::translate("DeskewOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class DeskewOptionsWidget: public Ui_DeskewOptionsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DESKEWOPTIONSWIDGET_H
