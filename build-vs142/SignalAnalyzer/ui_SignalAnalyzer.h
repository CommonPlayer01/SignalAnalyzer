/********************************************************************************
** Form generated from reading UI file 'SignalAnalyzer.ui'
**
** Created by: Qt User Interface Compiler version 5.15.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SIGNALANALYZER_H
#define UI_SIGNALANALYZER_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_SignalAnalyzerClass
{
public:
    QWidget *centralWidget;
    QVBoxLayout *verticalLayout;
    QHBoxLayout *controlLayout;
    QPushButton *btnImportFolder;
    QLabel *lblFile;
    QComboBox *comboFile;
    QLabel *lblSignal;
    QComboBox *comboSignal;
    QLabel *lblWindow;
    QComboBox *comboWindow;
    QSpacerItem *horizontalSpacer;
    QCustomPlot *plot;
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *SignalAnalyzerClass)
    {
        if (SignalAnalyzerClass->objectName().isEmpty())
            SignalAnalyzerClass->setObjectName(QString::fromUtf8("SignalAnalyzerClass"));
        SignalAnalyzerClass->resize(1000, 650);
        centralWidget = new QWidget(SignalAnalyzerClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        verticalLayout = new QVBoxLayout(centralWidget);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(11, 11, 11, 11);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        controlLayout = new QHBoxLayout();
        controlLayout->setSpacing(6);
        controlLayout->setObjectName(QString::fromUtf8("controlLayout"));
        btnImportFolder = new QPushButton(centralWidget);
        btnImportFolder->setObjectName(QString::fromUtf8("btnImportFolder"));

        controlLayout->addWidget(btnImportFolder);

        lblFile = new QLabel(centralWidget);
        lblFile->setObjectName(QString::fromUtf8("lblFile"));

        controlLayout->addWidget(lblFile);

        comboFile = new QComboBox(centralWidget);
        comboFile->setObjectName(QString::fromUtf8("comboFile"));
        comboFile->setMinimumSize(QSize(180, 0));

        controlLayout->addWidget(comboFile);

        lblSignal = new QLabel(centralWidget);
        lblSignal->setObjectName(QString::fromUtf8("lblSignal"));

        controlLayout->addWidget(lblSignal);

        comboSignal = new QComboBox(centralWidget);
        comboSignal->setObjectName(QString::fromUtf8("comboSignal"));
        comboSignal->setMinimumSize(QSize(140, 0));

        controlLayout->addWidget(comboSignal);

        lblWindow = new QLabel(centralWidget);
        lblWindow->setObjectName(QString::fromUtf8("lblWindow"));

        controlLayout->addWidget(lblWindow);

        comboWindow = new QComboBox(centralWidget);
        comboWindow->setObjectName(QString::fromUtf8("comboWindow"));
        comboWindow->setMinimumSize(QSize(140, 0));

        controlLayout->addWidget(comboWindow);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        controlLayout->addItem(horizontalSpacer);


        verticalLayout->addLayout(controlLayout);

        plot = new QCustomPlot(centralWidget);
        plot->setObjectName(QString::fromUtf8("plot"));
        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(plot->sizePolicy().hasHeightForWidth());
        plot->setSizePolicy(sizePolicy);

        verticalLayout->addWidget(plot);

        SignalAnalyzerClass->setCentralWidget(centralWidget);
        menuBar = new QMenuBar(SignalAnalyzerClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        SignalAnalyzerClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(SignalAnalyzerClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        SignalAnalyzerClass->addToolBar(mainToolBar);
        statusBar = new QStatusBar(SignalAnalyzerClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        SignalAnalyzerClass->setStatusBar(statusBar);

        retranslateUi(SignalAnalyzerClass);

        QMetaObject::connectSlotsByName(SignalAnalyzerClass);
    } // setupUi

    void retranslateUi(QMainWindow *SignalAnalyzerClass)
    {
        SignalAnalyzerClass->setWindowTitle(QCoreApplication::translate("SignalAnalyzerClass", "\344\277\241\345\217\267\345\210\206\346\236\220\345\231\250", nullptr));
        btnImportFolder->setText(QCoreApplication::translate("SignalAnalyzerClass", "\345\257\274\345\205\245\346\225\260\346\215\256\346\272\220\346\226\207\344\273\266\345\244\271", nullptr));
        lblFile->setText(QCoreApplication::translate("SignalAnalyzerClass", "\346\226\207\344\273\266\351\200\211\346\213\251", nullptr));
        lblSignal->setText(QCoreApplication::translate("SignalAnalyzerClass", "\344\277\241\345\217\267\351\200\211\346\213\251", nullptr));
        lblWindow->setText(QCoreApplication::translate("SignalAnalyzerClass", "\347\252\227\345\207\275\346\225\260\351\200\211\346\213\251", nullptr));
    } // retranslateUi

};

namespace Ui {
    class SignalAnalyzerClass: public Ui_SignalAnalyzerClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SIGNALANALYZER_H
