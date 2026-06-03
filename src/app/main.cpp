#include "ui/MainWindow.h"

#include "app/AppContext.h"

#include <QApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    AppContext context;
    QString error;
    if (!context.start(&error)) {
        QMessageBox::critical(nullptr, QStringLiteral("AnDrop 启动失败"), error);
        return 1;
    }

    MainWindow window(&context);
    window.resize(960, 640);
    window.show();

    return app.exec();
}
