#include <QApplication>
#include <QResource>
#include <QString>
#include "MainWindow.hpp"

int main(int argc, char *argv[])
{

  QApplication app(argc, argv);

  MainWindow *win = new MainWindow();
  win->show();

  return app.exec();
}
