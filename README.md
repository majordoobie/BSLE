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