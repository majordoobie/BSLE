# TL;DR
Just run the following and the script will handle everything for you
```bash
python3 builder.py --run-all
```

# How to compile
You compile the whole project with:
```bash
python3 builder.py --build
```

Or manually with:
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -S . -B build
cmake --build build -j $(nproc)
```
# Run all unit test
Use the provided python script to choose the tests you would like to run

```bash
# Run all tests
python3 builder.py -r

# Or manually with
ctest -j $(nproc) --test-dir build/ -R '^DBUserActions'
ctest -j 1 --test-dir build/ -E '^DBUserActions'
```

To run a specific test, first list the tests and use the name to run 
the test you are looking for
```bash
$ python3 builder.py -l 

gtest_server_args
gtest_server_db
gtest_server_crypto
gtest_server_file_api
```

The use the name to run the test
```bash
$ python3 builder.py -t gtest_server_crypto 

[==========] Running 7 tests from 2 test suites.
[----------] Global test environment set-up.
[----------] 1 test from TestMatchingFunc
[ RUN      ] TestMatchingFunc.TestMatch
[       OK ] TestMatchingFunc.TestMatch (3 ms)
[----------] 1 test from TestMatchingFunc (3 ms total)

[----------] 6 tests from HashTests/ServerCryptoHashTest
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/0
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/0 (0 ms)
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/1
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/1 (0 ms)
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/2
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/2 (0 ms)
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/3
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/3 (0 ms)
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/4
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/4 (0 ms)
[ RUN      ] HashTests/ServerCryptoHashTest.TestValidPorts/5
[       OK ] HashTests/ServerCryptoHashTest.TestValidPorts/5 (0 ms)
[----------] 6 tests from HashTests/ServerCryptoHashTest (0 ms total)

[----------] Global test environment tear-down
[==========] 7 tests from 2 test suites ran. (3 ms total)
[  PASSED  ] 7 tests.
```

# Headers
## Legend

| Symbol      | Description                   |
|-------------|-------------------------------|
| <- LABEL -> | The label spans multiple rows |
| \*\*LABEL** | Field is of variable length   |
| \~LABEL\~   | Next header segment           |

## Client Request Header
> Note that all `FILE_DATA_STREAM` are prepended with a 32 byte hash of 
> the file. The file length then becomes `PAYLOAD_LEN - 32`
> 
> The `FILE_DATA_STREAM` is also limited to 1016 bytes per payload so once
> the section is hit, you must start to account for the MTU size of 1016

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
## Client Request Std Payload
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
##  Client Request User Payload
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
## Server Response
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
