/********************************************************************************
** Form generated from reading UI file 'SettingsDialog.ui'
**
** Created by: Qt User Interface Compiler version 4.8.7
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SETTINGSDIALOG_H
#define UI_SETTINGSDIALOG_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QDialog>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QHeaderView>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>

QT_BEGIN_NAMESPACE

class Ui_SettingsDialog
{
public:
    QVBoxLayout *verticalLayout;
    QCheckBox *use3DAcceleration;
    QCheckBox *AutoSaveProject;
    QCheckBox *DontEqualizeIlluminationPicZones;
    QSpacerItem *verticalSpacer;
    QDialogButtonBox *buttonBox;

    void setupUi(QDialog *SettingsDialog)
    {
        if (SettingsDialog->objectName().isEmpty())
            SettingsDialog->setObjectName(QString::fromUtf8("SettingsDialog"));
        SettingsDialog->resize(395, 183);
        verticalLayout = new QVBoxLayout(SettingsDialog);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        use3DAcceleration = new QCheckBox(SettingsDialog);
        use3DAcceleration->setObjectName(QString::fromUtf8("use3DAcceleration"));

        verticalLayout->addWidget(use3DAcceleration);

        AutoSaveProject = new QCheckBox(SettingsDialog);
        AutoSaveProject->setObjectName(QString::fromUtf8("AutoSaveProject"));

        verticalLayout->addWidget(AutoSaveProject);

        DontEqualizeIlluminationPicZones = new QCheckBox(SettingsDialog);
        DontEqualizeIlluminationPicZones->setObjectName(QString::fromUtf8("DontEqualizeIlluminationPicZones"));

        verticalLayout->addWidget(DontEqualizeIlluminationPicZones);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer);

        buttonBox = new QDialogButtonBox(SettingsDialog);
        buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

        verticalLayout->addWidget(buttonBox);


        retranslateUi(SettingsDialog);
        QObject::connect(buttonBox, SIGNAL(accepted()), SettingsDialog, SLOT(accept()));
        QObject::connect(buttonBox, SIGNAL(rejected()), SettingsDialog, SLOT(reject()));

        QMetaObject::connectSlotsByName(SettingsDialog);
    } // setupUi

    void retranslateUi(QDialog *SettingsDialog)
    {
        SettingsDialog->setWindowTitle(QApplication::translate("SettingsDialog", "Settings", 0, QApplication::UnicodeUTF8));
        use3DAcceleration->setText(QApplication::translate("SettingsDialog", "Use 3D acceleration for user interface", 0, QApplication::UnicodeUTF8));
        AutoSaveProject->setText(QApplication::translate("SettingsDialog", "Auto-save the existing project", 0, QApplication::UnicodeUTF8));
        DontEqualizeIlluminationPicZones->setText(QApplication::translate("SettingsDialog", "Don't equalize the illumination in picture zones", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SettingsDialog: public Ui_SettingsDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SETTINGSDIALOG_H
