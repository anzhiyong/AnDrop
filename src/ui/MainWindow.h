#pragma once

#include <QMainWindow>

class QListWidget;
class QStackedWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QListWidget *m_navigation = nullptr;
    QStackedWidget *m_pages = nullptr;
};
