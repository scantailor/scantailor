/********************************************************************************
** Form generated from reading UI file 'NewOpenProjectPanel.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_NEWOPENPROJECTPANEL_H
#define UI_NEWOPENPROJECTPANEL_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_NewOpenProjectPanel
{
public:
    QVBoxLayout *verticalLayout_2;
    QWidget *widget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_2;
    QLabel *newProjectLabel;
    QSpacerItem *horizontalSpacer;
    QLabel *openProjectLabel;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *recentProjectsGroup;

    void setupUi(QWidget *NewOpenProjectPanel)
    {
        if (NewOpenProjectPanel->objectName().isEmpty())
            NewOpenProjectPanel->setObjectName(QString::fromUtf8("NewOpenProjectPanel"));
        NewOpenProjectPanel->resize(302, 81);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(NewOpenProjectPanel->sizePolicy().hasHeightForWidth());
        NewOpenProjectPanel->setSizePolicy(sizePolicy);
        NewOpenProjectPanel->setAutoFillBackground(false);
        verticalLayout_2 = new QVBoxLayout(NewOpenProjectPanel);
        verticalLayout_2->setContentsMargins(7, 7, 7, 7);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        widget = new QWidget(NewOpenProjectPanel);
        widget->setObjectName(QString::fromUtf8("widget"));
        verticalLayout = new QVBoxLayout(widget);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_2 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);

        newProjectLabel = new QLabel(widget);
        newProjectLabel->setObjectName(QString::fromUtf8("newProjectLabel"));
        newProjectLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout->addWidget(newProjectLabel);

        horizontalSpacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        openProjectLabel = new QLabel(widget);
        openProjectLabel->setObjectName(QString::fromUtf8("openProjectLabel"));

        horizontalLayout->addWidget(openProjectLabel);

        horizontalSpacer_3 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_3);


        verticalLayout->addLayout(horizontalLayout);

        recentProjectsGroup = new QGroupBox(widget);
        recentProjectsGroup->setObjectName(QString::fromUtf8("recentProjectsGroup"));

        verticalLayout->addWidget(recentProjectsGroup);


        verticalLayout_2->addWidget(widget);


        retranslateUi(NewOpenProjectPanel);

        QMetaObject::connectSlotsByName(NewOpenProjectPanel);
    } // setupUi

    void retranslateUi(QWidget *NewOpenProjectPanel)
    {
        NewOpenProjectPanel->setWindowTitle(QApplication::translate("NewOpenProjectPanel", "Form", 0, QApplication::UnicodeUTF8));
        newProjectLabel->setText(QApplication::translate("NewOpenProjectPanel", "New Project ...", 0, QApplication::UnicodeUTF8));
        openProjectLabel->setText(QApplication::translate("NewOpenProjectPanel", "Open Project ...", 0, QApplication::UnicodeUTF8));
        recentProjectsGroup->setTitle(QApplication::translate("NewOpenProjectPanel", "Recent Projects", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class NewOpenProjectPanel: public Ui_NewOpenProjectPanel {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_NEWOPENPROJECTPANEL_H
