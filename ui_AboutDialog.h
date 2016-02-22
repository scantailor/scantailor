/********************************************************************************
** Form generated from reading UI file 'AboutDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_ABOUTDIALOG_H
#define UI_ABOUTDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QGridLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QScrollArea>
#include <QtGui/QSpacerItem>
#include <QtGui/QTabWidget>
#include <QtGui/QTextBrowser>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_AboutDialog
{
public:
    QVBoxLayout *verticalLayout;
    QTabWidget *tabWidget;
    QWidget *tab;
    QGridLayout *gridLayout;
    QLabel *label_4;
    QSpacerItem *verticalSpacer;
    QLabel *label_3;
    QLabel *label;
    QLabel *version;
    QWidget *tab_2;
    QVBoxLayout *verticalLayout_3;
    QScrollArea *scrollArea;
    QWidget *scrollAreaWidgetContents;
    QVBoxLayout *verticalLayout_2;
    QLabel *label_2;
    QLabel *label_5;
    QLabel *label_31;
    QLabel *label_30;
    QLabel *label_6;
    QVBoxLayout *verticalLayout_5;
    QLabel *label_12;
    QLabel *label_13;
    QLabel *label_14;
    QLabel *label_15;
    QLabel *label_7;
    QVBoxLayout *verticalLayout_6;
    QLabel *label_11;
    QLabel *label_9;
    QLabel *label_29;
    QLabel *label_16;
    QLabel *label_17;
    QLabel *label_18;
    QLabel *label_19;
    QLabel *label_20;
    QLabel *label_21;
    QLabel *label_22;
    QLabel *label_23;
    QLabel *label_27;
    QLabel *label_24;
    QLabel *label_28;
    QLabel *label_301;
    QLabel *label_10;
    QVBoxLayout *verticalLayout_7;
    QLabel *label_8;
    QLabel *label_25;
    QLabel *label_26;
    QSpacerItem *verticalSpacer_2;
    QWidget *tab_3;
    QVBoxLayout *verticalLayout_4;
    QTextBrowser *licenseViewer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *AboutDialog)
    {
        if (AboutDialog->objectName().isEmpty())
            AboutDialog->setObjectName(QString::fromUtf8("AboutDialog"));
        AboutDialog->resize(482, 349);
        verticalLayout = new QVBoxLayout(AboutDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        tabWidget = new QTabWidget(AboutDialog);
        tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
        tabWidget->setDocumentMode(true);
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout = new QGridLayout(tab);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        label_4 = new QLabel(tab);
        label_4->setObjectName(QString::fromUtf8("label_4"));
        label_4->setWordWrap(true);

        gridLayout->addWidget(label_4, 3, 0, 1, 2);

        verticalSpacer = new QSpacerItem(440, 127, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout->addItem(verticalSpacer, 4, 0, 1, 2);

        label_3 = new QLabel(tab);
        label_3->setObjectName(QString::fromUtf8("label_3"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(label_3->sizePolicy().hasHeightForWidth());
        label_3->setSizePolicy(sizePolicy);
        label_3->setStyleSheet(QString::fromUtf8("font-size: 16pt;\n"
"font-weight: bold;"));
        label_3->setText(QString::fromUtf8("Scan Tailor Featured"));
        label_3->setAlignment(Qt::AlignBottom|Qt::AlignHCenter);

        gridLayout->addWidget(label_3, 0, 0, 1, 1);

        label = new QLabel(tab);
        label->setObjectName(QString::fromUtf8("label"));
        label->setText(QString::fromUtf8(""));
        label->setPixmap(QPixmap(QString::fromUtf8(":/icons/appicon-about.png")));

        gridLayout->addWidget(label, 0, 1, 2, 1);

        version = new QLabel(tab);
        version->setObjectName(QString::fromUtf8("version"));
        version->setStyleSheet(QString::fromUtf8("font-size:10pt;\n"
"font-weight:bold;"));
        version->setText(QString::fromUtf8("version"));
        version->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        gridLayout->addWidget(version, 1, 0, 1, 1);

        tabWidget->addTab(tab, QString());
        tab_2 = new QWidget();
        tab_2->setObjectName(QString::fromUtf8("tab_2"));
        verticalLayout_3 = new QVBoxLayout(tab_2);
        verticalLayout_3->setObjectName(QString::fromUtf8("verticalLayout_3"));
        scrollArea = new QScrollArea(tab_2);
        scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
        scrollArea->setWidgetResizable(true);
        scrollAreaWidgetContents = new QWidget();
        scrollAreaWidgetContents->setObjectName(QString::fromUtf8("scrollAreaWidgetContents"));
        scrollAreaWidgetContents->setGeometry(QRect(0, 0, 256, 448));
        verticalLayout_2 = new QVBoxLayout(scrollAreaWidgetContents);
        verticalLayout_2->setObjectName(QString::fromUtf8("verticalLayout_2"));
        label_2 = new QLabel(scrollAreaWidgetContents);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        label_2->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        verticalLayout_2->addWidget(label_2);

        label_5 = new QLabel(scrollAreaWidgetContents);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        verticalLayout_2->addWidget(label_5);

        label_31 = new QLabel(scrollAreaWidgetContents);
        label_31->setObjectName(QString::fromUtf8("label_31"));
        label_31->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        verticalLayout_2->addWidget(label_31);

        label_30 = new QLabel(scrollAreaWidgetContents);
        label_30->setObjectName(QString::fromUtf8("label_30"));

        verticalLayout_2->addWidget(label_30);

        label_6 = new QLabel(scrollAreaWidgetContents);
        label_6->setObjectName(QString::fromUtf8("label_6"));
        label_6->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        verticalLayout_2->addWidget(label_6);

        verticalLayout_5 = new QVBoxLayout();
        verticalLayout_5->setSpacing(0);
        verticalLayout_5->setObjectName(QString::fromUtf8("verticalLayout_5"));
        verticalLayout_5->setContentsMargins(-1, 0, -1, -1);
        label_12 = new QLabel(scrollAreaWidgetContents);
        label_12->setObjectName(QString::fromUtf8("label_12"));

        verticalLayout_5->addWidget(label_12);

        label_13 = new QLabel(scrollAreaWidgetContents);
        label_13->setObjectName(QString::fromUtf8("label_13"));

        verticalLayout_5->addWidget(label_13);

        label_14 = new QLabel(scrollAreaWidgetContents);
        label_14->setObjectName(QString::fromUtf8("label_14"));

        verticalLayout_5->addWidget(label_14);

        label_15 = new QLabel(scrollAreaWidgetContents);
        label_15->setObjectName(QString::fromUtf8("label_15"));

        verticalLayout_5->addWidget(label_15);


        verticalLayout_2->addLayout(verticalLayout_5);

        label_7 = new QLabel(scrollAreaWidgetContents);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        label_7->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        verticalLayout_2->addWidget(label_7);

        verticalLayout_6 = new QVBoxLayout();
        verticalLayout_6->setSpacing(0);
        verticalLayout_6->setObjectName(QString::fromUtf8("verticalLayout_6"));
        verticalLayout_6->setContentsMargins(-1, -1, -1, 0);
        label_11 = new QLabel(scrollAreaWidgetContents);
        label_11->setObjectName(QString::fromUtf8("label_11"));

        verticalLayout_6->addWidget(label_11);

        label_9 = new QLabel(scrollAreaWidgetContents);
        label_9->setObjectName(QString::fromUtf8("label_9"));

        verticalLayout_6->addWidget(label_9);

        label_29 = new QLabel(scrollAreaWidgetContents);
        label_29->setObjectName(QString::fromUtf8("label_29"));

        verticalLayout_6->addWidget(label_29);

        label_16 = new QLabel(scrollAreaWidgetContents);
        label_16->setObjectName(QString::fromUtf8("label_16"));

        verticalLayout_6->addWidget(label_16);

        label_17 = new QLabel(scrollAreaWidgetContents);
        label_17->setObjectName(QString::fromUtf8("label_17"));

        verticalLayout_6->addWidget(label_17);

        label_18 = new QLabel(scrollAreaWidgetContents);
        label_18->setObjectName(QString::fromUtf8("label_18"));

        verticalLayout_6->addWidget(label_18);

        label_19 = new QLabel(scrollAreaWidgetContents);
        label_19->setObjectName(QString::fromUtf8("label_19"));

        verticalLayout_6->addWidget(label_19);

        label_20 = new QLabel(scrollAreaWidgetContents);
        label_20->setObjectName(QString::fromUtf8("label_20"));

        verticalLayout_6->addWidget(label_20);

        label_21 = new QLabel(scrollAreaWidgetContents);
        label_21->setObjectName(QString::fromUtf8("label_21"));

        verticalLayout_6->addWidget(label_21);

        label_22 = new QLabel(scrollAreaWidgetContents);
        label_22->setObjectName(QString::fromUtf8("label_22"));

        verticalLayout_6->addWidget(label_22);

        label_23 = new QLabel(scrollAreaWidgetContents);
        label_23->setObjectName(QString::fromUtf8("label_23"));

        verticalLayout_6->addWidget(label_23);

        label_27 = new QLabel(scrollAreaWidgetContents);
        label_27->setObjectName(QString::fromUtf8("label_27"));

        verticalLayout_6->addWidget(label_27);

        label_24 = new QLabel(scrollAreaWidgetContents);
        label_24->setObjectName(QString::fromUtf8("label_24"));

        verticalLayout_6->addWidget(label_24);

        label_28 = new QLabel(scrollAreaWidgetContents);
        label_28->setObjectName(QString::fromUtf8("label_28"));

        verticalLayout_6->addWidget(label_28);

        label_301 = new QLabel(scrollAreaWidgetContents);
        label_301->setObjectName(QString::fromUtf8("label_301"));

        verticalLayout_6->addWidget(label_301);


        verticalLayout_2->addLayout(verticalLayout_6);

        label_10 = new QLabel(scrollAreaWidgetContents);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        label_10->setStyleSheet(QString::fromUtf8("font-weight: bold;"));

        verticalLayout_2->addWidget(label_10);

        verticalLayout_7 = new QVBoxLayout();
        verticalLayout_7->setSpacing(0);
        verticalLayout_7->setObjectName(QString::fromUtf8("verticalLayout_7"));
        verticalLayout_7->setContentsMargins(-1, -1, -1, 0);
        label_8 = new QLabel(scrollAreaWidgetContents);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        verticalLayout_7->addWidget(label_8);

        label_25 = new QLabel(scrollAreaWidgetContents);
        label_25->setObjectName(QString::fromUtf8("label_25"));
        label_25->setText(QString::fromUtf8("phaedrus"));

        verticalLayout_7->addWidget(label_25);

        label_26 = new QLabel(scrollAreaWidgetContents);
        label_26->setObjectName(QString::fromUtf8("label_26"));
        label_26->setText(QString::fromUtf8("Taxman"));

        verticalLayout_7->addWidget(label_26);


        verticalLayout_2->addLayout(verticalLayout_7);

        verticalSpacer_2 = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout_2->addItem(verticalSpacer_2);

        scrollArea->setWidget(scrollAreaWidgetContents);

        verticalLayout_3->addWidget(scrollArea);

        tabWidget->addTab(tab_2, QString());
        tab_3 = new QWidget();
        tab_3->setObjectName(QString::fromUtf8("tab_3"));
        verticalLayout_4 = new QVBoxLayout(tab_3);
        verticalLayout_4->setObjectName(QString::fromUtf8("verticalLayout_4"));
        licenseViewer = new QTextBrowser(tab_3);
        licenseViewer->setObjectName(QString::fromUtf8("licenseViewer"));
        licenseViewer->setHtml(QString::fromUtf8("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
"<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
"p, li { white-space: pre-wrap; }\n"
"</style></head><body style=\" font-family:'MS Shell Dlg 2'; font-size:8.25pt; font-weight:400; font-style:normal;\">\n"
"<p style=\"-qt-paragraph-type:empty; margin-top:0px; margin-bottom:0px; margin-left:0px; margin-right:0px; -qt-block-indent:0; text-indent:0px; font-family:'Ubuntu'; font-size:11pt;\"><br /></p></body></html>"));
        licenseViewer->setOpenExternalLinks(true);

        verticalLayout_4->addWidget(licenseViewer);

        tabWidget->addTab(tab_3, QString());

        verticalLayout->addWidget(tabWidget);

        buttonBox = new QDialogButtonBox(AboutDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setStandardButtons(QDialogButtonBox::Close);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(AboutDialog);
        QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), AboutDialog, SLOT(accept()));

        tabWidget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(AboutDialog);
    } // setupUi

    void retranslateUi(QDialog *AboutDialog)
    {
        AboutDialog->setWindowTitle(QApplication::translate("AboutDialog", "About Scan Tailor", 0, QApplication::UnicodeUTF8));
        label_4->setText(QApplication::translate("AboutDialog", "Scan Tailor is an interactive post-processing tool for scanned pages. It performs operations such as page splitting, skew correction, adding/removing margins, and others. You give it raw scans, and you get pages ready to be printed or assembled into a PDF or DJVU file.  Scanning and optical character recognition is out of scope of this project.", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab), QApplication::translate("AboutDialog", "About", 0, QApplication::UnicodeUTF8));
        label_2->setText(QApplication::translate("AboutDialog", "Lead Developer", 0, QApplication::UnicodeUTF8));
        label_5->setText(QApplication::translate("AboutDialog", "Joseph Artsimovich", 0, QApplication::UnicodeUTF8));
        label_31->setText(QApplication::translate("AboutDialog", "Fork Developer", 0, QApplication::UnicodeUTF8));
        label_30->setText(QApplication::translate("AboutDialog", "monday2000", 0, QApplication::UnicodeUTF8));
        label_6->setText(QApplication::translate("AboutDialog", "Contributors", 0, QApplication::UnicodeUTF8));
        label_12->setText(QApplication::translate("AboutDialog", "U235 - Picture auto-detection algorithm.", 0, QApplication::UnicodeUTF8));
        label_13->setText(QApplication::translate("AboutDialog", "Robert B. - First generation dewarping algorithm.", 0, QApplication::UnicodeUTF8));
        label_14->setText(QApplication::translate("AboutDialog", "Andrey Bergman - System load adjustment.", 0, QApplication::UnicodeUTF8));
        label_15->setText(QApplication::translate("AboutDialog", "Petr Kov\303\241\305\231 - Command line interface.", 0, QApplication::UnicodeUTF8));
        label_7->setText(QApplication::translate("AboutDialog", "Translators", 0, QApplication::UnicodeUTF8));
        label_11->setText(QApplication::translate("AboutDialog", "Neco Torquato - Brazilian Portuguese", 0, QApplication::UnicodeUTF8));
        label_9->setText(QApplication::translate("AboutDialog", "Svetoslav Sashkov, Mandor - Bulgarian", 0, QApplication::UnicodeUTF8));
        label_29->setText(QApplication::translate("AboutDialog", "Damir13 - Croatian", 0, QApplication::UnicodeUTF8));
        label_16->setText(QApplication::translate("AboutDialog", "Petr Kov\303\241\305\231 - Czech", 0, QApplication::UnicodeUTF8));
        label_17->setText(QApplication::translate("AboutDialog", "Stefan Birkner - German", 0, QApplication::UnicodeUTF8));
        label_18->setText(QApplication::translate("AboutDialog", "Angelo Gemmi - Italian", 0, QApplication::UnicodeUTF8));
        label_19->setText(QApplication::translate("AboutDialog", "Masahiro Kitagawa - Japanese", 0, QApplication::UnicodeUTF8));
        label_20->setText(QApplication::translate("AboutDialog", "Patrick Pascal - French", 0, QApplication::UnicodeUTF8));
        label_21->setText(QApplication::translate("AboutDialog", "Daniel Ko\304\207 - Polish", 0, QApplication::UnicodeUTF8));
        label_22->setText(QApplication::translate("AboutDialog", "Joseph Artsimovich - Russian", 0, QApplication::UnicodeUTF8));
        label_23->setText(QApplication::translate("AboutDialog", "Mari\303\241n Hvolka - Slovak", 0, QApplication::UnicodeUTF8));
        label_27->setText(QApplication::translate("AboutDialog", "Flavio Benelli - Spanish", 0, QApplication::UnicodeUTF8));
        label_24->setText(QApplication::translate("AboutDialog", "Davidson Wang - Traditional Chinese", 0, QApplication::UnicodeUTF8));
        label_28->setText(QApplication::translate("AboutDialog", "Yuri Chornoivan - Ukrainian", 0, QApplication::UnicodeUTF8));
        label_301->setText(QApplication::translate("AboutDialog", "Hxcan Cai - Simplified Chinese", 0, QApplication::UnicodeUTF8));
        label_10->setText(QApplication::translate("AboutDialog", "Documentation", 0, QApplication::UnicodeUTF8));
        label_8->setText(QApplication::translate("AboutDialog", "denver 22", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_2), QApplication::translate("AboutDialog", "Authors", 0, QApplication::UnicodeUTF8));
        tabWidget->setTabText(tabWidget->indexOf(tab_3), QApplication::translate("AboutDialog", "License", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class AboutDialog: public Ui_AboutDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_ABOUTDIALOG_H
