#include <unistd.h>
#include <QApplication>

#include "main.hh"

RaftNode::RaftNode() {
  qDebug() << "Welcome from your new node!";
  current_term = 0;
}

int main(int argc, char **argv) {
  // Initialize Qt toolkit
  QApplication app(argc,argv);

  RaftNode *node = new RaftNode();

  // Enter the Qt main loop; everything else is event driven
  return app.exec();
}
