/********************************************************************************
** Form generated from reading UI file 'SystemLoadWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SYSTEMLOADWIDGET_H
#define UI_SYSTEMLOADWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SystemLoadWidget
{
public:
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpacerItem *horizontalSpacer;
    QToolButton *minusBtn;
    QSlider *slider;
    QToolButton *plusBtn;

    void setupUi(QWidget *SystemLoadWidget)
    {
        if (SystemLoadWidget->objectName().isEmpty())
            SystemLoadWidget->setObjectName(QString::fromUtf8("SystemLoadWidget"));
        SystemLoadWidget->resize(232, 39);
        horizontalLayout = new QHBoxLayout(SystemLoadWidget);
        horizontalLayout->setSpacing(0);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        label = new QLabel(SystemLoadWidget);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout->addWidget(label);

        horizontalSpacer = new QSpacerItem(6, 20, QSizePolicy::Fixed, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        minusBtn = new QToolButton(SystemLoadWidget);
        minusBtn->setObjectName(QString::fromUtf8("minusBtn"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/minus-16.png"), QSize(), QIcon::Normal, QIcon::Off);
        minusBtn->setIcon(icon);
        minusBtn->setAutoRaise(true);

        horizontalLayout->addWidget(minusBtn);

        slider = new QSlider(SystemLoadWidget);
        slider->setObjectName(QString::fromUtf8("slider"));
        slider->setTracking(false);
        slider->setOrientation(Qt::Horizontal);
        slider->setTickPosition(QSlider::NoTicks);

        horizontalLayout->addWidget(slider);

        plusBtn = new QToolButton(SystemLoadWidget);
        plusBtn->setObjectName(QString::fromUtf8("plusBtn"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/plus-16.png"), QSize(), QIcon::Normal, QIcon::Off);
        plusBtn->setIcon(icon1);
        plusBtn->setAutoRaise(true);

        horizontalLayout->addWidget(plusBtn);


        retranslateUi(SystemLoadWidget);

        QMetaObject::connectSlotsByName(SystemLoadWidget);
    } // setupUi

    void retranslateUi(QWidget *SystemLoadWidget)
    {
        SystemLoadWidget->setWindowTitle(QApplication::translate("SystemLoadWidget", "Form", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("SystemLoadWidget", "System load", 0, QApplication::UnicodeUTF8));
        minusBtn->setText(QApplication::translate("SystemLoadWidget", "...", 0, QApplication::UnicodeUTF8));
        plusBtn->setText(QApplication::translate("SystemLoadWidget", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SystemLoadWidget: public Ui_SystemLoadWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SYSTEMLOADWIDGET_H
