# File Transfer App
## Table of Content
1. [Summary](#5)
1. [How To Compile](#1)
2. [How To Run Server](#2)
3. [How To Run Client](#3)
4. [Headers](#4)


## Summary <a name="5"</a/>
The server listens on the specified port for a connection. As soon as a connection is made to 
the server, the server will enqueue the connection received into the thread pools job queue.
Once the job is dequeued, an available thread will handle the connection with the client. The 
headers used for the server-client communications is displayed in the header section of this guide.  

On initial connection from the client, the client will set its session ID to 0, indicating that 
this is an initial connection from the client. The server will then authenticate the client and 
generate a session ID. The session ID is saved into a hash table containing the "session_id: time_of_connection". 
The session ID is then continuously used by the server and client unless the client closes the socket, or 
the client does not send any packets within the timeout limit set in the command line of the server start 
up. When the session expires, an OP_SESSION_EXPIRE code is sent to the client forcing the client to re-authenticate. 
Since I have not implemented any session hashes, like a JWT token, even if the session is valid, the client is 
still authenticated using the hashed password in the client request. This should mitigate some replay attack but, 
of course it is nothing like using a JWT token and encrypting the traffic. 

All requests and replies that contain a payload (CMDS: get, put, ls) will have the data's hash prefixed in 
the byte stream of the data being sent. The hash is generated from its source, for example, on a ls command, 
the server will generate the byte stream that represents the ls output. The byte streamed is then hashed using 
sha256 and prefixed in the byte stream in the format of <hash><byte_stream>. The client will then extract the 
byte stream, hash it, and compare the hash with what it received from the server.  

All user information is saved in the servers database (text file) in the server's home folder under `.cape`. 
The file is prefixed with a magic byte that is validated on read to ensure that the file read is the correct 
file. Additionally, the database is hashed and saved into a hash file “.cape/.cape.hash”. When the server boots 
up, the server will read the database “.cape/.cape.db”, hash it, and compare it with the hash in the hash file. 
If they do not match, the server will not boot up. All user passwords are stored as hashes, not plain text. 

## How To Compile <a name="1"></a>
The builder script `builder.py` can be used to build the project and even 
run the unit test for you.

To compile and run all unit tests
```bash
python3 builder.py --run-all
```
Or to build/rebuild the project
```bash
# Via script
python3 builder.py --build

# Manually
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
cmake --build build -j $(nproc)
```

## How To Run Server <a name="2"></a>
The binary is placed in the `${CWD}/bin`
```bash
➜ ./bin/server -h
Start up a file transfer server and server up the files located in the home directory which 
is specified by the -d argument. All operations must first be authenticated. 
Once authenticated a session ID is assigned to the connection until the connection 
terminates or the session times out. After which the user must re-authenticate.

options:
        -t      Session timeout in seconds (default: 10s)
        -p      Port number to listen on (default: 31337)
        -d      Home directory of the server. Path must have read and write permissions.


➜ ./bin/server -t 60 -d test/server
```

## How To Run Client <a name="3"></a>
The client script is stored in `${CWD}/src/client/client_main.py` 

The client has an indepth help menu that you can invoke with `${CWD}/src/client/client_main.py -h`.
Additionally, the robust input validation will let you know when you are missing arguments.

### Some Examples
```bash
# Drop into shell mode
python3 src/client/client_main.py -U "admin" --shell

# Create a new user
python3 src/client/client_main.py -U "admin" --create_user "new_user" --perm READ_WRITE

# List a directory
python3 src/client/client_main.py -U "admin" --ls --dst "/"
```


## Headers <a name="4"></a>
The headers below follow the wire protocol with additional portions
added at the end as a "payload"

### Legend

| Symbol      | Description                   |
|-------------|-------------------------------|
| <- LABEL -> | The label spans multiple rows |
| \*\*LABEL** | Field is of variable length   |
| \~LABEL\~   | Next header segment           |

### Client Request Header
> Note that all `FILE_DATA_STREAM` are prepended with a 32 byte hash of 
> the file. The file length then becomes `PAYLOAD_LEN - 32`. The hash 
> is used to validate that the data stream has not been modified through
> transfer. 

```
   0               1               2               3   
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |     OPCODE    |   USER_FLAG   |           RESERVED            |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |        USERNAME_LEN           |        PASSWORD_LEN           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          SESSION_ID                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    **USERNAME + PASSWORD**                    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                          PAYLOAD_LEN ->                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       <- PAYLOAD_LEN                          |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                ~user_payload || std_payload~                  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
#### Client Request: Std Payload
To indicate that there is a file data stream (Only occurs during REMOTE PUT command)
`(PAYLOAD_LEN - PATH_LEN) > 0`
```
   0               1               2               3   
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |          PATH_LEN             |         **PATH_NAME**         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                     **FILE_DATA_STREAM**                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
####  Client Request: User Payload
To indicate that there is a password field (Only occurs during user creation)
`(PAYLOAD_LEN - (USR_ACT_FLAG + PERMISSION + USERNAME_LEN)) > 0`
```
   0               1               2               3   
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |  USR_ACT_FLAG |   PERMISSION  |          USERNAME_LEN         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | **USERNAME**  |         PASSWORD_LEN          | **PASSWORD**  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
### Server Response
Every response will have a `MSG` describing the response even if it 
was a successful interaction.
```
   0               1               2               3   
   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | RETURN_CODE   |    RESERVED   |          SESSION_ID->         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       <- SESSION_ID           |         PAYLOAD_LEN ->        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                      <- PAYLOAD_LEN ->                        |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       <- PAYLOAD_LEN          |    MSG_LEN     |   **MSG**    |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    **FILE_DATA_STREAM**                       |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```