#ifndef RAFT_MAIN_HH
#define RAFT_MAIN_HH

#include <QDebug>

class RaftNode {
  public:
    RaftNode();

  private:
    quint64 current_term;
};

#endif // RAFT_MAIN_HH
