/********************************************************************************
** Form generated from reading UI file 'OutputOptionsWidget.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OUTPUTOPTIONSWIDGET_H
#define UI_OUTPUTOPTIONSWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QSlider>
#include <QtGui/QSpacerItem>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OutputOptionsWidget
{
public:
    QVBoxLayout *verticalLayout_6;
    QGroupBox *dpiPanel;
    QVBoxLayout *verticalLayout_2;
    QHBoxLayout *horizontalLayout_3;
    QSpacerItem *horizontalSpacer_5;
    QLabel *dpiLabel;
    QSpacerItem *horizontalSpacer_6;
    QHBoxLayout *horizontalLayout_4;
    QSpacerItem *horizontalSpacer_7;
    QPushButton *changeDpiButton;
    QSpacerItem *horizontalSpacer_8;
    QGroupBox *modePanel;
    QVBoxLayout *verticalLayout_4;
    QHBoxLayout *horizontalLayout_2;
    QSpacerItem *horizontalSpacer;
    QComboBox *colorModeSelector;
    QSpacerItem *horizontalSpacer_2;
    QWidget *colorGrayscaleOptions;
    QHBoxLayout *horizontalLayout_6;
    QSpacerItem *horizontalSpacer_13;
    QVBoxLayout *verticalLayout_3;
    QCheckBox *whiteMarginsCB;
    QCheckBox *equalizeIlluminationCB;
    QSpacerItem *horizontalSpacer_14;
    QWidget *bwOptions;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *horizontalLayout_12;
    QSpacerItem *horizontalSpacer_22;
    QLabel *thresholLabel;
    QSpacerItem *horizontalSpacer_21;
    QSlider *thresholdSlider;
    QHBoxLayout *horizontalLayout_7;
    QLabel *lighterThresholdLink;
    QToolButton *neutralThresholdBtn;
    QLabel *darkerThresholdLink;
    QWidget *pictureShapeOptions;
    QVBoxLayout *verticalLayout_10;
    QHBoxLayout *horizontalLayout_14;
    QSpacerItem *horizontalSpacer_25;
    QLabel *labePictureShape;
    QSpacerItem *horizontalSpacer_26;
    QHBoxLayout *horizontalLayout_13;
    QSpacerItem *horizontalSpacer_23;
    QComboBox *pictureShapeSelector;
    QSpacerItem *horizontalSpacer_24;
    QHBoxLayout *horizontalLayout_5;
    QSpacerItem *horizontalSpacer_9;
    QPushButton *applyColorsButton;
    QSpacerItem *horizontalSpacer_10;
    QGroupBox *dewarpingPanel;
    QVBoxLayout *verticalLayout_7;
    QHBoxLayout *horizontalLayout_11;
    QSpacerItem *horizontalSpacer_19;
    QLabel *dewarpingStatusLabel;
    QSpacerItem *horizontalSpacer_20;
    QHBoxLayout *horizontalLayout_8;
    QSpacerItem *horizontalSpacer_17;
    QPushButton *changeDewarpingButton;
    QSpacerItem *horizontalSpacer_18;
    QGroupBox *depthPerceptionPanel;
    QVBoxLayout *verticalLayout_8;
    QSlider *depthPerceptionSlider;
    QHBoxLayout *horizontalLayout_10;
    QSpacerItem *horizontalSpacer_3;
    QPushButton *applyDepthPerceptionButton;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *despecklePanel;
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_11;
    QToolButton *despeckleOffBtn;
    QToolButton *despeckleCautiousBtn;
    QToolButton *despeckleNormalBtn;
    QToolButton *despeckleAggressiveBtn;
    QSpacerItem *horizontalSpacer_12;
    QHBoxLayout *horizontalLayout_9;
    QSpacerItem *horizontalSpacer_15;
    QPushButton *applyDespeckleButton;
    QSpacerItem *horizontalSpacer_16;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *OutputOptionsWidget)
    {
        if (OutputOptionsWidget->objectName().isEmpty())
            OutputOptionsWidget->setObjectName(QString::fromUtf8("OutputOptionsWidget"));
        OutputOptionsWidget->resize(228, 735);
        verticalLayout_6 = new QVBoxLayout(OutputOptionsWidget);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        dpiPanel = new QGroupBox(OutputOptionsWidget);
        dpiPanel->setObjectName(QString::fromUtf8("dpiPanel"));
        verticalLayout_2 = new QVBoxLayout(dpiPanel);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_5);

        dpiLabel = new QLabel(dpiPanel);
        dpiLabel->setObjectName(QString::fromUtf8("dpiLabel"));

        horizontalLayout_3->addWidget(dpiLabel);

        horizontalSpacer_6 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_6);


        verticalLayout_2->addLayout(horizontalLayout_3);

        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_7);

        changeDpiButton = new QPushButton(dpiPanel);
        changeDpiButton->setObjectName(QString::fromUtf8("changeDpiButton"));

        horizontalLayout_4->addWidget(changeDpiButton);

        horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_8);


        verticalLayout_2->addLayout(horizontalLayout_4);


        verticalLayout_6->addWidget(dpiPanel);

        modePanel = new QGroupBox(OutputOptionsWidget);
        modePanel->setObjectName(QString::fromUtf8("modePanel"));
        modePanel->setFlat(false);
        modePanel->setCheckable(false);
        verticalLayout_4 = new QVBoxLayout(modePanel);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalSpacer = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer);

        colorModeSelector = new QComboBox(modePanel);
        colorModeSelector->setObjectName(QString::fromUtf8("colorModeSelector"));

        horizontalLayout_2->addWidget(colorModeSelector);

        horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);


        verticalLayout_4->addLayout(horizontalLayout_2);

        colorGrayscaleOptions = new QWidget(modePanel);
        colorGrayscaleOptions->setObjectName(QString::fromUtf8("colorGrayscaleOptions"));
        horizontalLayout_6 = new QHBoxLayout(colorGrayscaleOptions);
        horizontalLayout_6->setContentsMargins(0, 0, 0, 0);
        horizontalLayout_6->setObjectName(QString::fromUtf8("horizontalLayout_6"));
        horizontalSpacer_13 = new QSpacerItem(13, 17, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_13);

        verticalLayout_3 = new QVBoxLayout();
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        whiteMarginsCB = new QCheckBox(colorGrayscaleOptions);
        whiteMarginsCB->setObjectName(QString::fromUtf8("whiteMarginsCB"));

        verticalLayout_3->addWidget(whiteMarginsCB);

        equalizeIlluminationCB = new QCheckBox(colorGrayscaleOptions);
        equalizeIlluminationCB->setObjectName(QString::fromUtf8("equalizeIlluminationCB"));

        verticalLayout_3->addWidget(equalizeIlluminationCB);


        horizontalLayout_6->addLayout(verticalLayout_3);

        horizontalSpacer_14 = new QSpacerItem(13, 17, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_6->addItem(horizontalSpacer_14);


        verticalLayout_4->addWidget(colorGrayscaleOptions);

        bwOptions = new QWidget(modePanel);
        bwOptions->setObjectName(QString::fromUtf8("bwOptions"));
        verticalLayout = new QVBoxLayout(bwOptions);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        horizontalLayout_12 = new QHBoxLayout();
        horizontalLayout_12->setObjectName(QString::fromUtf8("horizontalLayout_12"));
        horizontalSpacer_22 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_22);

        thresholLabel = new QLabel(bwOptions);
        thresholLabel->setObjectName(QString::fromUtf8("thresholLabel"));

        horizontalLayout_12->addWidget(thresholLabel);

        horizontalSpacer_21 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_12->addItem(horizontalSpacer_21);


        verticalLayout->addLayout(horizontalLayout_12);

        thresholdSlider = new QSlider(bwOptions);
        thresholdSlider->setObjectName(QString::fromUtf8("thresholdSlider"));
        thresholdSlider->setMinimum(-30);
        thresholdSlider->setMaximum(30);
        thresholdSlider->setPageStep(5);
        thresholdSlider->setTracking(true);
        thresholdSlider->setOrientation(Qt::Horizontal);
        thresholdSlider->setInvertedAppearance(false);
        thresholdSlider->setInvertedControls(false);
        thresholdSlider->setTickPosition(QSlider::TicksBelow);

        verticalLayout->addWidget(thresholdSlider);

        horizontalLayout_7 = new QHBoxLayout();
        horizontalLayout_7->setSpacing(0);
        horizontalLayout_7->setObjectName(QString::fromUtf8("horizontalLayout_7"));
        lighterThresholdLink = new QLabel(bwOptions);
        lighterThresholdLink->setObjectName(QString::fromUtf8("lighterThresholdLink"));

        horizontalLayout_7->addWidget(lighterThresholdLink);

        neutralThresholdBtn = new QToolButton(bwOptions);
        neutralThresholdBtn->setObjectName(QString::fromUtf8("neutralThresholdBtn"));
        neutralThresholdBtn->setCursor(QCursor(Qt::PointingHandCursor));
        neutralThresholdBtn->setIconSize(QSize(12, 12));
        neutralThresholdBtn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        neutralThresholdBtn->setAutoRaise(true);
        neutralThresholdBtn->setArrowType(Qt::UpArrow);

        horizontalLayout_7->addWidget(neutralThresholdBtn);

        darkerThresholdLink = new QLabel(bwOptions);
        darkerThresholdLink->setObjectName(QString::fromUtf8("darkerThresholdLink"));
        darkerThresholdLink->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        horizontalLayout_7->addWidget(darkerThresholdLink);


        verticalLayout->addLayout(horizontalLayout_7);


        verticalLayout_4->addWidget(bwOptions);

        pictureShapeOptions = new QWidget(modePanel);
        pictureShapeOptions->setObjectName(QString::fromUtf8("pictureShapeOptions"));
        verticalLayout_10 = new QVBoxLayout(pictureShapeOptions);
        verticalLayout_10->setSpacing(3);
        verticalLayout_10->setObjectName(QString::fromUtf8("verticalLayout_10"));
        verticalLayout_10->setContentsMargins(-1, 3, -1, 3);
        horizontalLayout_14 = new QHBoxLayout();
        horizontalLayout_14->setObjectName(QString::fromUtf8("horizontalLayout_14"));
        horizontalSpacer_25 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_14->addItem(horizontalSpacer_25);

        labePictureShape = new QLabel(pictureShapeOptions);
        labePictureShape->setObjectName(QString::fromUtf8("labePictureShape"));

        horizontalLayout_14->addWidget(labePictureShape);

        horizontalSpacer_26 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_14->addItem(horizontalSpacer_26);


        verticalLayout_10->addLayout(horizontalLayout_14);

        horizontalLayout_13 = new QHBoxLayout();
        horizontalLayout_13->setObjectName(QString::fromUtf8("horizontalLayout_13"));
        horizontalSpacer_23 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_23);

        pictureShapeSelector = new QComboBox(pictureShapeOptions);
        pictureShapeSelector->setObjectName(QString::fromUtf8("pictureShapeSelector"));

        horizontalLayout_13->addWidget(pictureShapeSelector);

        horizontalSpacer_24 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_13->addItem(horizontalSpacer_24);


        verticalLayout_10->addLayout(horizontalLayout_13);


        verticalLayout_4->addWidget(pictureShapeOptions);

        horizontalLayout_5 = new QHBoxLayout();
        horizontalLayout_5->setObjectName(QString::fromUtf8("horizontalLayout_5"));
        horizontalSpacer_9 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_9);

        applyColorsButton = new QPushButton(modePanel);
        applyColorsButton->setObjectName(QString::fromUtf8("applyColorsButton"));

        horizontalLayout_5->addWidget(applyColorsButton);

        horizontalSpacer_10 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_5->addItem(horizontalSpacer_10);


        verticalLayout_4->addLayout(horizontalLayout_5);


        verticalLayout_6->addWidget(modePanel);

        dewarpingPanel = new QGroupBox(OutputOptionsWidget);
        dewarpingPanel->setObjectName(QString::fromUtf8("dewarpingPanel"));
        verticalLayout_7 = new QVBoxLayout(dewarpingPanel);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        horizontalLayout_11 = new QHBoxLayout();
        horizontalLayout_11->setObjectName(QString::fromUtf8("horizontalLayout_11"));
        horizontalSpacer_19 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_19);

        dewarpingStatusLabel = new QLabel(dewarpingPanel);
        dewarpingStatusLabel->setObjectName(QString::fromUtf8("dewarpingStatusLabel"));

        horizontalLayout_11->addWidget(dewarpingStatusLabel);

        horizontalSpacer_20 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_11->addItem(horizontalSpacer_20);


        verticalLayout_7->addLayout(horizontalLayout_11);

        horizontalLayout_8 = new QHBoxLayout();
        horizontalLayout_8->setObjectName(QString::fromUtf8("horizontalLayout_8"));
        horizontalSpacer_17 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_17);

        changeDewarpingButton = new QPushButton(dewarpingPanel);
        changeDewarpingButton->setObjectName(QString::fromUtf8("changeDewarpingButton"));

        horizontalLayout_8->addWidget(changeDewarpingButton);

        horizontalSpacer_18 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_8->addItem(horizontalSpacer_18);


        verticalLayout_7->addLayout(horizontalLayout_8);


        verticalLayout_6->addWidget(dewarpingPanel);

        depthPerceptionPanel = new QGroupBox(OutputOptionsWidget);
        depthPerceptionPanel->setObjectName(QString::fromUtf8("depthPerceptionPanel"));
        verticalLayout_8 = new QVBoxLayout(depthPerceptionPanel);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        depthPerceptionSlider = new QSlider(depthPerceptionPanel);
        depthPerceptionSlider->setObjectName(QString::fromUtf8("depthPerceptionSlider"));
        depthPerceptionSlider->setMinimum(10);
        depthPerceptionSlider->setMaximum(30);
        depthPerceptionSlider->setPageStep(5);
        depthPerceptionSlider->setValue(10);
        depthPerceptionSlider->setOrientation(Qt::Horizontal);

        verticalLayout_8->addWidget(depthPerceptionSlider);

        horizontalLayout_10 = new QHBoxLayout();
        horizontalLayout_10->setObjectName(QString::fromUtf8("horizontalLayout_10"));
        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_3);

        applyDepthPerceptionButton = new QPushButton(depthPerceptionPanel);
        applyDepthPerceptionButton->setObjectName(QString::fromUtf8("applyDepthPerceptionButton"));

        horizontalLayout_10->addWidget(applyDepthPerceptionButton);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_10->addItem(horizontalSpacer_4);


        verticalLayout_8->addLayout(horizontalLayout_10);


        verticalLayout_6->addWidget(depthPerceptionPanel);

        despecklePanel = new QGroupBox(OutputOptionsWidget);
        despecklePanel->setObjectName(QString::fromUtf8("despecklePanel"));
        gridLayout = new QGridLayout(despecklePanel);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setSpacing(6);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_11 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_11);

        despeckleOffBtn = new QToolButton(despecklePanel);
        despeckleOffBtn->setObjectName(QString::fromUtf8("despeckleOffBtn"));
        despeckleOffBtn->setIconSize(QSize(32, 32));
        despeckleOffBtn->setCheckable(true);
        despeckleOffBtn->setChecked(true);
        despeckleOffBtn->setAutoExclusive(true);

        horizontalLayout->addWidget(despeckleOffBtn);

        despeckleCautiousBtn = new QToolButton(despecklePanel);
        despeckleCautiousBtn->setObjectName(QString::fromUtf8("despeckleCautiousBtn"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/despeckle-cautious.png.png"), QSize(), QIcon::Normal, QIcon::Off);
        despeckleCautiousBtn->setIcon(icon);
        despeckleCautiousBtn->setIconSize(QSize(32, 32));
        despeckleCautiousBtn->setCheckable(true);
        despeckleCautiousBtn->setAutoExclusive(true);

        horizontalLayout->addWidget(despeckleCautiousBtn);

        despeckleNormalBtn = new QToolButton(despecklePanel);
        despeckleNormalBtn->setObjectName(QString::fromUtf8("despeckleNormalBtn"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/despeckle-normal.png.png"), QSize(), QIcon::Normal, QIcon::Off);
        despeckleNormalBtn->setIcon(icon1);
        despeckleNormalBtn->setIconSize(QSize(32, 32));
        despeckleNormalBtn->setCheckable(true);
        despeckleNormalBtn->setAutoExclusive(true);

        horizontalLayout->addWidget(despeckleNormalBtn);

        despeckleAggressiveBtn = new QToolButton(despecklePanel);
        despeckleAggressiveBtn->setObjectName(QString::fromUtf8("despeckleAggressiveBtn"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/despeckle-aggressive.png.png"), QSize(), QIcon::Normal, QIcon::Off);
        despeckleAggressiveBtn->setIcon(icon2);
        despeckleAggressiveBtn->setIconSize(QSize(32, 32));
        despeckleAggressiveBtn->setCheckable(true);
        despeckleAggressiveBtn->setAutoExclusive(true);

        horizontalLayout->addWidget(despeckleAggressiveBtn);

        horizontalSpacer_12 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_12);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        horizontalSpacer_15 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_15);

        applyDespeckleButton = new QPushButton(despecklePanel);
        applyDespeckleButton->setObjectName(QString::fromUtf8("applyDespeckleButton"));

        horizontalLayout_9->addWidget(applyDespeckleButton);

        horizontalSpacer_16 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_16);


        gridLayout->addLayout(horizontalLayout_9, 1, 0, 1, 1);


        verticalLayout_6->addWidget(despecklePanel);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer);


        retranslateUi(OutputOptionsWidget);

        QMetaObject::connectSlotsByName(OutputOptionsWidget);
    } // setupUi

    void retranslateUi(QWidget *OutputOptionsWidget)
    {
        OutputOptionsWidget->setWindowTitle(QApplication::translate("OutputOptionsWidget", "Form", 0, QApplication::UnicodeUTF8));
        dpiPanel->setTitle(QApplication::translate("OutputOptionsWidget", "Output Resolution (DPI)", 0, QApplication::UnicodeUTF8));
        dpiLabel->setText(QString());
        changeDpiButton->setText(QApplication::translate("OutputOptionsWidget", "Change ...", 0, QApplication::UnicodeUTF8));
        modePanel->setTitle(QApplication::translate("OutputOptionsWidget", "Mode", 0, QApplication::UnicodeUTF8));
        whiteMarginsCB->setText(QApplication::translate("OutputOptionsWidget", "White margins", 0, QApplication::UnicodeUTF8));
        equalizeIlluminationCB->setText(QApplication::translate("OutputOptionsWidget", "Equalize illumination", 0, QApplication::UnicodeUTF8));
        thresholLabel->setText(QString());
        lighterThresholdLink->setText(QApplication::translate("OutputOptionsWidget", "Thinner", 0, QApplication::UnicodeUTF8));
        neutralThresholdBtn->setText(QString());
        darkerThresholdLink->setText(QApplication::translate("OutputOptionsWidget", "Thicker", 0, QApplication::UnicodeUTF8));
        labePictureShape->setText(QApplication::translate("OutputOptionsWidget", "Picture Shape", 0, QApplication::UnicodeUTF8));
        applyColorsButton->setText(QApplication::translate("OutputOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
        dewarpingPanel->setTitle(QApplication::translate("OutputOptionsWidget", "Dewarping", 0, QApplication::UnicodeUTF8));
        dewarpingStatusLabel->setText(QString());
        changeDewarpingButton->setText(QApplication::translate("OutputOptionsWidget", "Change ...", 0, QApplication::UnicodeUTF8));
        depthPerceptionPanel->setTitle(QApplication::translate("OutputOptionsWidget", "Depth perception", 0, QApplication::UnicodeUTF8));
        applyDepthPerceptionButton->setText(QApplication::translate("OutputOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
        despecklePanel->setTitle(QApplication::translate("OutputOptionsWidget", "Despeckling", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        despeckleOffBtn->setStatusTip(QApplication::translate("OutputOptionsWidget", "No despeckling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        despeckleOffBtn->setText(QString());
#ifndef QT_NO_STATUSTIP
        despeckleCautiousBtn->setStatusTip(QApplication::translate("OutputOptionsWidget", "Cautious despeckling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        despeckleCautiousBtn->setText(QApplication::translate("OutputOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        despeckleNormalBtn->setStatusTip(QApplication::translate("OutputOptionsWidget", "Normal despeckling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        despeckleNormalBtn->setText(QApplication::translate("OutputOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        despeckleAggressiveBtn->setStatusTip(QApplication::translate("OutputOptionsWidget", "Aggressive despeckling", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        despeckleAggressiveBtn->setText(QApplication::translate("OutputOptionsWidget", "...", 0, QApplication::UnicodeUTF8));
        applyDespeckleButton->setText(QApplication::translate("OutputOptionsWidget", "Apply To ...", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OutputOptionsWidget: public Ui_OutputOptionsWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OUTPUTOPTIONSWIDGET_H
