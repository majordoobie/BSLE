import unittest
from src.client import cli_parser


class TestArgParser(unittest.TestCase):
    def setUp(self) -> None:
        self.args = {
            "host": "127.0.0.1",
            "port": "3388",
            "username": "Scooby",
            "src": None,
            "dst": None,
            "perm": None,
            "shell": False,
            "ls": False,
            "mkdir": False,
            "delete": False,
            "put": False,
            "l_ls": False,
            "l_mkdir": False,
            "create_user": False,
            "delete_user": False
        }

    def _test_valid(self):
        obj = cli_parser.ClientAction(**self.args)
        self.assertIsInstance(obj, cli_parser.ClientAction)

    def test_no_options(self):
        with self.assertRaises(ValueError):
            cli_parser.ClientAction(**self.args)

    def test_multi_actions(self):
        self.args["shell"] = True
        self.args["put"] = True

        with self.assertRaises(ValueError):
            cli_parser.ClientAction(**self.args)

    def test_zero_dependency_actions(self):
        self.args["shell"] = True
        self._test_valid()

        self.args["shell"] = False
        self.args["delete_user"] = True
        self._test_valid()

    def test_create_user(self):
        self.args["create_user"] = True

        # Error because missing --perm
        with self.assertRaises(ValueError):
            cli_parser.ClientAction(**self.args)

        self.args["perm"] = cli_parser.UserPerm["READ"]
        self._test_valid()




if __name__ == '__main__':
    unittest.main()
