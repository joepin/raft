#include "main.hh"


////////


// Construct the RaftNode and initialize to defaults.
RaftNode::RaftNode(NetSocket *netSock) {
    qDebug() << "RaftNode::RaftNode: Welcome from your new node!";

    // Set the port configs.
    sock   = netSock;
    nodeID = QString::number(sock->getMyPort());
    QList<quint16> neighborPorts = sock->getPorts();

    // Add list of all nodes within the range to knownNodes.
    for (auto const& x : neighborPorts) {
        knownNodes.push_back(x);
    }

    // Set default values. 
    currentTerm = 0;
    commitIndex = 0;
    lastApplied = 0;
    votedFor    = "";
    numVotesRcvd    = 0;   
    currentLeader   = "";
    protocolRunning = false;

    // Set timeouts.
    electionTimeout   = rand() % (ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN + 1) + ELECTION_TIMEOUT_MIN;
    heartbeatInterval = HEARTBEAT_INTERVAL;

    // Set timers.
    electionTimer = new QTimer();
    connect(electionTimer, SIGNAL(timeout()), this, SLOT(electionTimeoutHandler()));

    heartbeatTimer = new QTimer();
    connect(heartbeatTimer, SIGNAL(timeout()), this, SLOT(heartbeatTimeoutHandler()));

    // Default to a follower.
    currentState = FOLLOWER;
}

// Print usage to CLI.
void RaftNode::usage() {
    std::cout << "Error. Unrecognizable command. Expected usage: " << std::endl
          << "  <START>  " << std::endl
          << "  <MSG message>  " << std::endl
          << "  <GET_CHAT>  " << std::endl
          << "  <STOP>  " << std::endl
          << "  <DROP node_id>  " << std::endl
          << "  <RESTORE node_id>  " << std::endl
          << "  <GET_NODES>  " << std::endl;
}

// Callback when user presses "Enter" in the textline widget.
void RaftNode::receiveCommand() {
    QString message = dialogWindow->getTextline();

    // Handle message type.
    QString command = message.left(message.indexOf(" "));
    qDebug() << "RaftNode::receiveCommand: Handling command: " << command;

    // If user enters one of these commands, ignore any trailing words.
    if (command == "START") {
        dialogWindow->clearTextline();
        startProtocol();
        return;
    }
    else if (command == "GET_CHAT") {
        dialogWindow->clearTextline();
        getChat();
        return;
    }
    else if (command == "STOP") {
        dialogWindow->clearTextline();
        stopProtocol();
        return;
    }
    else if (command == "GET_NODES") {
        dialogWindow->clearTextline();
        getNodes();
        return;
    }

    // Remaining command options expect at least two words.
    if (message.indexOf(" ") == -1) {
        usage();
        return;
    }

    // Get remaining message parameters.
    message = message.mid(message.indexOf(" ") + 1);
    if (message.isNull() || message.isEmpty()) {
        usage();
        return;            
    }

    if (command == "MSG") {
        dialogWindow->clearTextline();
        sendChat(message);
    }
    else if (command == "DROP") {
        dialogWindow->clearTextline();
        dropComms(message);
    }
    else if (command == "RESTORE") {
        dialogWindow->clearTextline();
        restoreComms(message);
    }
    else {
        usage();
        return;
    }
}

// @TODO - Send message from the chatroom.
void RaftNode::sendChat(QString message) {
    qDebug() << "RaftNode::sendChat";

    // If not participating in the protocol, store all the messages to send
    // once participation begins again.
    if (!protocolRunning) {
        qDebug() << "RaftNode::sendChat: Message added to queue: " << message;
        messagesQueue.push_back(message);
        return;
    }

    switch (currentState) {
        case FOLLOWER:
            break;
        case CANDIDATE:
            break;
        case LEADER:
            break;
    }
}

// @TODO - Send backlog of messages from messagesQueue.
void RaftNode::sendBacklog() {
    qDebug() << "RaftNode::sendBacklog";
}

// @TODO - Callback when receiving a message from the socket. 
void RaftNode::receiveMessage() {
    qDebug() << "RaftNode::receiveMessage";

    // Read each datagram.
    while (sock->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(sock->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        QVariantMap message;

        sock->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // If not participating in the protocol, drop all incoming messages. 
        if (!protocolRunning) {
            qDebug() << "RaftNode::receiveMessage: Dropping incoming message.";
            return;
        }

        // If ignoring the requesting node, drop the incoming packet. 
        if (droppedNodes.contains(QString::number(senderPort))) {
            qDebug() << "RaftNode::receiveMessage: Dropping incoming message.";
            return;
        }

        QDataStream stream(&datagram, QIODevice::ReadOnly);

        stream >> message;

        switch (currentState) {
            // Respond to RPC's from Candidates and Leaders. 
            case FOLLOWER:
                // Respond to a vote request. 
                if (message["type"] == "RequestVote") {
                    qDebug() << "RaftNode::receiveMessage: Received vote request in Follower state.";
                
                    // Serialize the response.
                    QByteArray buf;
                    QDataStream datastream(&buf, QIODevice::ReadWrite);
                    QVariantMap response;

                    response["type"] = "RequestVoteACK";
                    response["term"] = currentTerm;
                    response["voteGranted"] = false;

                    // If Candidate is in the same or newer term:
                    if (message["term"] >= currentTerm) {
                        // If votedFor is null or candidateId
                        if (votedFor == "" || votedFor == message["candidateId"]) {
                            // And candidate’s log is at least as up-to-date as receiver’s log
                            if (log.size() > 0) {
                                if (message["lastLogIndex"].toInt() >= (int)log.size()) {
                                    if (message["lastLogTerm"].toInt() == (int)log.back().term) {
                                        response["voteGranted"] = true;
                                    }
                                }
                            }
                        }
                    }

                    // Return the response.
                    datastream << response;
                    sock->writeDatagram(&buf, buf.size(), senderPort);
                }
                return;
            // Handle incoming votes. 
            // If AppendEntries received from a new leader, convert to a follower. 
            case CANDIDATE:
                // Handle a vote. 
                if (message["type"] == "RequestVoteACK") {
                    qDebug() << "RaftNode::receiveMessage: Received vote in Candidate state.";
                    if (message["voteGranted"] == true) {
                        numVotesRcvd++;
                    }
                }
                // becomeFollower();
                // becomeLeader();
                break;
            case LEADER:
                // becomeFollower();
                break;
        }

        datagram.clear();
    }

    // handleReceivedMessage(message, senderPort);
}

// void RaftNode::handleReceivedMessage(QVariantMap message, quint16 port) {
//     // @TODO - Handle message type.
//     // Check if we have a messageACK. Check if it contains leader. If it does, stop discovery.
//     // If we have a heartbeat, restart the election timeout.

//     switch (message.type) {
//         case "AppendEntries":
//             // needs to be handled by all followers; only sent by leaders
//             handleAppendEntriesRPC(message, port);
//             break;
//         case "AppendEntriesACK":
//             // needs to be handled by leaders only; response from followers to an RPC
//             handleAppendEntriesACK(message, port);

//     switch (currentState) {
//         case FOLLOWER:
//             break;
//         case CANDIDATE:
//             break;
//         case LEADER:
//             break;
//     }
// }

// Transition to a follower.
void RaftNode::becomeFollower() {
    qDebug() << "RaftNode::becomeFollower";

    // Sanity check that timeouts are not running. 
    stopElectionTimer();
    stopHeartbeatTimer();

    switch (currentState) {
        case FOLLOWER:
            qDebug() << "RaftNode::becomeFollower: Internal error - transitioning to Follower when state is Follower.";
            break;
        case CANDIDATE:
            qDebug() << "RaftNode::becomeFollower: Transitioning to Follower from Candidate.";
            currentState = FOLLOWER;
            break;
        case LEADER:
            qDebug() << "RaftNode::becomeFollower: Transitioning to Follower from Leader.";
            currentState = FOLLOWER;
            break;
    }
}

// Transition to candidate.
void RaftNode::becomeCandidate() {
    qDebug() << "RaftNode::becomeCandidate";

    // Sanity check that timeouts are not running. 
    stopElectionTimer();
    stopHeartbeatTimer();

    switch (currentState) {
        case FOLLOWER:
            qDebug() << "RaftNode::becomeCandidate: Transitioning to Candidate from Follower.";
            currentState = CANDIDATE;
            break;
        case CANDIDATE:
            qDebug() << "RaftNode::becomeCandidate: Internal error - transitioning to Candidate when state is Candidate.";
            break;
        case LEADER:
            qDebug() << "RaftNode::becomeCandidate: Internal error - transitioning to Candidate when state is Leader.";
            break;
    }

}

// // to be invoked by leaders to generate a message for each neighbor
// QVariantMap RaftNode::createAppendEntriesRPC(quint16 port) {
//     quint64 nextIndexForPort = nextIndex[port];
//     QVariantMap message;
//     if (nextIndexForPort - 1 == commitIndex) {
//         // logs are up to date, send a heartbeat
//         message["term"] = currentTerm;
//         message["leaderId"] = nodeID;
//         message["prevLogIndex"] = nextIndexForPort - 1;
//         message["prevLogTerm"] = log[nextIndexForPort - 1].term;
//         message["entries"] = std::vector<LogEntry>();
//         message["leaderCommit"] = commitIndex;
//     } else {
//         // need to send new entries
//         message["term"] = currentTerm;
//         message["leaderId"] = nodeID;
//         message["prevLogIndex"] = nextIndexForPort - 1;
//         message["prevLogTerm"] = log[nextIndexForPort - 1].term;
//         message["entries"] = getAllEntriesFromIndex(nextIndexForPort - 1);
//         message["leaderCommit"] = commitIndex;
//     }
//     return message;
// }

// void RaftNode::handleAppendEntriesRPC(QVariantMap message, quint16 leaderPort) {
//     if (message.term < currentTerm) {
//         sendAppendEntriesACK(currentTerm, false, leaderPort);
//         return;
//     }
// }

// @TODO - Transition to leader.
void RaftNode::becomeLeader() {
    qDebug() << "RaftNode::becomeLeader";

    // Sanity check that timeouts are not running. 
    stopElectionTimer();
    stopHeartbeatTimer();

    switch (currentState) {
        case FOLLOWER:
            qDebug() << "RaftNode::becomeLeader: Internal error - transitioning to Leader when state is Follower.";
            return;
        case CANDIDATE:
            qDebug() << "RaftNode::becomeLeader: Transitioning to Leader from Candidate.";
            currentState = LEADER;
            break;
        case LEADER:
            qDebug() << "RaftNode::becomeLeader: Internal error - transitioning to Leader when state is Leader.";
            break;
    }

    // Reinitialize the nextIndex.
    // nextIndex[] for each server, index of the next log entry to send to that server (initialized to leader last log index + 1)

    // Reinitialize the matchIndex. 
    // matchIndex[] for each server, index of highest log entry known to be replicated on server (initialized to 0, increases monotonically)
}

// void RaftNode::handleAppendEntriesRPC(QVariantMap message, quint16 leaderPort) {
//     if (message.term < currentTerm) {
//         sendAppendEntriesACK(currentTerm, false, leaderPort);
//         return;
//     }

//     if (log[message.prevLogIndex].term != message.prevLogTerm) {
//         deleteAllEntriesFromIndex(message.prevLogIndex);
//         appendAllEntriesToLog(message.entries);
//         if (message.leaderCommit > commitIndex) {
//             commitIndex = min(message.leaderCommit, log.size());
//         }
//         sendAppendEntriesACK(currentTerm, false, leaderPort);
//         return;
//     }
// }

// void RaftNode::sendAppendEntriesACK(quint64 term, bool result, quint16 port) {
//     QByteArray buf;
//     QDataStream datastream(&buf, QIODevice::ReadWrite);

//     QVariantMap message;
//     message["term"] = term;
//     message["success"] = result;

//     datastream << message;

//     // Send message to the socket.
//     sock->writeDatagram(&buf, buf.size(), port);
// }

// void RaftNode::appendAllEntriesToLog(std::vector<LogEntry> entries) {
//     for (auto entry : entries) {
//         log.append(entry);
//     }
// }

// void RaftNode::deleteAllEntriesFromIndex(quint64 index) {
//     auto::iterator it = log.begin() + index;
//     while (it != log.end()) {
//         it = log.erase(it);
//     }
// }

// void RaftNode::handleAppendEntriesACK(QVariantMap message, quint16 senderPort) {

// }

// void RaftNode::appendAllEntriesToLog(std::vector<LogEntry> entries) {
//     for (auto entry : entries) {
//         log.append(entry);
//     }
// }

// std::vector<LogEntry> RaftNode::getAllEntriesFromIndex(quint64 index) {
//     std::vector<LogEntry> entries = std::vector<LogEntry>();
//     for (auto::iterator it = log.begin() + index; it != log.end(); ++it) {
//         entries.push_back(*it);
//     }
//     return entries;
// }

// Send votes for election.
void RaftNode::sendVotes() {
    qDebug() << "RaftNode::sendVotes";

    QByteArray buf;
    QDataStream datastream(&buf, QIODevice::ReadWrite);
    QVariantMap message;

    // Increment currentTerm.
    currentTerm++;

    // Vote for self in this term.
    votedFor = nodeID;
    numVotesRcvd = 1;

    // Reset election timer.
    startElectionTimer();

    // Get last log entries.
    if (log.size() > 0) {
        // Log index starts at 1.
        quint64 logSize = log.size(); 
        message["lastLogIndex"] = logSize;
        message["lastLogTerm"]  = log.back().term;
    } else {
        // Log is empty.
        message["lastLogIndex"] = 0;
        message["lastLogTerm"]  = 0;
    }

    // Serialize the message
    message["type"] = "RequestVote";
    message["term"] = currentTerm;
    message["candidateId"] = nodeID;

    // Generate a transaction ID.
    qsrand((uint) QDateTime::currentMSecsSinceEpoch());
    txnID = nodeID + QString::number(qrand());
    message["txnID"] = txnID;

    datastream << message;

    // Iterate over knownNodes and request votes.
    for (auto const& port : knownNodes) {
        sock->writeDatagram(&buf, buf.size(), port);
    }

}

// @TODO - Replicate log entries.
void RaftNode::appendEntries() {
    qDebug() << "RaftNode::appendEntriesRPC";

    switch (currentState) {
        case FOLLOWER:
            break;
        case CANDIDATE:
            break;
        case LEADER:
            break;
    }
}

// @TODO - Start participating in Raft protocol.
void RaftNode::startProtocol() {
    qDebug() << "RaftNode::startProtocol";
    protocolRunning = true;

    // @TODO - Reset to default values (?)
    currentState = FOLLOWER;

    // Begin printing for debugging.
    startPrintTimer();

    // Begin the election timeout. 
    // (In this implementation, the node knows of all neighboring ports already.)
    startElectionTimer();
}

// Stop participating in Raft protocol.
void RaftNode::stopProtocol() {
    qDebug() << "RaftNode::stopProtocol";
    
    protocolRunning = false;

    // Stop protocol elections.
    stopElectionTimer();

    // Stop heartbeat timeout. 
    stopHeartbeatTimer();
}

// Drop packets from targetNode.
void RaftNode::dropComms(QString targetNode) {
    qDebug() << "RaftNode::dropComms";

    QString message = "Ignoring Node "+ targetNode + ".";

    if (droppedNodes.contains(targetNode)) {
        qDebug() << "RaftNode::dropComms: Already ignoring" << targetNode << ".";
    } else {
        qDebug() << "RaftNode::dropComms: Ignoring" << targetNode << ".";
        droppedNodes.append(targetNode);
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// Restore communications (no longer drop packets) from targetNode.
void RaftNode::restoreComms(QString targetNode) {
    qDebug() << "RaftNode::restoreComms";

    QString message = "";

    if (droppedNodes.contains(targetNode)) {
        message += "Restoring communication with Node "+ targetNode + ".";
        qDebug() << "RaftNode::restoreComms: Restoring communication with" << targetNode << ".";
        droppedNodes.removeAll(targetNode);
    } else {
        message += "Cannot restore - Node "+ targetNode + " has not been dropped.";
        qDebug() << "RaftNode::restoreComms: Communication with" << targetNode << "has not been dropped.";
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// Print chat history. Only show messages that have reached consensus.
void RaftNode::getChat() {
    qDebug() << "RaftNode::getChat";
   
    QString message = "";

    if (log.size() == 0) {
        message += "Log is empty.";
    } else {
        message += "Log: ";
        // Print each log entry.
        for (auto const& x : log) {
            message +=  "<br>" + QString::number(x.term) + ": " + x.command;
        }
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// Print all node ID's and the Raft state.
void RaftNode::getNodes() {
    qDebug() << "RaftNode::getNodes";

    // Get state of the current node. 
    QString message = "Raft state: ";
    switch (currentState) {
        case FOLLOWER:
            message += "Follower";
            break;
        case CANDIDATE:
            message += "Candidate";
            break;
        case LEADER:
            message += "Leader";
            break;
    }

    // Print the leader if it exists.
    if (!currentLeader.isEmpty()) {
        message += "<br>Leader: " + currentLeader;
    }

    // Print each known node it exists.
    if (knownNodes.size() != 0) {
        message += "<br>Nodes:";    
        for (auto const& x : knownNodes) {
            message +=  "<br>" + QString::number(x);
        }
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// Start the election timer. 
void RaftNode::startElectionTimer() {
    qDebug() << "RaftNode::startElectionTimer: Starting the election timer.";
    electionTimer->start(electionTimeout);
}

// Stop the election timer.
void RaftNode::stopElectionTimer() {
    qDebug() << "RaftNode::stopElectionTimer: Stopping the election timer.";
    electionTimer->stop();
}

// Restart the election timer.
void RaftNode::restartElectionTimer() {
    qDebug() << "RaftNode::restartElectionTimer: Restarting the election timer.";
    electionTimer->start();
}

// Handler for the election timeout.
void RaftNode::electionTimeoutHandler() {
    qDebug() << "RaftNode::electionTimeout: Handling the election timeout.";

    // Sanity check that timeouts are not running. 
    stopElectionTimer();
    stopHeartbeatTimer();

    switch (currentState) {
        case FOLLOWER:
            becomeCandidate();
            sendVotes();
            break;
        case CANDIDATE:
            qDebug() << "RaftNode::electionTimeout: Election timeout while state is Candidate.";
            sendVotes();
            return;
        case LEADER:
            qDebug() << "RaftNode::electionTimeout: Internal error - election timeout while state is Leader.";
            return;
    }
}

// Start the heartbeat timer. 
void RaftNode::startHeartbeatTimer() {
    qDebug() << "RaftNode::startHeartbeatTimer: Starting the heartbeat timer.";
    heartbeatTimer->start(heartbeatInterval);
}

// Stop the heartbeat timer.
void RaftNode::stopHeartbeatTimer() {
    qDebug() << "RaftNode::stopHeartbeatTimer: Stopping the heartbeat timer.";
    heartbeatTimer->stop();
}

// Restart the heartbeat timer.
void RaftNode::restartHeartbeatTimer() {
    qDebug() << "RaftNode::restartHeartbeatTimer: Restarting the heartbeat timer.";
    heartbeatTimer->start();
}

// @TODO - Handler for the heartbeat timeout.
void RaftNode::heartbeatTimeoutHandler() {
    qDebug() << "RaftNode::heartbeatTimeoutHandler: Handling the heartbeat timeout.";
}

// Start the printing timer. 
void RaftNode::startPrintTimer() {
    qDebug() << "RaftNode::startPrintTimer: Starting the printing timer.";
    QTimer *printTimer = new QTimer();
    connect(printTimer, SIGNAL(timeout()), this, SLOT(auxPrint()));
    printTimer->start(20000);
}

// Helper to print current state of the node. 
void RaftNode::auxPrint() {
    qDebug() << "RaftNode::auxPrint";
    qDebug() << "     currentState: " << currentState;
    qDebug() << "      currentTerm: " << currentTerm;
    qDebug() << "         votedFor: " << votedFor;
    qDebug() << "      commitIndex: " << commitIndex;
    qDebug() << "      lastApplied: " << lastApplied;
    qDebug() << "    currentLeader: " << currentLeader;
    qDebug() << "  protocolRunning: " << protocolRunning;
    qDebug() << "     droppedNodes: ";
    
    for (auto const& x : droppedNodes) {
        qDebug() <<  "                 " << x;
    }
    
    qDebug() << "       knownNodes: ";
    for (auto const& x : knownNodes) {
        qDebug() <<  "                 " << QString::number(x);
    }

    QMapIterator<QString, quint64> iterNext(nextIndex);
    qDebug() << "        nextIndex: ";
    while (iterNext.hasNext()) {
        iterNext.next();
        qDebug() <<  "                 " << iterNext.key() << ": " << iterNext.value();
    }

    QMapIterator<QString, quint64> iterMatch(matchIndex);
    qDebug() << "        matchIndex: ";
    while (iterMatch.hasNext()) {
        iterMatch.next();
        qDebug() <<  "                 " << iterMatch.key() << ": " << iterMatch.value();
    }

    qDebug() << "              log: ";
    for (auto const& x : log) {
        qDebug() << "                 " << x.term << ": " << x.command;
    }
    
    qDebug() << "    messagesQueue: ";
    for (auto const& x : messagesQueue) {
        qDebug() << "                 " << x;
    }
}

// Return node's ID.       
QString RaftNode::getID() {
    return nodeID;
}

// Set reference to the dialog window.
void RaftNode::setDialog(ChatDialog *dialog) {
    dialogWindow = dialog;
}


////////


// Construct the ChatDialog window. 
// Set the layout and register necessary callbacks.
ChatDialog::ChatDialog(NetSocket *netSock, RaftNode *raftNode) {
    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    textview = new QTextEdit(this);
    textview->setReadOnly(true);

    // Small text-entry box where the user can enter messages.
    // This widget normally expands only horizontally,
    // leaving extra vertical space for the textview widget.
    textline = new QLineEdit(this);

    // Lay out the widgets to appear in the main window.
    // For Qt widget and layout concepts see:
    // http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(textview);
    layout->addWidget(textline);
    setLayout(layout);

    // Set the layout title. 
    QString origin = raftNode->getID();
    setWindowTitle("Raft Chat Room - " + origin);
    qDebug() << "ChatDialog::ChatDialog: Origin is:" << origin;

    // Register a callback on the textline's returnPressed signal
    // so that we can send the message entered by the user.
    connect(textline, SIGNAL(returnPressed()), raftNode, SLOT(receiveCommand()));
    
    // Register callback on socket's readyRead signal to read a packet.
    connect(netSock, SIGNAL(readyRead()), raftNode, SLOT(receiveMessage()));
}

// Clear the textline to get ready for the next input message.
void ChatDialog::clearTextline() {
    textline->clear();
}

// Add the message to the chat window.
void ChatDialog::addMessage(QString message) {
    textview->append(message);
}

// Get textline from the input field.
QString ChatDialog::getTextline() {
    return textline->text();
}


////////


// Constructor for the NetSocket class.
// Define a range of UDP ports.
NetSocket::NetSocket() {
    // Pick a range of five UDP ports to try to allocate by default,
    // computed based on my Unix user ID. This makes it trivial for 
    // up to five P2Papp instances per user to find each other on the
    // same host, barring UDP port conflicts with other applications.
    // We use the range from 32768 to 49151 for this purpose.
    myPortMin = 32768 + (getuid() % 4096) * 4;
    myPortMax = myPortMin + 4;
    qDebug() << "NetSocket::NetSocket: Range of ports:" << myPortMin << "-" << myPortMax;
}

// Send a message to a socket.
qint64 NetSocket::writeDatagram(QByteArray *buf, int bufSize, quint16 port) {
    qint64 bytesSent = 0;
    bytesSent = QUdpSocket::writeDatagram(*buf, QHostAddress::LocalHost, port);
    if (bytesSent < 0 || bytesSent != bufSize) {
        qDebug() << "Error sending full datagram to" << QHostAddress::LocalHost << ":" << port;
    }
    return bytesSent;
}

// Bind the Netsocket to a port in a range of ports defined above.
bool NetSocket::bind() {
    // Try to bind to each of the ports in range myPortMin...myPortMax.
    for (quint16 port = myPortMin; port <= myPortMax; port++) {
        if (QUdpSocket::bind(port)) {
            qDebug() << "NetSocket::bind: Bound to UDP port:" << port;
            myPort = port;
            return true;
        }
    }

    // Failure binding.
    qDebug() << "NetSocket::bind: No ports in default range: " << myPortMin 
        << "-" << myPortMax << " available.";
    return false;
}

// Set list of neighboring ports.
void NetSocket::setPorts() {
    for (quint16 port = myPortMin; port <= myPortMax; port++) {
        if (port != myPort) {
          neighborPorts.append(port);
        }
    }
}

// Get list of neighboring ports.
QList<quint16> NetSocket::getPorts() {
    return neighborPorts;
}

// Return bound port.
quint16 NetSocket::getMyPort() {
    return myPort;
}


////////


int main(int argc, char **argv) {
    // Initialize Qt toolkit.
    QApplication app(argc,argv);

    // Create and bind a UDP network socket.
    NetSocket *sock = new NetSocket();
    if (!sock->bind())
        exit(1);

    // Set list of neighboring ports.
    sock->setPorts();

    // Create a new Raft node. 
    RaftNode *node = new RaftNode(sock);

    // Create an initial chat dialog window.
    ChatDialog *dialog = new ChatDialog(sock, node);
    
    // Set reference to the dialog window.
    node->setDialog(dialog);

    // Pop up the dialog window.
    dialog->show();

    // Enter the Qt main loop; everything else is event driven.
    return app.exec();
}


////////

