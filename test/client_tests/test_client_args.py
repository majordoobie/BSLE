import unittest
import sys
from src.client import cli_parser
import argparse


class TestArgParser(unittest.TestCase):
    # def setUp(self) -> None:
    #     self.args = ["--server-ip", "127.0.0.1", "--server-port", "3388"]

    def test_no_options(self):
        self.assertEqual(1, 1)

        print("hi")
        cli_parser.get_args(["--server-ip", "127.0.0.1", "--server-port", "3388", "--put", "shit"])
        # with self.assertRaises(argparse.ArgumentError):
        #     cli_parser.get_args(self.args)


if __name__ == '__main__':
    unittest.main()
