#ifndef RAFT_MAIN_HH
#define RAFT_MAIN_HH

#include <QDebug>
#include <QString>
#include <QDateTime>
#include <QApplication>
#include <QDialog>
#include <QTimer>
#include <QTextEdit>
#include <QLineEdit>
#include <QHostInfo>
#include <QUdpSocket>
#include <QElapsedTimer>
#include <QVBoxLayout>
#include <vector>
#include <unistd.h>

#define ELECTION_TIMEOUT_MIN 0
#define ELECTION_TIMEOUT_MAX 0
#define HEARTBEAT_TIMEOUT 0
#define BROADCAST_TIMEOUT 0

enum states {
    FOLLOWER,    /* Node is a follower                   */
    CANDIDATE,   /* Node is a candidate to become leader */
    LEADER       /* Node is the leader                   */
};

enum messages {
    START,       /* User’s application starts participating in the protocol */
    MSG,         /* Send chat message to the chat room                      */
    GET_CHAT,    /* Print current chat history of the selected node         */
    STOP,        /* Stop the application from participating in the protocol */
    DROP,        /* Drop packets received from a particular node            */
    RESTORE,     /* Restore communication with a particular node            */
    GET_NODES    /* Get all node ids, show Raft state                       */
};

class RaftNode {
    public:
        RaftNode();

    private:
        enum states current_state;  /* Current node state */
        quint64 currentTerm;        /* Latest term the node has seen                       */
        quint64 votedFor;           /* CandidateId that received vote in current term      */
        quint64 commitIndex;        /* Index of highest log entry known to be committed    */
        quint64 lastApplied;        /* Index of highest log entry applied to state machine */
        std::vector<QString> nextIndex;   /* Per node, index of the next log entry to send to that node */
        std::vector<QString> matchIndex;  /* Per node, index of the highest log entry known to be replicated on that node */
};

struct LogEntry {
    quint64 currentTerm;
    quint64 seqNum;
    QString command;
    QDateTime recvByLeader;
};

struct PeerNode {
    quint64 peer;
    quint64 next_index;
    quint64 match_index;
    bool vote_granted;
    bool RPC_due;
    bool heartbeat_due;
};

class NetSocket : public QUdpSocket {
  Q_OBJECT

  public:
    // Initialize the net socket.
    NetSocket();

    // Bind this socket to a P2Papp-specific default port.
    bool bind();

    // Find ports.
    QList<quint16> findPorts();

  private:
    quint16 myPortMin;
    quint16 myPortMax;
    quint16 myPort;
    QList<quint16> ports;
};

////////

class ChatDialog : public QDialog {
  Q_OBJECT

  public:
    NetSocket *sock;
    ChatDialog(NetSocket*);

  public slots:
    void gotReturnPressed();
    void gotMessage();

  private:
    QTextEdit *textview;
    QLineEdit *textline;
    
    QString myOrigin;
};

#endif // RAFT_MAIN_HH
