/********************************************************************************
** Form generated from reading UI file 'PageLayoutOptionsWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PAGELAYOUTOPTIONSWIDGET_H
#define UI_PAGELAYOUTOPTIONSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
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

class Ui_PageLayoutOptionsWidget
{
public:
    QVBoxLayout *verticalLayout_3;
    QGroupBox *marginsGroup;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_4;
    QComboBox *unitsComboBox;
    QSpacerItem *horizontalSpacer_2;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer_3;
    QGridLayout *gridLayout;
    QLabel *label;
    QToolButton *topBottomLink;
    QLabel *label_2;
    QDoubleSpinBox *bottomMarginSpinBox;
    QLabel *label_3;
    QDoubleSpinBox *leftMarginSpinBox;
    QToolButton *leftRightLink;
    QLabel *label_4;
    QDoubleSpinBox *rightMarginSpinBox;
    QDoubleSpinBox *topMarginSpinBox;
    QSpacerItem *horizontalSpacer;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_9;
    QPushButton *applyMarginsBtn;
    QSpacerItem *horizontalSpacer_10;
    QGroupBox *alignmentGroup;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_7;
    QCheckBox *alignWithOthersCB;
    QSpacerItem *horizontalSpacer_8;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_6;
    QGridLayout *gridLayout_2;
    QToolButton *alignTopLeftBtn;
    QToolButton *alignTopBtn;
    QToolButton *alignTopRightBtn;
    QToolButton *alignLeftBtn;
    QToolButton *alignCenterBtn;
    QToolButton *alignRightBtn;
    QToolButton *alignBottomLeftBtn;
    QToolButton *alignBottomBtn;
    QToolButton *alignBottomRightBtn;
    QSpacerItem *horizontalSpacer_5;
    QHBoxLayout *horizontalLayout_6;
    QSpacerItem *horizontalSpacer_11;
    QPushButton *applyAlignmentBtn;
    QSpacerItem *horizontalSpacer_12;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *PageLayoutOptionsWidget)
    {
        if (PageLayoutOptionsWidget->objectName().isEmpty())
            PageLayoutOptionsWidget->setObjectName(QString::fromUtf8("PageLayoutOptionsWidget"));
        PageLayoutOptionsWidget->resize(262, 499);
        verticalLayout_3 = new QVBoxLayout(PageLayoutOptionsWidget);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        marginsGroup = new QGroupBox(PageLayoutOptionsWidget);
        marginsGroup->setObjectName(QString::fromUtf8("marginsGroup"));
        verticalLayout = new QVBoxLayout(marginsGroup);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_4 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_4);

        unitsComboBox = new QComboBox(marginsGroup);
        unitsComboBox->setObjectName(QString::fromUtf8("unitsComboBox"));

        horizontalLayout->addWidget(unitsComboBox);

        horizontalSpacer_2 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_2);


        verticalLayout->addLayout(horizontalLayout);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer_3 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_3);

        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label = new QLabel(marginsGroup);
        label->setObjectName(QString::fromUtf8("label"));

        gridLayout->addWidget(label, 0, 0, 1, 1);

        topBottomLink = new QToolButton(marginsGroup);
        topBottomLink->setObjectName(QString::fromUtf8("topBottomLink"));
        topBottomLink->setMinimumSize(QSize(24, 48));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/stock-vchain-broken-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        icon.addFile(QString::fromUtf8(":/icons/stock-vchain-24.png"), QSize(), QIcon::Normal, QIcon::On);
        topBottomLink->setIcon(icon);
        topBottomLink->setIconSize(QSize(9, 24));
        topBottomLink->setAutoRaise(true);

        gridLayout->addWidget(topBottomLink, 0, 2, 2, 1);

        label_2 = new QLabel(marginsGroup);
        label_2->setObjectName(QString::fromUtf8("label_2"));

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        bottomMarginSpinBox = new QDoubleSpinBox(marginsGroup);
        bottomMarginSpinBox->setObjectName(QString::fromUtf8("bottomMarginSpinBox"));
        bottomMarginSpinBox->setDecimals(1);
        bottomMarginSpinBox->setMaximum(999);

        gridLayout->addWidget(bottomMarginSpinBox, 1, 1, 1, 1);

        label_3 = new QLabel(marginsGroup);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout->addWidget(label_3, 2, 0, 1, 1);

        leftMarginSpinBox = new QDoubleSpinBox(marginsGroup);
        leftMarginSpinBox->setObjectName(QString::fromUtf8("leftMarginSpinBox"));
        leftMarginSpinBox->setDecimals(1);
        leftMarginSpinBox->setMaximum(999);

        gridLayout->addWidget(leftMarginSpinBox, 2, 1, 1, 1);

        leftRightLink = new QToolButton(marginsGroup);
        leftRightLink->setObjectName(QString::fromUtf8("leftRightLink"));
        leftRightLink->setMinimumSize(QSize(24, 48));
        leftRightLink->setIcon(icon);
        leftRightLink->setIconSize(QSize(9, 24));
        leftRightLink->setChecked(false);
        leftRightLink->setAutoRaise(true);

        gridLayout->addWidget(leftRightLink, 2, 2, 2, 1);

        label_4 = new QLabel(marginsGroup);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout->addWidget(label_4, 3, 0, 1, 1);

        rightMarginSpinBox = new QDoubleSpinBox(marginsGroup);
        rightMarginSpinBox->setObjectName(QString::fromUtf8("rightMarginSpinBox"));
        rightMarginSpinBox->setDecimals(1);
        rightMarginSpinBox->setMaximum(999);

        gridLayout->addWidget(rightMarginSpinBox, 3, 1, 1, 1);

        topMarginSpinBox = new QDoubleSpinBox(marginsGroup);
        topMarginSpinBox->setObjectName(QString::fromUtf8("topMarginSpinBox"));
        topMarginSpinBox->setDecimals(1);
        topMarginSpinBox->setMaximum(999);

        gridLayout->addWidget(topMarginSpinBox, 0, 1, 1, 1);


        horizontalLayout_2->addLayout(gridLayout);

        horizontalSpacer = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);


        verticalLayout->addLayout(horizontalLayout_2);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_9 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_9);

        applyMarginsBtn = new QPushButton(marginsGroup);
        applyMarginsBtn->setObjectName(QString::fromUtf8("applyMarginsBtn"));

        horizontalLayout_5->addWidget(applyMarginsBtn);

        horizontalSpacer_10 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_10);


        verticalLayout->addLayout(horizontalLayout_5);


        verticalLayout_3->addWidget(marginsGroup);

        alignmentGroup = new QGroupBox(PageLayoutOptionsWidget);
        alignmentGroup->setObjectName(QString::fromUtf8("alignmentGroup"));
        verticalLayout_2 = new QVBoxLayout(alignmentGroup);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setSpacing(0);
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_7 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_7);

        alignWithOthersCB = new QCheckBox(alignmentGroup);
        alignWithOthersCB->setObjectName(QString::fromUtf8("alignWithOthersCB"));
        alignWithOthersCB->setChecked(true);

        horizontalLayout_3->addWidget(alignWithOthersCB);

        horizontalSpacer_8 = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_8);


        verticalLayout_2->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer_6 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_6);

        gridLayout_2 = new QGridLayout();
        gridLayout_2->setSpacing(16);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        alignTopLeftBtn = new QToolButton(alignmentGroup);
        alignTopLeftBtn->setObjectName(QString::fromUtf8("alignTopLeftBtn"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/stock-gravity-north-west-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignTopLeftBtn->setIcon(icon1);
        alignTopLeftBtn->setIconSize(QSize(24, 24));
        alignTopLeftBtn->setCheckable(true);
        alignTopLeftBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignTopLeftBtn, 0, 0, 1, 1);

        alignTopBtn = new QToolButton(alignmentGroup);
        alignTopBtn->setObjectName(QString::fromUtf8("alignTopBtn"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/stock-gravity-north-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignTopBtn->setIcon(icon2);
        alignTopBtn->setIconSize(QSize(24, 24));
        alignTopBtn->setCheckable(true);
        alignTopBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignTopBtn, 0, 1, 1, 1);

        alignTopRightBtn = new QToolButton(alignmentGroup);
        alignTopRightBtn->setObjectName(QString::fromUtf8("alignTopRightBtn"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/stock-gravity-north-east-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignTopRightBtn->setIcon(icon3);
        alignTopRightBtn->setIconSize(QSize(24, 24));
        alignTopRightBtn->setCheckable(true);
        alignTopRightBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignTopRightBtn, 0, 2, 1, 1);

        alignLeftBtn = new QToolButton(alignmentGroup);
        alignLeftBtn->setObjectName(QString::fromUtf8("alignLeftBtn"));
        QIcon icon4;
        icon4.addFile(QString::fromUtf8(":/icons/stock-gravity-west-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignLeftBtn->setIcon(icon4);
        alignLeftBtn->setIconSize(QSize(24, 24));
        alignLeftBtn->setCheckable(true);
        alignLeftBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignLeftBtn, 1, 0, 1, 1);

        alignCenterBtn = new QToolButton(alignmentGroup);
        alignCenterBtn->setObjectName(QString::fromUtf8("alignCenterBtn"));
        QIcon icon5;
        icon5.addFile(QString::fromUtf8(":/icons/stock-center-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignCenterBtn->setIcon(icon5);
        alignCenterBtn->setIconSize(QSize(24, 24));
        alignCenterBtn->setCheckable(true);
        alignCenterBtn->setChecked(true);
        alignCenterBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignCenterBtn, 1, 1, 1, 1);

        alignRightBtn = new QToolButton(alignmentGroup);
        alignRightBtn->setObjectName(QString::fromUtf8("alignRightBtn"));
        QIcon icon6;
        icon6.addFile(QString::fromUtf8(":/icons/stock-gravity-east-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignRightBtn->setIcon(icon6);
        alignRightBtn->setIconSize(QSize(24, 24));
        alignRightBtn->setCheckable(true);
        alignRightBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignRightBtn, 1, 2, 1, 1);

        alignBottomLeftBtn = new QToolButton(alignmentGroup);
        alignBottomLeftBtn->setObjectName(QString::fromUtf8("alignBottomLeftBtn"));
        QIcon icon7;
        icon7.addFile(QString::fromUtf8(":/icons/stock-gravity-south-west-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignBottomLeftBtn->setIcon(icon7);
        alignBottomLeftBtn->setIconSize(QSize(24, 24));
        alignBottomLeftBtn->setCheckable(true);
        alignBottomLeftBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignBottomLeftBtn, 2, 0, 1, 1);

        alignBottomBtn = new QToolButton(alignmentGroup);
        alignBottomBtn->setObjectName(QString::fromUtf8("alignBottomBtn"));
        QIcon icon8;
        icon8.addFile(QString::fromUtf8(":/icons/stock-gravity-south-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignBottomBtn->setIcon(icon8);
        alignBottomBtn->setIconSize(QSize(24, 24));
        alignBottomBtn->setCheckable(true);
        alignBottomBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignBottomBtn, 2, 1, 1, 1);

        alignBottomRightBtn = new QToolButton(alignmentGroup);
        alignBottomRightBtn->setObjectName(QString::fromUtf8("alignBottomRightBtn"));
        QIcon icon9;
        icon9.addFile(QString::fromUtf8(":/icons/stock-gravity-south-east-24.png"), QSize(), QIcon::Normal, QIcon::Off);
        alignBottomRightBtn->setIcon(icon9);
        alignBottomRightBtn->setIconSize(QSize(24, 24));
        alignBottomRightBtn->setCheckable(true);
        alignBottomRightBtn->setAutoExclusive(true);

        gridLayout_2->addWidget(alignBottomRightBtn, 2, 2, 1, 1);


        horizontalLayout_4->addLayout(gridLayout_2);

        horizontalSpacer_5 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_5);


        verticalLayout_2->addLayout(horizontalLayout_4);

        horizontalLayout_6 = new QHBoxLayout();
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalSpacer_11 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_11);

        applyAlignmentBtn = new QPushButton(alignmentGroup);
        applyAlignmentBtn->setObjectName(QString::fromUtf8("applyAlignmentBtn"));

        horizontalLayout_6->addWidget(applyAlignmentBtn);

        horizontalSpacer_12 = new QSpacerItem(1, 1, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_12);


        verticalLayout_2->addLayout(horizontalLayout_6);


        verticalLayout_3->addWidget(alignmentGroup);

        verticalSpacer = new QSpacerItem(1, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_3->addItem(verticalSpacer);

        QWidget::setTabOrder(unitsComboBox, topBottomLink);
        QWidget::setTabOrder(topBottomLink, topMarginSpinBox);
        QWidget::setTabOrder(topMarginSpinBox, bottomMarginSpinBox);
        QWidget::setTabOrder(bottomMarginSpinBox, leftRightLink);
        QWidget::setTabOrder(leftRightLink, leftMarginSpinBox);
        QWidget::setTabOrder(leftMarginSpinBox, rightMarginSpinBox);
        QWidget::setTabOrder(rightMarginSpinBox, applyMarginsBtn);
        QWidget::setTabOrder(applyMarginsBtn, alignWithOthersCB);
        QWidget::setTabOrder(alignWithOthersCB, alignTopLeftBtn);
        QWidget::setTabOrder(alignTopLeftBtn, alignTopBtn);
        QWidget::setTabOrder(alignTopBtn, alignTopRightBtn);
        QWidget::setTabOrder(alignTopRightBtn, alignLeftBtn);
        QWidget::setTabOrder(alignLeftBtn, alignCenterBtn);
        QWidget::setTabOrder(alignCenterBtn, alignRightBtn);
        QWidget::setTabOrder(alignRightBtn, alignBottomLeftBtn);
        QWidget::setTabOrder(alignBottomLeftBtn, alignBottomBtn);
        QWidget::setTabOrder(alignBottomBtn, alignBottomRightBtn);
        QWidget::setTabOrder(alignBottomRightBtn, applyAlignmentBtn);

        retranslateUi(PageLayoutOptionsWidget);

        QMetaObject::connectSlotsByName(PageLayoutOptionsWidget);
    } // setupUi

    void retranslateUi(QWidget *PageLayoutOptionsWidget)
    {
        PageLayoutOptionsWidget->setWindowTitle(QApplication::translate("PageLayoutOptionsWidget", "Form", 0, QApplication::UnicodeUTF8));
        marginsGroup->setTitle(QApplication::translate("PageLayoutOptionsWidget", "Margins", 0, QApplication::UnicodeUTF8));
        unitsComboBox->clear();
        unitsComboBox->insertItems(0, QStringList()
         << QApplication::translate("PageLayoutOptionsWidget", "Millimeters (mm)", 0, QApplication::UnicodeUTF8)
         << QApplication::translate("PageLayoutOptionsWidget", "Inches (in)", 0, QApplication::UnicodeUTF8)
        );
        label->setText(QApplication::translate("PageLayoutOptionsWidget", "Top", 0, QApplication::UnicodeUTF8));
        topBottomLink->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("PageLayoutOptionsWidget", "Bottom", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("PageLayoutOptionsWidget", "Left", 0, QApplication::UnicodeUTF8));
        leftRightLink->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("PageLayoutOptionsWidget", "Right", 0, QApplication::UnicodeUTF8));
        applyMarginsBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
        alignmentGroup->setTitle(QApplication::translate("PageLayoutOptionsWidget", "Alignment", 0, QApplication::UnicodeUTF8));
        alignWithOthersCB->setText(QApplication::translate("PageLayoutOptionsWidget", "Match size with other pages", 0, QApplication::UnicodeUTF8));
        alignTopLeftBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignTopBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignTopRightBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignLeftBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignCenterBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignRightBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignBottomLeftBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignBottomBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        alignBottomRightBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        applyAlignmentBtn->setText(QApplication::translate("PageLayoutOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PageLayoutOptionsWidget: public Ui_PageLayoutOptionsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PAGELAYOUTOPTIONSWIDGET_H
