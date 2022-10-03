import unittest
from pathlib import Path

from src.client import client_classes


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
        """Function is used to verify that the modified args are valid. This
        is called from the individual test cases that SHOULD work"""
        obj = client_classes.ClientRequest(**self.args)
        self.assertIsInstance(obj, client_classes.ClientRequest)

    def test_no_options(self):
        """Test failure when no action is provided"""
        with self.assertRaises(ValueError):
            client_classes.ClientRequest(**self.args)

    def test_multi_actions(self):
        """Test failure when multiple actions are provided"""
        self.args["shell"] = True
        self.args["put"] = True

        with self.assertRaises(ValueError):
            client_classes.ClientRequest(**self.args)

    def test_zero_dependency_actions(self):
        """Test functions that require no dependency"""
        self.args["shell"] = True
        self._test_valid()

        self.args["shell"] = False
        self.args["delete_user"] = True
        self._test_valid()

    def test_src_arg_dependency(self):
        self.args["src"] = Path(__file__)

        # Test l_ls which requires --src
        self.args["l_ls"] = True
        self._test_valid()
        self.args["l_ls"] = False

        # Test  l_delete which requires --src
        self.args["l_delete"] = True
        self._test_valid()
        self.args["l_delete"] = False

        # Test l_mkdir which requires --src
        self.args["l_mkdir"] = True
        self._test_valid()

    def test_dst_arg_dependency(self):
        self.args["dst"] = Path(__file__)

        # Test ls which requires --dst
        self.args["ls"] = True
        self._test_valid()
        self.args["ls"] = False

        # Test mkdir which requires --dst
        self.args["mkdir"] = True
        self._test_valid()
        self.args["mkdir"] = False

        # Test  which requires --dst
        self.args["delete"] = True
        self._test_valid()

    def test_dst_src_arg_dependency(self):
        self.args["src"] = Path(__file__)
        self.args["dst"] = Path(__file__)

        # Test put which requires --dst and --src
        self.args["put"] = True
        self._test_valid()

    def test_create_user(self):
        """Test dependency of creating user which only requires the --perm"""
        self.args["create_user"] = True

        # Error because missing --perm
        with self.assertRaises(ValueError):
            client_classes.ClientRequest(**self.args)

        self.args["perm"] = client_classes.UserPerm["READ"]
        self._test_valid()


if __name__ == '__main__':
    unittest.main()
