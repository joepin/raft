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

#define ELECTION_TIMEOUT_MIN 10000
#define ELECTION_TIMEOUT_MAX 15000
#define HEARTBEAT_INTERVAL   1000

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
        quint16 getID();
        void setDialog(ChatDialog *);

    public slots:
        void receiveMessage();                   /* Handler for when node receives a packet.             */
        void receiveCommand();                   /* Handler for when client inputs a message.            */
        void electionTimeoutHandler();           /* Handler for when the election timeout is triggered.  */
        void heartbeatTimeoutHandler();          /* Handler for when the heartbeat timeout is triggered. */
        void auxPrint();                         /* Helper to print known values.                        */

    private:
        NetSocket *sock;
        ChatDialog *dialogWindow;

        QString txnID;                           /* Transaction ID of last message sent.                 */
        quint16 nodeID;                          /* Current node ID (port)                               */
        enum states currentState;                /* Current node state.                                  */
        quint64 currentTerm;                     /* Latest term the node has seen.                       */
        quint16 votedFor;                        /* CandidateId that received vote in current term.      */
        quint64 commitIndex;                     /* Index of highest log entry known to be committed.    */
        quint64 lastApplied;                     /* Index of highest log entry applied to state machine. */
        QString currentLeader;                   /* Port of the current leader.                          */
        quint64 numVotesRcvd;                    /* Number of votes node has received to become leader.  */
        quint64 majority;                        /* Majority needed for election.                        */

        bool protocolRunning;                    /* T/F is node is running the protocol.                 */

        QTimer *electionTimer;                   /* Timer that handles elections. Reset whenever heartbeat is received. */
        QTimer *heartbeatTimer;                  /* Timer that sends out heartbeats.                                    */
        quint64 electionTimeout;                 /* Election timeout value.                                             */
        quint64 heartbeatInterval;               /* Leader's heartbeat interval value.                                  */

        QList<quint16> knownNodes;               /* List of all known active nodes (ports).                                          */
        QList<QString> droppedNodes;             /* List of dropped (ignored) nodes (ports).                                         */
        QMap<QString, quint64> nextIndex;        /* Per node, index of the next log entry to send to that node.                      */
        QMap<QString, quint64> matchIndex;       /* Per node, index of the highest log entry known to be replicated on that node.    */

        std::vector<LogEntry> log;               /* Log entries for the state machine.                                               */
        std::vector<QString> messagesQueue;      /* Queue of messages accumulated when application is not participating in protocol. */ 

        void becomeFollower();                   /* Transition to the follower state.                                   */
        void becomeCandidate();                  /* Invoked by candidates to gather votes.                              */
        void becomeLeader();                     /* Transition to the Leader state.                                     */

        void sendVotes();                        /* Invoked by Candidate to send votes to become leader.                */
        void appendEntries();                    /* Invoked by Leader to replicate log entries. Also used as heartbeat. */

        void getChat();                          /* Print current chat history (logs with consensus) of current node.   */
        void getNodes();                         /* Print all nodes, current node state, and leader ID (port).          */

        void sendChat(QString);                  /* Send chat message from the node to the leader.       */
        void sendBacklog();                      /* Send backlog of messages from messagesQueue.         */

        void stopProtocol();                     /* Stop participation in the Raft protocol.             */
        void startProtocol();                    /* Start participation in the Rat protocol.             */

        void dropComms(QString);                 /* Ignore packet from a given node.                     */
        void restoreComms(QString);              /* Stop ignoring packets from a given nodes.            */

        void startElectionTimer();               /* Start election timeout.                              */
        void stopElectionTimer();                /* Stop election timeout.                               */
        void restartElectionTimer();             /* Restart (reset) the election timeout.                */
        
        void startHeartbeatTimer();               /* Start heartbeat timeout.                            */
        void stopHeartbeatTimer();                /* Stop heartbeat timeout.                             */
        void restartHeartbeatTimer();             /* Restart (reset) the heartbeat timeout.              */

        void startPrintTimer();                  /* Start timer for helper printing function.            */
        void usage();                            /* Print usage message in CLI for client.               */

};


////////


// 
// Message definitions:
// 
// AppendEntries message from the Leader:
// {
//     "type": "AppendEntries"
//     "term": "",             Leader’s term
//     "leaderId": "",         So follower can redirect clients
//     "prevLogIndex": "",     Index of log entry immediately preceding new ones
//     "prevLogTerm": "",      Term of prevLogIndex entry
//     "entries": "",          Entries to store (empty for heartbeat)
//     "leaderCommit": ""      Leader’s commitIndex
// }
// 
// AppendEntries ACK from the nodes:
// {
//     "type": "AppendEntriesACK"
//     "term": "",             currentTerm, for leader to update itself
//     "success": ""           True if follower contained entry matching prevLogIndex and prevLogTerm
// }
//
// RequestVote message from candidates:
// {
//     "type": "RequestVote"
//     "term": "",             Candidate’s term
//     "candidateId": "",      Candidate requesting vote
//     "lastLogIndex": "",     Index of candidate’s last log entry
//     "lastLogTerm": ""       Term of candidate’s last log entry 
// }
// 
// RequestVote ACK from the nodes:
// {
//     "type": "RequestVoteACK"
//     "term": "",             currentTerm, for candidate to update itself
//     "voteGranted": ""       True means candidate received vote
// }
// 
// Message from a follower:
// {
//     "type": "Message"
//     "entries": ""           Log entries to forward to the Leader (empty for startup)
//     "txnID": ""             Transaction ID
// }
// 
// Message ACK from the nodes:
// {
//     "type": "MessageACK"
//     "leaderId": "",         So follower can redirect clients
//     "txnID": ""             Transaction ID
// }
// 

#endif // RAFT_MAIN_HH
