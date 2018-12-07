#include <unistd.h>
#include <QApplication>

#include "main.hh"

int main(int argc, char **argv) {
  // Initialize Qt toolkit
  QApplication app(argc,argv);

  qDebug() << "Hello World!";

  // Enter the Qt main loop; everything else is event driven
  return app.exec();
}
