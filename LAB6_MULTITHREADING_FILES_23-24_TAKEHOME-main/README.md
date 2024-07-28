# LAB6_MULTITHREADING_FILES_23-24_TAKEHOME
## Important
- If you are facing issues such as inconsistent evaluation, and are using WSL, please try using a VM, native Linux or the CC lab machines.
## **Instructions**

- This lab will be evaluated automatically.

- You may write your own test cases. No scripts or test cases will be provided.

- You will build up on the previous lab's materials.

- Any strings which are required to be sent by the client or server must be exactly as specified. (This includes the number of bytes that are being sent)

- Submit a zip file with the name `<id_no>_lab6.zip` containing your  implementation of client and server. It should be a zip of this repo. Ensure the impl folder is present and contains server.c and client.c.
  


- All 
Example Submission of necessary files, (you may have extra folders/files, it wont change anything):
```
id_no>_lab6.zip
└── impl
    └── server.c
    └── client.c

```


<br>

The purpose of this lab is to implement a client and server program that
uses multi-threading to communicate and can share files.

## **Take Home**

For this lab, implement both client and server code, they will both be utilizing **multithreading** to send and receive messages in parallel. 
We will continue using the two types of clients – root clients and normal clients that we introduced in the previous lab.

### **client.c**

- The client will take in the server's IP address, port through CLI arguments in the format: `./a.out <ip_address> <port>` and connect to the server. Here the ip_address and port numbers are that of the server. 

- A `<username>@<ip_address>|<authority>\n` will be sent to the server as the first message after the server accepts the connection (Similar to what you had done in the previous lab). Note carefully that the client  includes `\n` along with the `<username>@<ip_address>|<authority>\n` sent!
  * Assume that this is ALWAYS the first communication that any client would compulsorily do to make your life easier :)
  * The `<username>` will be a single word without spaces or special characters.
  * The `<ip_address>` is the machine's ip address. Ensure you pick it and put it exactly in ip_address format (e.g., 10.1.19.200) (0.5 mark)
  * The `<authority>` specifies whether it's a root user identified by sending "r" or a normal user identified by sending "n".
  * Example of details sent by alice can be : `alice@10.1.19.200|r\n`. 

- If a root user connects, the server will expect the very next message to be the password.
    * The client will send the password in the format: `<password>\n`.
    * E.g., [cli] user123\n

- The client will be referred to by this `<username>@<ip_address>` in any communication.

- The client will take input through stdin and send it to the server.

- The client will receive data from the server and print it to stdout.

- **Note:** that taking input from stdin and receiving data are both
  blocking calls. Ensure these 2 can run in parallel using
  multithreading.

### **server.c**


- The server that you develop should be capable of handling both root and normal clients. To start off,  the server will take in IP address, port and password through CLI arguments in the format: `./a.out <ip_address> <port> <password>`.
* The `<password>` is used by root users to login (see client.c for details).
* E.g., [cli] ./a.out 127.0.0.1 4444 user123\n
* If a root user (client with ‘r’ as authorization) connects, the client must provide the `<password>` in the next message (see client.c for the details).
* If the password validates, the user is connected. Send "PASSWORD ACCEPTED\n" back to the root client.
* If the password is incorrect send "EROR:PASSWORD REJECTED\n" and disconnect the client.

- The server will listen to a max of 1024 connections, although it can accept any amount of
  client connections. (Requirement for server evaluation)

- The server must be able to receive data from all clients in parallel
  using multithreading. **Prerequisite for further evaluation**

- The client will send messages that start with a 4 letter command. The server's behaviour will depend on this command. In case of errors, the server will send the error message using the keyword `EROR`.

- Based on the data received from the client, the server will perform
  different functions.

- In addition to the above details, the server will maintain various types of files. (2 marks)
  * There will be files for individual message exchange, which will be identified as `01_<username1>@<ip_address>-<username2>@<ip_address>.txt` (Note that there should be only one file for two client communication. Thus if `01_<username1>@<ip_address>-<username2>@<ip_address>.txt` is created, a new file `01_<username2>@<ip_address>-<username1>@<ip_address>.txt` should never get created).
  * There are files for group communication that will be named as `02_<groupname>.txt`
  * There are files to handle broadcasts that will be named `03_bcst.txt`.
  * Data in all these files will be stored in the following format:
    1.  `<username1>@<ip_address>:<message>\n<username2>@<ip_address>:<message>`.
    2.  individual files will only contain data exchanged by the two clients using MSGC
    3.  group communication file will only store MCST messages for that group
    4.  broadcast file will only contain messages sent using BCST


### **Commands available to client**
1. #### **LIST\n**
  * When the server receives this command it sends a string containing the names of all the clients that are currently connected to in a new format: `LIST-<name1>@<ip_addr1>|<authority1>:<name2>@<ip_addr2>|<authority2>:<name3>@<ip_addr3>|<authority3>\n` 
  * The server will also broadcast this list to all exiting connected clients when any **new client joins, or when an existing client exits**. (1 mark)

2. #### **`MSGC:<receiver_username>@<ip_address>:<message>\n`**
   [Same as what was done in the previous lab's take home]

3. #### **`GRPS:<user1>@<ip_address>,<user2>@<ip_address>,...,<usern>@<ip_address>:<groupname>\n`**
   [Same as what was done in the previous lab's take home]

4. #### **`MCST:<groupname>:<message>\n`**
   [Same as what was done in the previous lab's take home]

5. #### **`BCST:<message>\n`**
   [Same as what was done in the previous lab's take home]

6. #### **`HISF:<options>\n`**
   * This command can only be issued by root users. In case a normal user issues this command, the server responds with 'EROR:UNAUTHORIZED\n' (1 mark -- note that its is EROR, not ERROR)
   * When a server receives the HISF command from a root user, it will return back a specific type of file.
   * The type of file is identified based on options. Options are separated by `|`.
   * -t option indicates the type of file: individual(01), group communication(02), or broadcast(03).
   * When -t is 01 or 02, then -n option is also expected. -n will be the `<username>` in case of individual, and `<groupname>` in case of group communication.
   * Example commands are `HISF:-t 01|-n alice@10.0.0.10\n` or `HISF:-t 02|-n GoodOnes\n` or `HISF:-t 03\n`.
     1. In the first case, if bob issued this command, then a file with all communication between bob and alice will be returned to bob. In case no communication between alice and bob has occured, a blank file with the appropriate name will be created on the server and returned back to bob. (1 mark)
     2. In the second case, if bob issued this command and bob is part of GoodOnes, then the file 02_GoodOnes.txt will be returned. If there has been no MCST for GoodOnes, then a blank file with the appropriate name will be created on the server and returned back to bob (0.5 marks). If bob is not part of GoodOnes, then 'EROR:UNAUTHORIZED\n' will be returned (this is true even if bob was a root user).
     3. In the third case, all 03_bcst.txt file will be returned back to bob.
    
7.  #### **`EXIT\n`**
   - The server will stop listening for that client.

- **Note:** Ensure if a client sends `EXIT\n` it will no longer show in the
  `LIST\n` of clients.

If the server receives a message which is not a part of any of these
commands, send `EROR:INVALID COMMAND\n` back to the sender.


**Note:** ensure that all filenames follow the correct naming protocol.

What to show:
- Start 3 clients with usernames alice, bob and eve. 
- alice and eve are roots, bob is normal user
- eve should supply wrong password and get disconnected. eve again joins with corrct password. In all these scenarios every client would have receive the updated list. (Note: eve should not be a part of the list before she supplies the correct password)
- All three clients then send LIST to server (user input). The server should display all connected users along with their ip address.
- alice sends one message to bob using MSGC, bob replies back to that message using MSGC.
- alice creates a group GoodOnes which has alice and bob.
- bob sends HISF to get all 01 type conversation with alice. bob should get error message because bob is not the root user.
- alice sends HISF to get all conversation with bob and should get the file.
- alice sends a HISF to get all coversations in `02_GoodOnes.txt`. alice gets a blank file.
- alice sends a BCST, as well as a MCST to `GoodOnes`.
- eve asks for the BCST file and should get the file. eve asks for the `02_GoodOnes.txt` and gets error message.
- alice exits the chat using the EXIT keyword. all users are notified.
