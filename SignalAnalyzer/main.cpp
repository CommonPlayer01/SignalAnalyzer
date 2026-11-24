#include "SignalAnalyzer.h"

#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SignalAnalyzer window;
    window.show();
    return app.exec();
}
