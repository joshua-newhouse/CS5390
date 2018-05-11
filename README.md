# CS5390
Network project
This project simulates network nodes which forward data to one another.  Shared files are used as the data links between nodes.  Each node is an instance of the program.  It is instantiated by running the program name at the command line and feeding the parameters to it via command line arguments.

./node 3 100 5 "This is the message from node 3 destined to node 5." 1 4 7

The above line is an instantiation of a single node with address 3, that will run no longer than 100 seconds, whose message destination is 5, followed by the message, and finally a list of all neighboring nodes (1, 4, 7).

The script.sh file will perform a test run with 10 nodes.  10 is the maximal number of nodes supportable by the code and more than 100 packets per node at the network layer will result in parts of the message being lost.

This project implements a network protocol stack starting with the Transport layer, then network layer, and finally the datalink layer.

The datalink layer uses two files for each link as separate channels, one file is outbount the other is inbound.  The datalink layer frames packets from the network layer and uses byte stuffing.  It handles reliable delivery through acknowlegements and retransmission.

The network layer handles routing between nodes in a simplistic fashion by broadcasting messages new to the node over all links (files) unless it is destined for this node in which case it hands the packet to the transport layer.

The transport layer breaks an originating string into messages and passes them to the network layer.  The transport layer is responsible for reliable delivery and performs individual acknowledgements and retransmission of unacknowledged messages.  It also receives inbound messages from the network layer and puts them in order based on source.
