/********************************************************************************
** Form generated from reading UI file 'OutOfMemoryDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OUTOFMEMORYDIALOG_H
#define UI_OUTOFMEMORYDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QFrame>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QScrollArea>
#include <QtGui/QSpacerItem>
#include <QtGui/QStackedWidget>
#include <QtGui/QTabWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_OutOfMemoryDialog
{
public:
    QVBoxLayout *verticalLayout;
    QStackedWidget *topLevelStack;
    QWidget *mainPage;
    QVBoxLayout *verticalLayout_2;
    QLabel *label;
    QSpacerItem *verticalSpacer_4;
    QTabWidget *tabWidget;
    QWidget *tab;
    QVBoxLayout *verticalLayout_3;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_4;
    QFrame *line;
    QLabel *label_6;
    QFrame *line_2;
    QLabel *label_8;
    QSpacerItem *verticalSpacer_2;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_4;
    QScrollArea *scrollArea_2;
    QWidget *scrollAreaWidgetContents_2;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_10;
    QFrame *line_3;
    QLabel *only_32bit_1;
    QFrame *only_32bit_2;
    QLabel *label_12;
    QFrame *line_5;
    QLabel *label_14;
    QSpacerItem *verticalSpacer;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_8;
    QScrollArea *scrollArea_3;
    QWidget *scrollAreaWidgetContents_3;
    QVBoxLayout *verticalLayout_7;
    QLabel *label_5;
    QSpacerItem *verticalSpacer_3;
    QHBoxLayout *horizontalLayout_9;
    QPushButton *saveProjectBtn;
    QSpacerItem *horizontalSpacer;
    QPushButton *saveProjectAsBtn;
    QSpacerItem *horizontalSpacer_2;
    QPushButton *dontSaveBtn;
    QWidget *saveSuccessPage;
    QVBoxLayout *verticalLayout_9;
    QLabel *label_2;
    QSpacerItem *verticalSpacer_5;
    QLabel *label_3;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *OutOfMemoryDialog)
    {
        if (OutOfMemoryDialog->objectName().isEmpty())
            OutOfMemoryDialog->setObjectName(QString::fromUtf8("OutOfMemoryDialog"));
        OutOfMemoryDialog->resize(470, 342);
        verticalLayout = new QVBoxLayout(OutOfMemoryDialog);
        verticalLayout->setSpacing(0);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        topLevelStack = new QStackedWidget(OutOfMemoryDialog);
        topLevelStack->setObjectName(QString::fromUtf8("topLevelStack"));
        mainPage = new QWidget();
        mainPage->setObjectName(QString::fromUtf8("mainPage"));
        verticalLayout_2 = new QVBoxLayout(mainPage);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label = new QLabel(mainPage);
        label->setObjectName(QString::fromUtf8("label"));
        label->setStyleSheet(QString::fromUtf8("font-size: 17px;\n"
"font-weight: bold;"));
        label->setAlignment(Qt::AlignCenter);

        verticalLayout_2->addWidget(label);

        verticalSpacer_4 = new QSpacerItem(434, 7, QSizePolicy::Minimum, QSizePolicy::Minimum);

        verticalLayout_2->addItem(verticalSpacer_4);

        tabWidget = new QTabWidget(mainPage);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setDocumentMode(false);
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        verticalLayout_3 = new QVBoxLayout(tab);
        verticalLayout_3->setSpacing(0);
        verticalLayout_3->setContentsMargins(0, 0, 0, 0);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        scrollArea = new QScrollArea(tab);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setFrameShadow(QFrame::Plain);
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 446, 227));
        verticalLayout_5 = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        label_4 = new QLabel(scrollAreaWidgetContents);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setWordWrap(true);
        label_4->setOpenExternalLinks(true);

        verticalLayout_5->addWidget(label_4);

        line = new QFrame(scrollAreaWidgetContents);
        line->setObjectName(QString::fromUtf8("line"));
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);

        verticalLayout_5->addWidget(line);

        label_6 = new QLabel(scrollAreaWidgetContents);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setWordWrap(true);
        label_6->setOpenExternalLinks(true);

        verticalLayout_5->addWidget(label_6);

        line_2 = new QFrame(scrollAreaWidgetContents);
        line_2->setObjectName(QString::fromUtf8("line_2"));
        line_2->setFrameShape(QFrame::HLine);
        line_2->setFrameShadow(QFrame::Sunken);

        verticalLayout_5->addWidget(line_2);

        label_8 = new QLabel(scrollAreaWidgetContents);
        label_8->setObjectName(QString::fromUtf8("label_8"));
        label_8->setWordWrap(true);
        label_8->setOpenExternalLinks(true);

        verticalLayout_5->addWidget(label_8);

        verticalSpacer_2 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_5->addItem(verticalSpacer_2);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_3->addWidget(scrollArea);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        verticalLayout_4 = new QVBoxLayout(tab_2);
        verticalLayout_4->setSpacing(0);
        verticalLayout_4->setContentsMargins(0, 0, 0, 0);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        scrollArea_2 = new QScrollArea(tab_2);
        scrollArea_2->setObjectName(QString::fromUtf8("scrollArea_2"));
        scrollArea_2->setFrameShape(QFrame::NoFrame);
        scrollArea_2->setWidgetResizable(true);
        scrollAreaWidgetContents_2 = new QWidget();
        scrollAreaWidgetContents_2->setObjectName(QString::fromUtf8("scrollAreaWidgetContents_2"));
        scrollAreaWidgetContents_2->setGeometry(QRect(0, 0, 446, 227));
        verticalLayout_6 = new QVBoxLayout(scrollAreaWidgetContents_2);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        label_10 = new QLabel(scrollAreaWidgetContents_2);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setWordWrap(true);
        label_10->setOpenExternalLinks(true);

        verticalLayout_6->addWidget(label_10);

        line_3 = new QFrame(scrollAreaWidgetContents_2);
        line_3->setObjectName(QString::fromUtf8("line_3"));
        line_3->setFrameShape(QFrame::HLine);
        line_3->setFrameShadow(QFrame::Sunken);

        verticalLayout_6->addWidget(line_3);

        only_32bit_1 = new QLabel(scrollAreaWidgetContents_2);
        only_32bit_1->setObjectName(QString::fromUtf8("only_32bit_1"));
        only_32bit_1->setWordWrap(true);
        only_32bit_1->setOpenExternalLinks(true);

        verticalLayout_6->addWidget(only_32bit_1);

        only_32bit_2 = new QFrame(scrollAreaWidgetContents_2);
        only_32bit_2->setObjectName(QString::fromUtf8("only_32bit_2"));
        only_32bit_2->setFrameShape(QFrame::HLine);
        only_32bit_2->setFrameShadow(QFrame::Sunken);

        verticalLayout_6->addWidget(only_32bit_2);

        label_12 = new QLabel(scrollAreaWidgetContents_2);
        label_12->setObjectName(QString::fromUtf8("label_12"));
        label_12->setWordWrap(true);
        label_12->setOpenExternalLinks(true);

        verticalLayout_6->addWidget(label_12);

        line_5 = new QFrame(scrollAreaWidgetContents_2);
        line_5->setObjectName(QString::fromUtf8("line_5"));
        line_5->setFrameShape(QFrame::HLine);
        line_5->setFrameShadow(QFrame::Sunken);

        verticalLayout_6->addWidget(line_5);

        label_14 = new QLabel(scrollAreaWidgetContents_2);
        label_14->setObjectName(QString::fromUtf8("label_14"));
        label_14->setWordWrap(true);
        label_14->setOpenExternalLinks(true);

        verticalLayout_6->addWidget(label_14);

        verticalSpacer = new QSpacerItem(20, 9, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_6->addItem(verticalSpacer);

        scrollArea_2->setWidget(scrollAreaWidgetContents_2);

        verticalLayout_4->addWidget(scrollArea_2);

        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        verticalLayout_8 = new QVBoxLayout(tab_3);
        verticalLayout_8->setContentsMargins(0, 0, 0, 0);
        verticalLayout_8->setObjectName(QString::fromUtf8("verticalLayout_8"));
        scrollArea_3 = new QScrollArea(tab_3);
        scrollArea_3->setObjectName(QString::fromUtf8("scrollArea_3"));
        scrollArea_3->setFrameShape(QFrame::NoFrame);
        scrollArea_3->setFrameShadow(QFrame::Plain);
        scrollArea_3->setWidgetResizable(true);
        scrollAreaWidgetContents_3 = new QWidget();
        scrollAreaWidgetContents_3->setObjectName(QString::fromUtf8("scrollAreaWidgetContents_3"));
        scrollAreaWidgetContents_3->setGeometry(QRect(0, 0, 446, 227));
        verticalLayout_7 = new QVBoxLayout(scrollAreaWidgetContents_3);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        label_5 = new QLabel(scrollAreaWidgetContents_3);
        label_5->setObjectName(QString::fromUtf8("label_5"));
        label_5->setWordWrap(true);
        label_5->setOpenExternalLinks(true);

        verticalLayout_7->addWidget(label_5);

        verticalSpacer_3 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_7->addItem(verticalSpacer_3);

        scrollArea_3->setWidget(scrollAreaWidgetContents_3);

        verticalLayout_8->addWidget(scrollArea_3);

        tabWidget->addTab(tab_3, QString());

        verticalLayout_2->addWidget(tabWidget);

        horizontalLayout_9 = new QHBoxLayout();
        horizontalLayout_9->setObjectName(QString::fromUtf8("horizontalLayout_9"));
        saveProjectBtn = new QPushButton(mainPage);
        saveProjectBtn->setObjectName(QString::fromUtf8("saveProjectBtn"));
        saveProjectBtn->setDefault(true);

        horizontalLayout_9->addWidget(saveProjectBtn);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer);

        saveProjectAsBtn = new QPushButton(mainPage);
        saveProjectAsBtn->setObjectName(QString::fromUtf8("saveProjectAsBtn"));

        horizontalLayout_9->addWidget(saveProjectAsBtn);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_9->addItem(horizontalSpacer_2);

        dontSaveBtn = new QPushButton(mainPage);
        dontSaveBtn->setObjectName(QString::fromUtf8("dontSaveBtn"));

        horizontalLayout_9->addWidget(dontSaveBtn);


        verticalLayout_2->addLayout(horizontalLayout_9);

        topLevelStack->addWidget(mainPage);
        saveSuccessPage = new QWidget();
        saveSuccessPage->setObjectName(QString::fromUtf8("saveSuccessPage"));
        verticalLayout_9 = new QVBoxLayout(saveSuccessPage);
        verticalLayout_9->setObjectName(QString::fromUtf8("verticalLayout_9"));
        label_2 = new QLabel(saveSuccessPage);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setStyleSheet(QString::fromUtf8("font-size: 17px;\n"
"font-weight: bold;"));
        label_2->setAlignment(Qt::AlignCenter);

        verticalLayout_9->addWidget(label_2);

        verticalSpacer_5 = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Preferred);

        verticalLayout_9->addItem(verticalSpacer_5);

        label_3 = new QLabel(saveSuccessPage);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy);
        label_3->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignTop);
        label_3->setWordWrap(true);

        verticalLayout_9->addWidget(label_3);

        buttonBox = new QDialogButtonBox(saveSuccessPage);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        verticalLayout_9->addWidget(buttonBox);

        topLevelStack->addWidget(saveSuccessPage);

        verticalLayout->addWidget(topLevelStack);


        retranslateUi(OutOfMemoryDialog);
        QObject::connect(buttonBox, SIGNAL(rejected()), OutOfMemoryDialog, SLOT(reject()));

        topLevelStack->setCurrentIndex(0);
        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(OutOfMemoryDialog);
    } // setupUi

    void retranslateUi(QDialog *OutOfMemoryDialog)
    {
        OutOfMemoryDialog->setWindowTitle(QApplication::translate("OutOfMemoryDialog", "Out of memory", 0, QApplication::UnicodeUTF8));
        label->setText(QApplication::translate("OutOfMemoryDialog", "Out of Memory Situation in Scan Tailor", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("OutOfMemoryDialog", "Did you have to fix the DPI of your source images? Are you sure the values you entered were correct?", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("OutOfMemoryDialog", "Sometimes your source images may have wrong DPI embedded into them. Scan Tailor tries to detect those, but it's not always easy to tell. You may need to check \"Fix DPI even if they look normal\" when creating a project and look into \"All pages\" tab in the \"Fix DPI\" dialog, which is also accessible from the Tools menu.", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("OutOfMemoryDialog", "Is your output DPI set too high? Usually you don't need it higher than 600.", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("OutOfMemoryDialog", "Possible reasons", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("OutOfMemoryDialog", "Fix your DPIs. Learn how to <a href=\"http://vimeo.com/12524529\">estimate unknown DPIs</a>.", 0, QApplication::UnicodeUTF8));
        only_32bit_1->setText(QApplication::translate("OutOfMemoryDialog", "If your hardware and operating system are 64-bit capable, consider switching to a 64-bit version of Scan Tailor.", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("OutOfMemoryDialog", "When working with grayscale images, make sure they are really grayscale. If they are actually color images that just happen to look grayscale, convert them to grayscale using some kind of batch image converter. This will both save memory and increase performance.", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("OutOfMemoryDialog", "As a last resort, you can save some memory by making sure thumbnails are pre-created rather than created on demand. This can be done by slowly scrolling the thumbnail list all the way from top to bottom before starting any real work.", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("OutOfMemoryDialog", "What can help", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("OutOfMemoryDialog", "Surprisingly, upgrading your RAM won't help here. The lack of RAM is compensated by the swap mechanism, which makes things slow, but keeps programs running. An out of memory situation means we ran out of memory address space, which has nothing to do with the amount of RAM you have. The only way to increase the memory address space is to go 64-bit hardware, 64-bit operating system and 64-bit Scan Tailor.", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QApplication::translate("OutOfMemoryDialog", "What won't help", 0, QApplication::UnicodeUTF8));
        saveProjectBtn->setText(QApplication::translate("OutOfMemoryDialog", "Save Project", 0, QApplication::UnicodeUTF8));
        saveProjectAsBtn->setText(QApplication::translate("OutOfMemoryDialog", "Save Project As ...", 0, QApplication::UnicodeUTF8));
        dontSaveBtn->setText(QApplication::translate("OutOfMemoryDialog", "Don't Save", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("OutOfMemoryDialog", "Project Saved Successfully", 0, QApplication::UnicodeUTF8));
        label_3->setText(QApplication::translate("OutOfMemoryDialog", "Please note that while Scan Tailor tries to catch out-of-memory situations and give you the opportunity to save your project, it's not always possible. This time it succeeded, but the next time it might just crash.", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class OutOfMemoryDialog: public Ui_OutOfMemoryDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OUTOFMEMORYDIALOG_H
