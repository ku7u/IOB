UDP is used for messaging both to and from the app and between units when in MU configuration. Multicast mode messages are used for rollcall.  Commands may be sent as unicast. 
A standard format is used for all messaging. Data is encapsulated in a JSON formatted string. The format is as follows:
key: 'node'; data: the sending nodename. Primarily for troubleshooting and optional.
key: 'topic'; data: as follows
'cmd/[destination (optional)]/[command name]'
'tlm/[source]/[telemetry name]'
key: 'data'; data: a value or a JSON object containing multiple key/value pairs

The app begins its session by sending a mDNS request to the network. All powered nodes respond to this asynchronously. The app builds a list of the available nodes which can be displayed to the operator at any time. The app maintains this list by periodically sending the mDNS message. In this way it can gather new nodes and/or delete the ones that are no longer active.

Each node's response to the mDNS request includes the loco ID, such as 'GN416', its type, such as 'EMD SD9', and its IP address. The latter is used for further communication from the app to the specific node.

After a loco is chosen in the app UDP unicast messages are used bidirectionally between app and loco. 

The MU scheme
Any single loco will compute its own acceleration and subsequent speed. When a loco is mued to a lead loco then that trailing loco will no longer be resposible for calculating its own speed. Instead it will use the telemetry of the lead loco, transmitted once each second, to set its own speed. All trailing locos will use this technique simultaneously. Since all locos are speed calculated, then when any loco is directed to run at speed x, it will attain that speed immediately. 

The MU data flow. 
The app will present the operator with a list of candidate locos to attach to the already selected lead. (TBA filtering) 
-When a loco is selected from the MU fragment dropdown list a message (muSetState) will be transmitted from the app to the candidate loco. This is done with a udp unicast message since we know the ip address of the candidate.
-The candidate loco will then set its muState variable to 2 or 3 depending on whether it is a mid or trailing loco in the consist. It will send a message (muLocoData) to the lead. This message contains its ID, ip address, mass, hp, te (tractive effort) and mustate. 
-The lead loco runs the routine muSetPerformance on receiving the muLocoData message. 
-  If the lead is already running this will result in the lead sending a message to the trailing unit to turn on its prime mover
-  The lead builds a json document (muDoc) containing all trailing locos and their performance data and ip addresses
-  The lead loco uses the performance data in muDoc to recalculate acceleration going forward.
-  It will include the mued locomotive(s) as an addition to subsequent status messages it transmits of the form "consist":"locoID1", "locoID2", etc. (TBD necessity)