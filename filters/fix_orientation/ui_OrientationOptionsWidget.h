/********************************************************************************
** Form generated from reading UI file 'OrientationOptionsWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ORIENTATIONOPTIONSWIDGET_H
#define UI_ORIENTATIONOPTIONSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OrientationOptionsWidget
{
public:
    QVBoxLayout *vboxLayout;
    QGroupBox *groupBox;
    QVBoxLayout *vboxLayout1;
    QHBoxLayout *hboxLayout;
    QSpacerItem *spacerItem;
    QToolButton *rotateLeftBtn;
    QToolButton *rotateRightBtn;
    QSpacerItem *spacerItem1;
    QHBoxLayout *hboxLayout1;
    QSpacerItem *spacerItem2;
    QLabel *rotationIndicator;
    QSpacerItem *spacerItem3;
    QHBoxLayout *hboxLayout2;
    QSpacerItem *spacerItem4;
    QPushButton *resetBtn;
    QSpacerItem *spacerItem5;
    QGroupBox *scopeBox;
    QVBoxLayout *vboxLayout2;
    QHBoxLayout *hboxLayout3;
    QSpacerItem *spacerItem6;
    QPushButton *applyToBtn;
    QSpacerItem *spacerItem7;
    QSpacerItem *spacerItem8;

    void setupUi(QWidget *OrientationOptionsWidget)
    {
        if (OrientationOptionsWidget->objectName().isEmpty())
            OrientationOptionsWidget->setObjectName(QString::fromUtf8("OrientationOptionsWidget"));
        OrientationOptionsWidget->resize(224, 272);
        vboxLayout = new QVBoxLayout(OrientationOptionsWidget);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        groupBox = new QGroupBox(OrientationOptionsWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        vboxLayout1 = new QVBoxLayout(groupBox);
        vboxLayout1->setObjectName(QString::fromUtf8("vboxLayout1"));
        hboxLayout = new QHBoxLayout();
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem);

        rotateLeftBtn = new QToolButton(groupBox);
        rotateLeftBtn->setObjectName(QString::fromUtf8("rotateLeftBtn"));
        const QIcon icon = QIcon(QString::fromUtf8(":/icons/object-rotate-left.png"));
        rotateLeftBtn->setIcon(icon);
        rotateLeftBtn->setIconSize(QSize(24, 24));

        hboxLayout->addWidget(rotateLeftBtn);

        rotateRightBtn = new QToolButton(groupBox);
        rotateRightBtn->setObjectName(QString::fromUtf8("rotateRightBtn"));
        const QIcon icon1 = QIcon(QString::fromUtf8(":/icons/object-rotate-right.png"));
        rotateRightBtn->setIcon(icon1);
        rotateRightBtn->setIconSize(QSize(24, 24));

        hboxLayout->addWidget(rotateRightBtn);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout->addItem(spacerItem1);


        vboxLayout1->addLayout(hboxLayout);

        hboxLayout1 = new QHBoxLayout();
        hboxLayout1->setObjectName(QString::fromUtf8("hboxLayout1"));
        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem2);

        rotationIndicator = new QLabel(groupBox);
        rotationIndicator->setObjectName(QString::fromUtf8("rotationIndicator"));
        rotationIndicator->setPixmap(QPixmap(QString::fromUtf8(":/icons/big-up-arrow.png")));

        hboxLayout1->addWidget(rotationIndicator);

        spacerItem3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout1->addItem(spacerItem3);


        vboxLayout1->addLayout(hboxLayout1);

        hboxLayout2 = new QHBoxLayout();
        hboxLayout2->setObjectName(QString::fromUtf8("hboxLayout2"));
        spacerItem4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem4);

        resetBtn = new QPushButton(groupBox);
        resetBtn->setObjectName(QString::fromUtf8("resetBtn"));

        hboxLayout2->addWidget(resetBtn);

        spacerItem5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout2->addItem(spacerItem5);


        vboxLayout1->addLayout(hboxLayout2);


        vboxLayout->addWidget(groupBox);

        scopeBox = new QGroupBox(OrientationOptionsWidget);
        scopeBox->setObjectName(QString::fromUtf8("scopeBox"));
        vboxLayout2 = new QVBoxLayout(scopeBox);
        vboxLayout2->setObjectName(QString::fromUtf8("vboxLayout2"));
        hboxLayout3 = new QHBoxLayout();
        hboxLayout3->setObjectName(QString::fromUtf8("hboxLayout3"));
        spacerItem6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem6);

        applyToBtn = new QPushButton(scopeBox);
        applyToBtn->setObjectName(QString::fromUtf8("applyToBtn"));

        hboxLayout3->addWidget(applyToBtn);

        spacerItem7 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        hboxLayout3->addItem(spacerItem7);


        vboxLayout2->addLayout(hboxLayout3);


        vboxLayout->addWidget(scopeBox);

        spacerItem8 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        vboxLayout->addItem(spacerItem8);


        retranslateUi(OrientationOptionsWidget);

        QMetaObject::connectSlotsByName(OrientationOptionsWidget);
    } // setupUi

    void retranslateUi(QWidget *OrientationOptionsWidget)
    {
        OrientationOptionsWidget->setWindowTitle(QApplication::translate("OrientationOptionsWidget", "Form", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("OrientationOptionsWidget", "Rotate", 0, QApplication::UnicodeUTF8));
        rotateLeftBtn->setText(QApplication::translate("OrientationOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        rotateRightBtn->setText(QApplication::translate("OrientationOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        rotationIndicator->setText(QString());
        resetBtn->setText(QApplication::translate("OrientationOptionsWidget", "Reset", 0, QApplication::UnicodeUTF8));
        scopeBox->setTitle(QApplication::translate("OrientationOptionsWidget", "Scope", 0, QApplication::UnicodeUTF8));
        applyToBtn->setText(QApplication::translate("OrientationOptionsWidget", "Apply to ...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OrientationOptionsWidget: public Ui_OrientationOptionsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ORIENTATIONOPTIONSWIDGET_H
