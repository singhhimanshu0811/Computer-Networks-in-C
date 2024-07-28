Sending `<username>@<ipaddress>` with every message requires transmitting several bytes to the server multiple times. 
Rather than sending the whole name, an equivalent mapping of the username can be maintained by the server. 
You have taken the assumption that there will be 1024 clients at max. This can be represented with 10 bits of data, i.e., with values from 0x0000 to 0x03FF.

In this lab, when a client joins, the server will assign the client with the smallest available address. This is the user's **`<id>`** 
For example, if alice joins first, she will get the address 0x0000, next if bob joins, he will get 0x0001, next if eve joins, she will get 0x0002. next if bob leaves and oscar joins right after that, oscar will get the address of 0x0001. MSBs are always the first 8 bits, followed by 8 bit LSBs

To start off: 
- The client will take in the server's IP address, port through CLI arguments in the format: `./a.out <ip_address> <port>` and connect to the server. Here the ip_address and port numbers are that of the server. 

-The server will take in IP address, port and password through CLI arguments in the format: `./a.out <ip_address> <port> <password>`. 


- A `<username>@<ip_address>|<authority>\n` will be sent to the server as the first message after the server accepts the connection (Similar to what you had done in the take home). Note carefully that the client  includes `\n` along with the `<username>@<ip_address>|<authority>\n` sent!
  * Assume that this is ALWAYS the first communication that any client would compulsorily do to make your life easier :)
  * The `<username>` will be a single word without spaces or special characters.
  * The `<ip_address>` is the machine's ip address. Ensure you pick it and put it exactly in ip_address format (e.g., 10.1.19.200) 
  * The `<authority>` specifies whether it's a root user identified by sending "r" or a normal user identified by sending "n".
  * Example of details sent by alice can be : `alice@10.1.19.200|r\n`. 

- If a root user connects, the server will expect the very next message to be the password.
    * The client will send the password in the format: `<password>\n`.
    * E.g., [cli] user123\n

- The server that you develop should be capable of handling both root and normal clients. 
  * The `<password>` is used by root users to login.
  * E.g., [cli] ./a.out 127.0.0.1 4444 user123\n
  * If a root user (client with ‘r’ as authorization) connects, the client must provide the `<password>` in the next message (see client.c for the details).
  * If the password validates, the user is connected. Send `0x02 0x00 <id>` back to the root client.E.g., if the client gets an id of 0x0003, then the server will respond with 0x02 0x00 0x00 0x03. 
  * If the password is incorrect send `0x05 0x00` and disconnect the client.
  * Note that 0x02 0x00 and 0x05 0x00 are byte values, not strings

 - When a normal client joins, the server does not ask for password, but will rather send back `0x02 0x00 <id>`, similar to what it sent back for the root user.
   
 - In the in-lab, we will change the client's reference from `<username>@<ip_address>` to `<id>`. This will help reduce the number of bytes to be transmitted. 


 - When the server receives data from normal user, or password authenticated root user information, it will manage two data structures. 
 	* The first data structure will hold the `<username>@<ip_address>` to `<id>` mapping.
 	* The second data structure will manage the `<id>` to `<authority>` mapping.



- The client will take input through stdin and send it to the server.

- The client will receive data from the server and print it to stdout.


- Similar to the take-home, the server will maintain various types of files. 
  * There will be files for individual message exchange, which will be identified as `01_<id1>-<id_2>.txt` (Note that there should be only one file for two client communication. Thus if `01_<id1>-<id2>.txt` is created, a new file `01_<id2>-<id1>.txt` should never get created).
  * There are files for group communication that will be named as `02_<groupname>.txt`
  * There are files to handle broadcasts that will be named `03_bcst.txt`.
  * Data in all these files will be stored in the following format:
    1.  `<id1>:<message>\n<id2>:<message>`.
    2.  individual files will only contain data exchanged by the two clients using MSGC (message type 0x06 0x01)
    3.  group communication file will only store MCST (message type 0x06 0x02) messages for that group
    4.  broadcast file will only contain messages sent using BCST (message type 0x06 0x03)


 - In this lab, we will replace all the 4 byte commands with 2 byte message types. We next list down the command to message type mapping. Please note that you will still take user input in the same format, i.e., you will expect the user will type LIST\n, but you will replace the string with the command code as identified below: 
   * LIST\n is 0x06 0x04 
   * MSGC\n is 0x06 0x01
   * GRPS\n is 0x06 0x05
   * MCST\n is 0x06 0x02
   * BCST\n is 0x06 0x03
   * HISF\n is 0x06 0x06
   * EXIT\n is 0x06 0x07

###Command-wise details:###

- 0x06 0x04: When server receives this command, it will respond back with 0x06 0x04 `-` `<name1>@<ip_addr1>|<id1>|<authority1>:<name2>@<ip_addr2>|<id2>|<authority2>:<name3>@<ip_addr3>|<id3>|<authority3>\n`. The server will also send this command out when new clients join or existing clients leave.

- 0x06 0x01: A client will send 0x06 0x04 0x00 0x03 <message> when it wants to send a message to another client identified by 0x0003. Thus it uses 4 bytes to tell the server that this is a MSGC, and which client to send the message to. When server receives this, it will replace the receiving clients id with sending client id.     User input: please take user input as `MCGC:<id>:<message>`, and replace the strings with appropriate bytes.

- 0x06 0x05: This will be used as `0x06 0x05 <id1> <id2>	...<idn>:<groupname>\n`. Please note that these are byte values with no space in between. `<id>` is always of 2 bytes. Please take `GRPS:<id1>...<idn>:<groupname>\n` as user input

- 0x06 0x02: There is no change in this, except that MCST: is replaced with 0x06 0x02 while sending. Take MCST as user input

- 0x06 0x03: There is no change in this, except that BCST: is replaced with 0x06 0x03 while sending

- 0x06 0x06: There is no change in this, except that HISF: is replaced with 0x06 0x06 while sending

- 0x06 0x07: There is no change in this. Take EXIT\n as user input.


- Please test your code using 4 clients, alice -- root user, bob -- normal user, eve -- root user, and oscar -- root user. Let eve join and leave, and then make oscar join after eve leaves.

- Please follow the same format as the take-home to test your code.

- Any details not mentioned in the question can be taken as an assumption 
