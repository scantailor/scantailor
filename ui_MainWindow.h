/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QFrame>
#include <QtGui/QGraphicsView>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMenuBar>
#include <QtGui/QSplitter>
#include <QtGui/QStatusBar>
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>
#include "NonOwningWidget.h"
#include "StageListView.h"

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionDebug;
    QAction *actionSaveProject;
    QAction *actionSaveProjectAs;
    QAction *actionNextPage;
    QAction *actionPrevPage;
    QAction *actionNewProject;
    QAction *actionOpenProject;
    QAction *actionPrevPageQ;
    QAction *actionNextPageW;
    QAction *actionCloseProject;
    QAction *actionQuit;
    QAction *actionSettings;
    QAction *actionFirstPage;
    QAction *actionLastPage;
    QAction *actionAbout;
    QAction *actionFixDpi;
    QAction *actionRelinking;
    QAction *actionExport;
    QWidget *centralwidget;
    QHBoxLayout *horizontalLayout;
    QSplitter *splitter;
    QWidget *layoutWidget;
    QVBoxLayout *vboxLayout;
    StageListView *filterList;
    NonOwningWidget *filterOptions;
    QFrame *imageViewFrame;
    QVBoxLayout *verticalLayout;
    QToolButton *focusButton;
    QGraphicsView *thumbView;
    QComboBox *sortOptions;
    QMenuBar *menubar;
    QMenu *menuDebug;
    QMenu *menuFile;
    QMenu *menuHelp;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName(QString::fromUtf8("MainWindow"));
        MainWindow->resize(613, 445);
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(MainWindow->sizePolicy().hasHeightForWidth());
        MainWindow->setSizePolicy(sizePolicy);
        MainWindow->setStyleSheet(QString::fromUtf8(""));
        actionDebug = new QAction(MainWindow);
        actionDebug->setObjectName(QString::fromUtf8("actionDebug"));
        actionDebug->setCheckable(true);
        actionDebug->setChecked(false);
        actionSaveProject = new QAction(MainWindow);
        actionSaveProject->setObjectName(QString::fromUtf8("actionSaveProject"));
        actionSaveProjectAs = new QAction(MainWindow);
        actionSaveProjectAs->setObjectName(QString::fromUtf8("actionSaveProjectAs"));
        actionNextPage = new QAction(MainWindow);
        actionNextPage->setObjectName(QString::fromUtf8("actionNextPage"));
        actionNextPage->setAutoRepeat(false);
        actionPrevPage = new QAction(MainWindow);
        actionPrevPage->setObjectName(QString::fromUtf8("actionPrevPage"));
        actionPrevPage->setAutoRepeat(false);
        actionNewProject = new QAction(MainWindow);
        actionNewProject->setObjectName(QString::fromUtf8("actionNewProject"));
        actionOpenProject = new QAction(MainWindow);
        actionOpenProject->setObjectName(QString::fromUtf8("actionOpenProject"));
        actionPrevPageQ = new QAction(MainWindow);
        actionPrevPageQ->setObjectName(QString::fromUtf8("actionPrevPageQ"));
        actionPrevPageQ->setAutoRepeat(false);
        actionNextPageW = new QAction(MainWindow);
        actionNextPageW->setObjectName(QString::fromUtf8("actionNextPageW"));
        actionNextPageW->setAutoRepeat(false);
        actionCloseProject = new QAction(MainWindow);
        actionCloseProject->setObjectName(QString::fromUtf8("actionCloseProject"));
        actionQuit = new QAction(MainWindow);
        actionQuit->setObjectName(QString::fromUtf8("actionQuit"));
        actionSettings = new QAction(MainWindow);
        actionSettings->setObjectName(QString::fromUtf8("actionSettings"));
        actionFirstPage = new QAction(MainWindow);
        actionFirstPage->setObjectName(QString::fromUtf8("actionFirstPage"));
        actionLastPage = new QAction(MainWindow);
        actionLastPage->setObjectName(QString::fromUtf8("actionLastPage"));
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName(QString::fromUtf8("actionAbout"));
        actionFixDpi = new QAction(MainWindow);
        actionFixDpi->setObjectName(QString::fromUtf8("actionFixDpi"));
        actionRelinking = new QAction(MainWindow);
        actionRelinking->setObjectName(QString::fromUtf8("actionRelinking"));
        actionExport = new QAction(MainWindow);
        actionExport->setObjectName(QString::fromUtf8("actionExport"));
        centralwidget = new QWidget(MainWindow);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        sizePolicy.setHeightForWidth(centralwidget->sizePolicy().hasHeightForWidth());
        centralwidget->setSizePolicy(sizePolicy);
        centralwidget->setCursor(QCursor(Qt::ArrowCursor));
        horizontalLayout = new QHBoxLayout(centralwidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(-1, -1, -1, 0);
        splitter = new QSplitter(centralwidget);
        splitter->setObjectName(QString::fromUtf8("splitter"));
        QSizePolicy sizePolicy1(QSizePolicy::Expanding, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(splitter->sizePolicy().hasHeightForWidth());
        splitter->setSizePolicy(sizePolicy1);
        splitter->setOrientation(Qt::Horizontal);
        splitter->setChildrenCollapsible(false);
        layoutWidget = new QWidget(splitter);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        vboxLayout = new QVBoxLayout(layoutWidget);
        vboxLayout->setSpacing(6);
        vboxLayout->setObjectName(QString::fromUtf8("vboxLayout"));
        vboxLayout->setContentsMargins(0, 0, 0, 0);
        filterList = new StageListView(layoutWidget);
        filterList->setObjectName(QString::fromUtf8("filterList"));
        filterList->setAutoScroll(false);
        filterList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        filterList->setTabKeyNavigation(false);
        filterList->setProperty("showDropIndicator", QVariant(false));
        filterList->setAlternatingRowColors(true);
        filterList->setSelectionMode(QAbstractItemView::SingleSelection);
        filterList->setSelectionBehavior(QAbstractItemView::SelectRows);
        filterList->setTextElideMode(Qt::ElideNone);
        filterList->setShowGrid(false);
        filterList->setWordWrap(false);
        filterList->setCornerButtonEnabled(false);

        vboxLayout->addWidget(filterList);

        filterOptions = new NonOwningWidget(layoutWidget);
        filterOptions->setObjectName(QString::fromUtf8("filterOptions"));

        vboxLayout->addWidget(filterOptions);

        splitter->addWidget(layoutWidget);
        imageViewFrame = new QFrame(splitter);
        imageViewFrame->setObjectName(QString::fromUtf8("imageViewFrame"));
        QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy2.setHorizontalStretch(1);
        sizePolicy2.setVerticalStretch(1);
        sizePolicy2.setHeightForWidth(imageViewFrame->sizePolicy().hasHeightForWidth());
        imageViewFrame->setSizePolicy(sizePolicy2);
        imageViewFrame->setFrameShape(QFrame::StyledPanel);
        imageViewFrame->setFrameShadow(QFrame::Sunken);
        splitter->addWidget(imageViewFrame);

        horizontalLayout->addWidget(splitter);

        verticalLayout = new QVBoxLayout();
        verticalLayout->setSpacing(0);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        focusButton = new QToolButton(centralwidget);
        focusButton->setObjectName(QString::fromUtf8("focusButton"));
        QSizePolicy sizePolicy3(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy3.setHorizontalStretch(0);
        sizePolicy3.setVerticalStretch(0);
        sizePolicy3.setHeightForWidth(focusButton->sizePolicy().hasHeightForWidth());
        focusButton->setSizePolicy(sizePolicy3);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/keep-in-view.png"), QSize(), QIcon::Normal, QIcon::Off);
        focusButton->setIcon(icon);
        focusButton->setCheckable(true);
        focusButton->setChecked(true);

        verticalLayout->addWidget(focusButton);

        thumbView = new QGraphicsView(centralwidget);
        thumbView->setObjectName(QString::fromUtf8("thumbView"));
        thumbView->setMaximumSize(QSize(249, 16777215));
        thumbView->setAlignment(Qt::AlignHCenter|Qt::AlignTop);

        verticalLayout->addWidget(thumbView);

        sortOptions = new QComboBox(centralwidget);
        sortOptions->setObjectName(QString::fromUtf8("sortOptions"));
        sortOptions->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLength);

        verticalLayout->addWidget(sortOptions);


        horizontalLayout->addLayout(verticalLayout);

        MainWindow->setCentralWidget(centralwidget);
        menubar = new QMenuBar(MainWindow);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 613, 21));
        menuDebug = new QMenu(menubar);
        menuDebug->setObjectName(QString::fromUtf8("menuDebug"));
        menuFile = new QMenu(menubar);
        menuFile->setObjectName(QString::fromUtf8("menuFile"));
        menuHelp = new QMenu(menubar);
        menuHelp->setObjectName(QString::fromUtf8("menuHelp"));
        MainWindow->setMenuBar(menubar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        MainWindow->setStatusBar(statusBar);

        menubar->addAction(menuFile->menuAction());
        menubar->addAction(menuDebug->menuAction());
        menubar->addAction(menuHelp->menuAction());
        menuDebug->addAction(actionFixDpi);
        menuDebug->addAction(actionRelinking);
        menuDebug->addSeparator();
        menuDebug->addAction(actionDebug);
        menuDebug->addSeparator();
        menuDebug->addAction(actionSettings);
        menuDebug->addAction(actionExport);
        menuFile->addAction(actionNewProject);
        menuFile->addAction(actionOpenProject);
        menuFile->addSeparator();
        menuFile->addAction(actionSaveProject);
        menuFile->addAction(actionSaveProjectAs);
        menuFile->addSeparator();
        menuFile->addAction(actionCloseProject);
        menuFile->addSeparator();
        menuFile->addAction(actionQuit);
        menuHelp->addAction(actionAbout);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QApplication::translate("MainWindow", "MainWindow", 0, QApplication::UnicodeUTF8));
        actionDebug->setText(QApplication::translate("MainWindow", "Debug Mode", 0, QApplication::UnicodeUTF8));
        actionSaveProject->setText(QApplication::translate("MainWindow", "Save Project", 0, QApplication::UnicodeUTF8));
        actionSaveProject->setShortcut(QApplication::translate("MainWindow", "Ctrl+S", 0, QApplication::UnicodeUTF8));
        actionSaveProjectAs->setText(QApplication::translate("MainWindow", "Save Project As ...", 0, QApplication::UnicodeUTF8));
        actionSaveProjectAs->setShortcut(QApplication::translate("MainWindow", "Ctrl+Shift+S", 0, QApplication::UnicodeUTF8));
        actionNextPage->setText(QApplication::translate("MainWindow", "Next Page", 0, QApplication::UnicodeUTF8));
        actionNextPage->setShortcut(QApplication::translate("MainWindow", "PgDown", 0, QApplication::UnicodeUTF8));
        actionPrevPage->setText(QApplication::translate("MainWindow", "Previous Page", 0, QApplication::UnicodeUTF8));
        actionPrevPage->setShortcut(QApplication::translate("MainWindow", "PgUp", 0, QApplication::UnicodeUTF8));
        actionNewProject->setText(QApplication::translate("MainWindow", "New Project ...", 0, QApplication::UnicodeUTF8));
        actionNewProject->setShortcut(QApplication::translate("MainWindow", "Ctrl+N", 0, QApplication::UnicodeUTF8));
        actionOpenProject->setText(QApplication::translate("MainWindow", "Open Project ...", 0, QApplication::UnicodeUTF8));
        actionOpenProject->setShortcut(QApplication::translate("MainWindow", "Ctrl+O", 0, QApplication::UnicodeUTF8));
        actionPrevPageQ->setText(QApplication::translate("MainWindow", "Previous Page", 0, QApplication::UnicodeUTF8));
        actionPrevPageQ->setShortcut(QApplication::translate("MainWindow", "Q", 0, QApplication::UnicodeUTF8));
        actionNextPageW->setText(QApplication::translate("MainWindow", "Next Page", 0, QApplication::UnicodeUTF8));
        actionNextPageW->setShortcut(QApplication::translate("MainWindow", "W", 0, QApplication::UnicodeUTF8));
        actionCloseProject->setText(QApplication::translate("MainWindow", "Close Project", 0, QApplication::UnicodeUTF8));
        actionCloseProject->setShortcut(QApplication::translate("MainWindow", "Ctrl+W", 0, QApplication::UnicodeUTF8));
        actionQuit->setText(QApplication::translate("MainWindow", "Quit", 0, QApplication::UnicodeUTF8));
        actionQuit->setShortcut(QApplication::translate("MainWindow", "Ctrl+Q", 0, QApplication::UnicodeUTF8));
        actionSettings->setText(QApplication::translate("MainWindow", "Settings ...", 0, QApplication::UnicodeUTF8));
        actionFirstPage->setText(QApplication::translate("MainWindow", "First Page", 0, QApplication::UnicodeUTF8));
        actionFirstPage->setShortcut(QApplication::translate("MainWindow", "Home", 0, QApplication::UnicodeUTF8));
        actionLastPage->setText(QApplication::translate("MainWindow", "Last Page", 0, QApplication::UnicodeUTF8));
        actionLastPage->setShortcut(QApplication::translate("MainWindow", "End", 0, QApplication::UnicodeUTF8));
        actionAbout->setText(QApplication::translate("MainWindow", "About", 0, QApplication::UnicodeUTF8));
        actionFixDpi->setText(QApplication::translate("MainWindow", "Fix DPI ...", 0, QApplication::UnicodeUTF8));
        actionRelinking->setText(QApplication::translate("MainWindow", "Relinking ...", 0, QApplication::UnicodeUTF8));
        actionExport->setText(QApplication::translate("MainWindow", "Export...", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_STATUSTIP
        focusButton->setStatusTip(QApplication::translate("MainWindow", "Keep current page in view.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        focusButton->setText(QString());
#ifndef QT_NO_STATUSTIP
        thumbView->setStatusTip(QApplication::translate("MainWindow", "Use Home, End, PgUp (or Q), PgDown (or W) to navigate between pages.", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_STATUSTIP
        menuDebug->setTitle(QApplication::translate("MainWindow", "Tools", 0, QApplication::UnicodeUTF8));
        menuFile->setTitle(QApplication::translate("MainWindow", "File", 0, QApplication::UnicodeUTF8));
        menuHelp->setTitle(QApplication::translate("MainWindow", "Help", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
