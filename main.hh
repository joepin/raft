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
#include <map>
#include <unistd.h>
#include <iostream>

#define ELECTION_TIMEOUT_MIN 150
#define ELECTION_TIMEOUT_MAX 300
#define HEARTBEAT_TIMEOUT 2000

// Forward declaration.
class RaftNode;


////////


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


////////


class NetSocket : public QUdpSocket {
    Q_OBJECT

    public:
        NetSocket();                             /* Initialize the net socket.          */
        bool bind();                             /* Bind socket to a port.              */
        void setPorts();                         /* Set list of neighboring ports.      */
        QList<quint16> getPorts();               /* Return list of neighboring ports.   */
        quint16 getMyPort();                     /* Return bound port.                  */
        qint64 writeDatagram(QByteArray*, int, quint16);     /* Send a datagram         */
    
    private:
        quint16 myPortMin;
        quint16 myPortMax;
        quint16 myPort;
        QList<quint16> neighborPorts;
};


////////


class ChatDialog : public QDialog {
    Q_OBJECT

    public:
        ChatDialog(NetSocket *, RaftNode *);     /* Create chat dialog window.                         */
        void clearTextline();                    /* Clear input textline.                              */
        void addMessage(QString);                /* Add messaege to chat dialog window.                */
        QString getTextline();                   /* Return the input textline after user presses enter */

    private:
        QTextEdit *textview;
        QLineEdit *textline;
};


////////


class RaftNode : public QObject {
    Q_OBJECT

    public:
        RaftNode(NetSocket *);
        QString getID();
        void setDialog(ChatDialog *);

    public slots:
        void receiveMessage();
        void receiveCommand();

    private:
        ChatDialog *dialogWindow;

        NetSocket *sock;

        QString nodeID;
        QList<quint16> neighborPorts;
        std::map<QString, quint16> knownNeighbors;

        enum states current_state;               /* Current node state                                  */
        quint64 currentTerm;                     /* Latest term the node has seen                       */
        quint64 votedFor;                        /* CandidateId that received vote in current term      */
        quint64 commitIndex;                     /* Index of highest log entry known to be committed    */
        quint64 lastApplied;                     /* Index of highest log entry applied to state machine */
        
        quint64 electionTimeout;
        quint64 heartbeatTimeout;

        std::map<QString, quint64> nextIndex;    /* Per node, index of the next log entry to send to that node                   */
        std::map<QString, quint64> matchIndex;   /* Per node, index of the highest log entry known to be replicated on that node */

        void usage();

        void sendMessage(QString);

        void getChat();

        void stopProtocol();
        void startProtocol();

        void dropComms(QString);
        void restoreComms(QString);

        void getNodes();
};


////////


#endif // RAFT_MAIN_HH




// struct LogEntry {
//     quint64 currentTerm;
//     quint64 seqNum;
//     QString command;
//     QDateTime recvByLeader;
// };

// struct PeerNode {
//     quint64 peer;
//     quint64 next_index;
//     quint64 match_index;
//     bool vote_granted;
//     bool RPC_due;
//     bool heartbeat_due;
// };
