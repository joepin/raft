#ifndef RAFT_MAIN_HH
#define RAFT_MAIN_HH

#include <QDebug>
#include <QString>

enum states {
    FOLLOWER,     /* Node is a follower                   */
    CANDIDATE,    /* Node is a candidate to become leader */
    LEADER,       /* Node is the leader                   */
};


class RaftNode {
  public:
    RaftNode();

  private:
    quint64 current_term;
    enum states current_state;
};

#endif // RAFT_MAIN_HH
