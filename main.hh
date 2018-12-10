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

#define ELECTION_TIMEOUT_MIN 1500
#define ELECTION_TIMEOUT_MAX 3000
#define HEARTBEAT_INTERVAL   200

/* Forward declaration. */
class RaftNode;


////////


enum states {
    FOLLOWER,           /* Node is a follower.                                      */
    CANDIDATE,          /* Node is a candidate to become leader.                    */
    LEADER              /* Node is the leader.                                      */
};

enum messages {
    START,              /* User’s application starts participating in the protocol. */
    MSG,                /* Send chat message to the chat room.                      */
    GET_CHAT,           /* Print current chat history of the selected node.         */
    STOP,               /* Stop the application from participating in the protocol. */
    DROP,               /* Drop packets received from a particular node.            */
    RESTORE,            /* Restore communication with a particular node.            */
    GET_NODES           /* Get all node ids, show Raft state.                       */
};


////////


class NetSocket : public QUdpSocket {
    Q_OBJECT

    public:
        NetSocket();                             /* Initialize the net socket.                           */
        bool bind();                             /* Bind socket to a port.                               */
        void setPorts();                         /* Set list of neighboring ports.                       */
        QList<quint16> getPorts();               /* Return list of neighboring ports.                    */
        quint16 getMyPort();                     /* Return bound port.                                   */
        qint64 writeDatagram(QByteArray*, int, quint16);     /* Send a datagram.                         */
    
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
        ChatDialog(NetSocket *, RaftNode *);     /* Create chat dialog window.                           */
        void clearTextline();                    /* Clear input textline.                                */
        void addMessage(QString);                /* Add messaege to chat dialog window.                  */
        QString getTextline();                   /* Return the input textline after user presses enter.  */

    private:
        QTextEdit *textview;
        QLineEdit *textline;
};


////////


struct LogEntry {
    quint64 term;                                /* Term when entry was received by the leader.          */
    QString command;                             /* Command for state machine.                           */
};


////////


class RaftNode : public QObject {
    Q_OBJECT

    public:
        RaftNode(NetSocket *);
        QString getID();
        void setDialog(ChatDialog *);

    public slots:
        void receiveMessage();                   /* Handler for when node receives a packet.             */
        void receiveCommand();                   /* Handler for when client inputs a message.            */
        void electionTimeoutHandler();           /* Handler for when the election timeout is triggered.  */
        void auxPrint();                         /* Helper to print known values.                        */

    private:
        NetSocket *sock;
        ChatDialog *dialogWindow;

        QString nodeID;                          /* Current node ID (port)                               */
        enum states currentState;                /* Current node state.                                  */
        quint64 currentTerm;                     /* Latest term the node has seen.                       */
        QString votedFor;                        /* CandidateId that received vote in current term.      */
        quint64 commitIndex;                     /* Index of highest log entry known to be committed.    */
        quint64 lastApplied;                     /* Index of highest log entry applied to state machine. */
        QString currentLeader;                   /* Port of the current leader.                          */
        
        QList<quint16> neighborPorts;            /* List of all neighbor ports within range.             */

        bool protocolRunning;                    /* T/F is node is running the protocol.                 */

        QTimer *electionTimer;                   /* Timer that handles elections. Reset whenever heartbeat is received. */
        quint64 electionTimeout;                 /* Election timeout value.                                             */
        quint64 heartbeatInterval;               /* Leader's heartbeat interval value.                                  */

        QList<QString> knownNodes;               /* List of all known active nodes (ports).                                          */
        QList<QString> droppedNodes;             /* List of dropped (ignored) nodes (ports).                                         */
        std::map<QString, quint64> nextIndex;    /* Per node, index of the next log entry to send to that node.                      */
        std::map<QString, quint64> matchIndex;   /* Per node, index of the highest log entry known to be replicated on that node.    */

        std::vector<LogEntry> log;               /* Log entries for the state machine.                                               */
        std::vector<QString> messagesQueue;      /* Queue of messages accumulated when application is not participating in protocol. */ 

        void requestVoteRPC();                   /* Invoked by candidates to gather votes.                              */
        void appendEntriesRPC();                 /* Invoked by leader to replicate log entries. Also used as heartbeat. */

        void getChat();                          /* Print current chat history (logs with consensus) of current node.   */
        void getNodes();                         /* Print all nodes, current node state, and leader ID (port).          */

        void sendMessage(QString);               /* Send message from the node to the leader.            */

        void stopProtocol();                     /* Stop participation in the Raft protocol.             */
        void startProtocol();                    /* Start participation in the Rat protocol.             */

        void dropComms(QString);                 /* Ignore packet from a given node.                     */
        void restoreComms(QString);              /* Stop ignoring packets from a given nodes.            */

        void startElectionTimer();               /* Start election timeout.                              */
        void restartElectionTimer();             /* Restart (reset) the election timeout.                */
        
        void startPrintTimer();                  /* Start timer for helper printing function.            */
        void usage();                            /* Print usage message in CLI for client.               */

};


////////


#endif // RAFT_MAIN_HH
