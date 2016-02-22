/********************************************************************************
** Form generated from reading UI file 'ErrorWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ERRORWIDGET_H
#define UI_ERRORWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_ErrorWidget
{
public:
    QGridLayout *gridLayout;
    QSpacerItem *spacerItem;
    QSpacerItem *spacerItem1;
    QGroupBox *groupBox;
    QHBoxLayout *hboxLayout;
    QLabel *imageLabel;
    QLabel *textLabel;
    QSpacerItem *spacerItem2;
    QSpacerItem *spacerItem3;

    void setupUi(QWidget *ErrorWidget)
    {
        if (ErrorWidget->objectName().isEmpty())
            ErrorWidget->setObjectName(QString::fromUtf8("ErrorWidget"));
        ErrorWidget->resize(368, 208);
        gridLayout = new QGridLayout(ErrorWidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        spacerItem = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem, 0, 1, 1, 1);

        spacerItem1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem1, 1, 0, 1, 1);

        groupBox = new QGroupBox(ErrorWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        groupBox->setAlignment(Qt::AlignHCenter);
        hboxLayout = new QHBoxLayout(groupBox);
        hboxLayout->setObjectName(QString::fromUtf8("hboxLayout"));
        imageLabel = new QLabel(groupBox);
        imageLabel->setObjectName(QString::fromUtf8("imageLabel"));
        imageLabel->setText(QString::fromUtf8("ImageLabel"));

        hboxLayout->addWidget(imageLabel);

        textLabel = new QLabel(groupBox);
        textLabel->setObjectName(QString::fromUtf8("textLabel"));
        textLabel->setText(QString::fromUtf8("TextLabel"));
        textLabel->setTextFormat(Qt::AutoText);
        textLabel->setWordWrap(true);
        textLabel->setTextInteractionFlags(Qt::LinksAccessibleByMouse);

        hboxLayout->addWidget(textLabel);


        gridLayout->addWidget(groupBox, 1, 1, 1, 1);

        spacerItem2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout->addItem(spacerItem2, 1, 2, 1, 1);

        spacerItem3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(spacerItem3, 2, 1, 1, 1);


        retranslateUi(ErrorWidget);

        QMetaObject::connectSlotsByName(ErrorWidget);
    } // setupUi

    void retranslateUi(QWidget *ErrorWidget)
    {
        ErrorWidget->setWindowTitle(QApplication::translate("ErrorWidget", "Form", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QString());
    } // retranslateUi

};

namespace Ui {
    class ErrorWidget: public Ui_ErrorWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ERRORWIDGET_H
