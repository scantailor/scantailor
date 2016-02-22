/********************************************************************************
** Form generated from reading UI file 'RelinkingDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_RELINKINGDIALOG_H
#define UI_RELINKINGDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include "RelinkablePathVisualization.h"
#include "RelinkingListView.h"

QT_BEGIN_NAMESPACE

class Ui_RelinkingDialog
{
public:
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_3;
    RelinkablePathVisualization *pathVisualization;
    QLabel *errorLabel;
    QToolButton *undoButton;
    RelinkingListView *listView;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *RelinkingDialog)
    {
        if (RelinkingDialog->objectName().isEmpty())
            RelinkingDialog->setObjectName(QString::fromUtf8("RelinkingDialog"));
        RelinkingDialog->resize(400, 300);
        verticalLayout = new QVBoxLayout(RelinkingDialog);
        verticalLayout->setSpacing(6);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalLayout_3->setContentsMargins(-1, -1, 0, -1);
        pathVisualization = new RelinkablePathVisualization(RelinkingDialog);
        pathVisualization->setObjectName(QString::fromUtf8("pathVisualization"));
        QSizePolicy sizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(pathVisualization->sizePolicy().hasHeightForWidth());
        pathVisualization->setSizePolicy(sizePolicy);

        horizontalLayout_3->addWidget(pathVisualization);

        errorLabel = new QLabel(RelinkingDialog);
        errorLabel->setObjectName(QString::fromUtf8("errorLabel"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(errorLabel->sizePolicy().hasHeightForWidth());
        errorLabel->setSizePolicy(sizePolicy1);
        QPalette palette;
        QBrush brush(QColor(255, 0, 0, 255));
        brush.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Active, QPalette::WindowText, brush);
        palette.setBrush(QPalette::Inactive, QPalette::WindowText, brush);
        QBrush brush1(QColor(120, 120, 120, 255));
        brush1.setStyle(Qt::SolidPattern);
        palette.setBrush(QPalette::Disabled, QPalette::WindowText, brush1);
        errorLabel->setPalette(palette);
        errorLabel->setText(QString::fromUtf8("error"));
        errorLabel->setWordWrap(true);

        horizontalLayout_3->addWidget(errorLabel);

        undoButton = new QToolButton(RelinkingDialog);
        undoButton->setObjectName(QString::fromUtf8("undoButton"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(undoButton->sizePolicy().hasHeightForWidth());
        undoButton->setSizePolicy(sizePolicy2);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/undo-22.png"), QSize(), QIcon::Normal, QIcon::Off);
        undoButton->setIcon(icon);

        horizontalLayout_3->addWidget(undoButton);


        verticalLayout->addLayout(horizontalLayout_3);

        listView = new RelinkingListView(RelinkingDialog);
        listView->setObjectName(QString::fromUtf8("listView"));
        listView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        listView->setEditTriggers(QAbstractItemView::NoEditTriggers);

        verticalLayout->addWidget(listView);

        buttonBox = new QDialogButtonBox(RelinkingDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(RelinkingDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), RelinkingDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(RelinkingDialog);
    } // setupUi

    void retranslateUi(QDialog *RelinkingDialog)
    {
        RelinkingDialog->setWindowTitle(QApplication::translate("RelinkingDialog", "Relinking", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        undoButton->setToolTip(QApplication::translate("RelinkingDialog", "Undo", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
        undoButton->setText(QApplication::translate("RelinkingDialog", "...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class RelinkingDialog: public Ui_RelinkingDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_RELINKINGDIALOG_H
