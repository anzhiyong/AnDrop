#include <QApplication>
#include <QLabel>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QLabel label(QStringLiteral("AnDrop"));
    label.resize(360, 180);
    label.show();

    return app.exec();
}
