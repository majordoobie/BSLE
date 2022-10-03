# File Transfer App
## Table of Content
1. [How To Compile](#1)
2. [How To Run Server](#2)
3. [How To Run Client](#3)
4. [Headers](#4)



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