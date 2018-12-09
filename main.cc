#include "main.hh"


////////


// Construct the RaftNode and initialize to defaults.
RaftNode::RaftNode() {
    qDebug() << "RaftNode::RaftNode: Welcome from your new node!";
    current_state = FOLLOWER;
    currentTerm = 0;
    votedFor    = 0;
    commitIndex = 0;
    lastApplied = 0;
}


////////


// Construct the ChatDialog window. 
// Set the layout, the sequence number, and register necessary callbacks.
ChatDialog::ChatDialog(NetSocket *s) {
    // Read-only text box where we display messages from everyone.
    // This widget expands both horizontally and vertically.
    textview = new QTextEdit(this);
    textview->setReadOnly(true);

    // Small text-entry box the user can enter messages.
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

    // Set the socket.
    sock = s;

    // Set the unique identifier.
    qsrand((uint) QDateTime::currentMSecsSinceEpoch());
    myOrigin = QString::number(qrand());
    setWindowTitle("Raft Chat Room - " + myOrigin);
    qDebug() << "ChatDialog::ChatDialog: myOrigin is:" << myOrigin;

    // Register a callback on the textline's returnPressed signal
    // so that we can send the message entered by the user.
    connect(textline, SIGNAL(returnPressed()), this, SLOT(gotReturnPressed()));
    
    // Register callback on sockets' readyRead signal to read a packet.
    connect(s, SIGNAL(readyRead()), this, SLOT(gotMessage()));

}

// Callback when user presses "Enter" in the textline widget.
void ChatDialog::gotReturnPressed() {
  QString message = textline->text();

  // Add the message to the chat window. 
  QString messageText = "<span style=\"color:'red';\"><b>" + myOrigin + "</b></span>: " + message;
  textview->append(messageText);

  // Clear the textline to get ready for the next input message.
  textline->clear();
}

// Callback when receiving a message from the socket. 
void ChatDialog::gotMessage() {
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

// Find neighboring ports.
QList<quint16> NetSocket::findPorts() {
    for (quint16 port = myPortMin; port <= myPortMax; port++) {
        if (port != myPort) {
          // Get a list of ports.
          ports.append(port);
        }
    }
    return ports;
}


////////


int main(int argc, char **argv) {
    // Initialize Qt toolkit.
    QApplication app(argc,argv);

    // Create a UDP network socket, bind, and get list of ports.
    NetSocket *sock = new NetSocket();
    if (!sock->bind())
        exit(1);
    sock->findPorts();

    // Create a new raft node. 
    RaftNode *node = new RaftNode();

    // Create an initial chat dialog window.
    ChatDialog *dialog = new ChatDialog(sock);
    dialog->show();

    // Enter the Qt main loop; everything else is event driven.
    return app.exec();
}


////////

