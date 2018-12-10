#include "main.hh"


////////


// Construct the RaftNode and initialize to defaults.
RaftNode::RaftNode(NetSocket *netSock) {
    qDebug() << "RaftNode::RaftNode: Welcome from your new node!";

    // Set the port configs.
    sock   = netSock;
    nodeID = QString::number(sock->getMyPort());
    neighborPorts = sock->getPorts();
    
    currentTerm = 0;
    votedFor    = 0;
    commitIndex = 0;
    lastApplied = 0;
    
    // Set timeouts.
    electionTimeout  = rand() % (ELECTION_TIMEOUT_MAX - ELECTION_TIMEOUT_MIN + 1) + ELECTION_TIMEOUT_MIN;
    heartbeatTimeout = HEARTBEAT_TIMEOUT;

    current_state = FOLLOWER;
}

// Callback when receiving a message from the socket. 
void RaftNode::receiveMessage() {
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

// Print usage to console.
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
    qDebug() << "RaftNode::sendMessage: Handling command: " << command;

    if (command == "START") {
        startProtocol();
        return;
    }
    else if (command == "GET_CHAT") {
        getChat();
        return;
    }
    else if (command == "STOP") {
        stopProtocol();
        return;
    }
    else if (command == "GET_NODES") {
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
        sendMessage(message);
    }
    else if (command == "DROP") {
        dropComms(message);
    }
    else if (command == "RESTORE") {
        restoreComms(message);
    }
    else {
        usage();
        return;
    }
}

// Send message to the chatroom.
void RaftNode::sendMessage(QString message) {
    qDebug() << "RaftNode::sendMessage";
}

 // Print chat history.
void RaftNode::getChat() {
    qDebug() << "RaftNode::getChat";

}

// Stop participating in Raft protocol.
void RaftNode::stopProtocol() {
    qDebug() << "RaftNode::stopProtocol";
}

// Start participating in Raft protocol.
void RaftNode::startProtocol() {
    qDebug() << "RaftNode::startProtocol";
}

// Drop packets from targetNode.
void RaftNode::dropComms(QString targetNode) {
    qDebug() << "RaftNode::dropComms";
}

// Restore communications (no longer drop packets) from targetNode.
void RaftNode::restoreComms(QString targetNode) {
    qDebug() << "RaftNode::restoreComms";
}

// Print all node ID's and Raft state.
void RaftNode::getNodes() {
    qDebug() << "RaftNode::getNodes";
}

// // Add the message to the chat window. 
// QString messageText = "<span style=\"color:'red';\"><b>" + nodeID + "</b></span>: " + message;
// dialogWindow->addMessage(messageText);

// // Clear the textline to get ready for the next input message.
// dialogWindow->clearTextline();

// QByteArray datagram;
// QDataStream datastream(&datagram, QIODevice::ReadWrite);

// datastream << message;

// // Send message to the socket. 
// sock->writeDatagram(&datagram, datagram.size(), neighborPorts[0]);

// void ChatDialog::sendRumorMessage(QString origin, qint32 seq, QString text, quint16 port) {
//   QByteArray buf;
//   QDataStream datastream(&buf, QIODevice::ReadWrite);
//   QVariantMap message;

//   // Serialize the message.
//   message["ChatText"] = text;
//   message["Origin"] = origin;
//   message["SeqNo"] = seq;

//   qDebug() << "Sending \"rumor\" message to port:" << port
//     << ", <\"ChatText\"," << message["ChatText"].toString()
//     << "><\"Origin\"," << message["Origin"].toString()
//     << "><\"SeqNo\"," << message["SeqNo"].toString() << ">";

//   datastream << message;

//   // Send message to the socket.
//   sock->writeDatagram(&buf, buf.size(), port);
// }

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

