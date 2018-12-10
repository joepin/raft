#include "main.hh"


////////


// Construct the RaftNode and initialize to defaults.
RaftNode::RaftNode(NetSocket *netSock) {
    qDebug() << "RaftNode::RaftNode: Welcome from your new node!";

    // Set the port configs.
    sock   = netSock;
    nodeID = QString::number(sock->getMyPort());
    neighborPorts = sock->getPorts();

    // Set default values.    
    currentTerm = 0;
    commitIndex = 0;
    lastApplied = 0;
    votedFor    = "";
    currentLeader   = "";
    protocolRunning = false;

    // Set timeouts.
    electionTimeout  = rand() % (ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN + 1) + ELECTION_TIMEOUT_MIN;
    heartbeatInterval = HEARTBEAT_INTERVAL;

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

    // Handle message type
    QString command = message.left(message.indexOf(" "));
    qDebug() << "RaftNode::receiveCommand: Handling command: " << command;

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
        sendMessage(message);
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
void RaftNode::sendMessage(QString message) {
    qDebug() << "RaftNode::sendMessage";

    // // Add the message to the chat window. 
    // QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    // dialogWindow->addMessage(messageText);
}

// @TODO - Callback when receiving a message from the socket. 
void RaftNode::receiveMessage() {
    qDebug() << "RaftNode::receiveMessage";

    NetSocket *sock = this->sock;

    // Read each datagram.
    while (sock->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(sock->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        QVariantMap message;

        sock->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        QDataStream stream(&datagram, QIODevice::ReadOnly);

        stream >> message;

        datagram.clear();
    }
}

// @TODO - Transition to candidate and begin process of gathering votes.
void RaftNode::requestVoteRPC() {
    qDebug() << "RaftNode::requestVoteRPC";
}

// @TODO - Replicate log entries.
void RaftNode::appendEntriesRPC() {
    qDebug() << "RaftNode::appendEntriesRPC";
}

// @TODO - Print chat history. Only show messages that have reached consensus.
void RaftNode::getChat() {
    qDebug() << "RaftNode::getChat";
   
    QString message = "";

    if (log.size() == 0) {
        message += "Log is empty.";
    } else {
        message += "<br>Log: ";
        // Print each log entry.
        for (auto const& x : log) {
            message +=  "<br>" + QString::number(x.term) + ": " + x.command;
        }
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// @TODO - Stop participating in Raft protocol.
void RaftNode::stopProtocol() {
    qDebug() << "RaftNode::stopProtocol";
    protocolRunning = false;
}

// @TODO - Start participating in Raft protocol.
void RaftNode::startProtocol() {
    qDebug() << "RaftNode::startProtocol";

    protocolRunning = true;

    // On startup, a node will send a message to a random port. 
    // If the port is the leader, the port node will send back the leader's port. 
    // If the port is not the leader, but the port node knows of the leader, the port node will send back the leader's port. 
    // If the port is not the leader, and the port node does not know of the leader, the port node will send an empty response. 
    // The initiator will time out and choose a new port.  

    // Begin printing for debugging.
    startPrintTimer();

    // Begin the election timeout. 
    startElectionTimer();
    
    // QByteArray buf;
    // QDataStream datastream(&buf, QIODevice::ReadWrite);
    // QVariantMap message;

    // // Serialize the message.
    // message["term"] = "";
    // message["leaderId"] = "";
    // message["prevLogIndex"] = "";
    // message["prevLogTerm"] = "";
    // message["entries"] = "";
    // message["leaderCommit"] = "";

    // datastream << message;

    // // Send message to the socket.
    // sock->writeDatagram(&buf, buf.size(), port);
}

// @TODO - Drop packets from targetNode.
void RaftNode::dropComms(QString targetNode) {
    qDebug() << "RaftNode::dropComms";
}

// @TODO - Restore communications (no longer drop packets) from targetNode.
void RaftNode::restoreComms(QString targetNode) {
    qDebug() << "RaftNode::restoreComms";
}

// Print all node ID's and the Raft state.
void RaftNode::getNodes() {
    qDebug() << "RaftNode::getNodes";

    // Get state of the current node. 
    QString message = "<br>Raft state: ";
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

    // Print each known node if they exist.
    if (knownNodes.size() != 0) {
        message += "<br>Nodes:";    
        for (auto const& x : knownNodes) {
            message +=  "<br>" + x;
        }
    }

    // Add the message to the chat window. 
    QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
    dialogWindow->addMessage(messageText);
}

// @TODO - Handler for the election timeout.
void RaftNode::electionTimeoutHandler() {
    qDebug() << "RaftNode::electionTimeout: Handling the election timeout.";

    // Stop the timer.
    electionTimer->stop();
}

// Start the election timer. 
void RaftNode::startElectionTimer() {
    qDebug() << "RaftNode::startElectionTimer: Starting the election timer.";
    electionTimer = new QTimer();
    connect(electionTimer, SIGNAL(timeout()), this, SLOT(electionTimeoutHandler()));
    electionTimer->start(electionTimeout);
}

// Restart the election timer.
void RaftNode::restartElectionTimer() {
    qDebug() << "RaftNode::restartElectionTimer: Restarting the election timer.";
    electionTimer->start();
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
        qDebug() <<  "       " << x;
    }
    
    qDebug() << "       knownNodes: ";
    for (auto const& x : knownNodes) {
        qDebug() <<  "       " << x;
    }

    qDebug() << "        nextIndex: ";
    for (auto const& x : nextIndex) {
        qDebug() <<  "       " << x.first << ':' << x.second;
    }

    qDebug() << "       matchIndex: ";
    for (auto const& x : matchIndex) {
        qDebug() <<  "       " << x.first << ':' << x.second;
    }

    qDebug() << "              log: ";
    for (auto const& x : log) {
        qDebug() << "       " << x.term << ": " << x.command;
    }
    
    qDebug() << "    messagesQueue: ";
    for (auto const& x : messagesQueue) {
        qDebug() << "       " << x;
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
    // Pick a range of six UDP ports to try to allocate by default,
    // computed based on my Unix user ID. This makes it trivial for 
    // up to six P2Papp instances per user to find each other on the
    // same host, barring UDP port conflicts with other applications.
    // We use the range from 32768 to 49151 for this purpose.
    myPortMin = 32768 + (getuid() % 4096) * 5;
    myPortMax = myPortMin + 5;
    qDebug() << "NetSocket::NetSocket: Range of ports:" << myPortMin << "-" << myPortMax;
}

// Send a message to a socket.
qint64 NetSocket::writeDatagram(QByteArray *buf, int bufSize, quint16 port) {
    qint64 bytesSent = 0;
    bytesSent = QUdpSocket::writeDatagram(*buf, QHostAddress::LocalHost, port);
    if (bytesSent < 0 || bytesSent != bufSize) {
        qDebug() << "Error sending full datagram to" << QHostAddress::LocalHost << ":" << port << ".";
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
        << "-" << myPortMax << " available";
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

