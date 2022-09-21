from __future__ import annotations

import argparse
from enum import Enum, auto
from pathlib import Path
from typing import Optional


class ActionType(Enum):
    """
    The action type Enums have a value that specifies the dependency of that
    action.
    0 - No dependency
    1 - Depends on --src
    2 - Depends on --dst
    3 - Depends on --src and --dst
    4 - Depends on --perm
    """
    SHELL = 0
    LS = 2
    MKDIR = 2
    DELETE = 2
    PUT = 3
    L_LS = 1
    L_DELETE = 1
    L_MKDIR = 1
    CREATE_USER = 4
    DELETE_USER = 0


class UserPerm(Enum):
    READ = auto()
    READ_WRITE = auto()
    ADMIN = auto()

    def __str__(self):
        """Provides an easier to read output when using -h"""
        return self.name

    @staticmethod
    def permission(perm: str):
        """Called from argparse.parse_args(). Takes the cli argument and
        attempts to return a UserPerm object if it exists."""
        try:
            return UserPerm[perm.upper()]
        except KeyError:
            raise ValueError()


class ClientAction:
    def __init__(self, host: str, port: int, username: str, src: Optional[Path],
                 dst: Optional[Path], perm: Optional[UserPerm], **kwargs):
        """
        Create a "dataclass" containing the configuration and action required
        to communicate with the server.

        Args:
            host (str): IP of the server
            port (int): Port of the server
            username (str): Username used to either authenticate or to action
                on when using commands such as "--create_user"
            src (:obj:`pathlib.Path`, optional): Path to the source file to
                reference
            dst (:obj:`pathlib.Path`, optional): Path to the server file to
                reference
            perm (:obj:`UserPerm`, optional): Permission to set to user when
                using "--create_user"
            **kwargs (dict): Remainder of optional arguments passed.
        """
        self.host = host
        self.port = port
        self.username = username
        self.src = src
        self.dst = dst
        self.perm = perm

        self.action: [ActionType] = None
        self._parse_kwargs(kwargs)

    def _parse_kwargs(self, kwargs) -> None:
        """
        Parse the remaining arguments and ensure that only a single action
        is specified. If more than a single argument is passed in, raise
        an error

        Args:
            kwargs (dict): Remainder of arguments passed in

        Raises:
            ValueError: If more than a single action is passed in or if the
                action is missing dependencies such as --src or --dst
        """
        action = None
        for key, value in kwargs.items():
            if value:
                if action is not None:
                    raise ValueError("[!] Only one command flag may be set")
                action = key

        if action is None:
            raise ValueError("[!] No command flag set")

        for name, member in ActionType.__members__.items():
            if name == action.upper():
                self.action: ActionType = member

        if self.action.value == 1:
            if self.src is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--src\" argument")

        elif self.action.value == 2:
            if self.dst is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--dst\" argument")

        elif self.action.value == 3:
            if self.dst is None or self.src is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--src\" and \"--dst\" argument")

        elif self.action.value == 4:
            if self.perm is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--perm\" argument")


def _local_path(path: str) -> Path:
    """
    argparser callback for checking if the path provided exists

    Args:
        path (str): Path string

    Returns:
        pathlib.Path: Return path if file exists

    Raises:
        argparse.ArgumentTypeError: Raise type error if the path provided does
            not resolve to a file path
    """
    path = Path(path)

    if path.exists():
        if path.is_file():
            return path
        raise argparse.ArgumentTypeError(f"f[!] File \"{path}\" is not a file")
    raise argparse.ArgumentTypeError(f"[!] File \"{path}\" does not exist")


def get_args() -> ClientAction:
    """
    Parse CLI arguments and return a ClientAction dataclass

    Returns:
        ClientAction: Return ClientAction object
    """
    parser = argparse.ArgumentParser(
        description="File transfer application uses a CLI or interactive menu "
                    "to perform file transfers between the client and server."
    )

    parser.add_argument(
        "-i", "--server-ip", dest="host", default="127.0.0.1", metavar="[HOST]",
        required=True,
        help="Specify the address of the server. (Default: %(default)s)"
    )
    parser.add_argument(
        "-p", "--server-port", dest="port", default=31337, metavar="[PORT]",
        required=True,
        help="Specify the port to connect to. (Default: %(default)s)"
    )
    parser.add_argument(
        "-U", "--username", dest="username", type=str, metavar="[USR]",
        required=True, help="User to log in as."
    )
    parser.add_argument(
        "--shell", dest="shell", action="store_true",
        help="Drop into interactive menu mode"
    )

    # The --src and --dst are required arguments base on the action executed
    parser.add_argument(
        "--src", dest="src", type=_local_path, metavar="[SRC]",
        help="Source file to reference"
    )
    parser.add_argument(
        "--dst", dest="dst", type=Path, metavar="[DST]",
        help="Destination file to reference"
    )

    # Only one of the commands can be used at a time
    remote_commands = parser.add_argument_group(
        title="Remote Commands",
    )
    local_commands = parser.add_argument_group(
        title="Local Commands"
    )
    user_account_commands = parser.add_argument_group(
        title="User Account Commands"
    )

    #
    # Remote commands
    #
    remote_commands.add_argument(
        "--ls", dest="ls", action="store_true",
        help="List contents of server directory."
    )
    remote_commands.add_argument(
        "--mkdir", dest="mkdir", action="store_true",
        help="Create directory at server. Can only be invoked by users with "
             "at least CREATE_RW permissions."
    )
    remote_commands.add_argument(
        "--delete", dest="delete", action="store_true",
        help="Delete file at server directory. Can only be invoked by users "
             "with CREATE_RW permissions."
    )
    remote_commands.add_argument(
        "--put", dest="put", action="store_true",
        help="Copy file from client directory to server directory. Can only be "
             "invoked by users with CREATE_RW permissions."
    )

    #
    # Local commands
    #
    local_commands.add_argument(
        "--l_ls", dest="l_ls", action="store_true",
        help="List contents of client directory."
    )
    # local_commands.add_argument(
    #     "--l_delete", dest="l_delete", action="store_true",
    #     help="Delete file at client directory. Can only be invoked by users "
    #          "with at least CREATE_RW permissions"
    # )
    local_commands.add_argument(
        "--l_mkdir", dest="l_mkdir", action="store_true",
        help="Create directory at client directory. Can only be invoked with "
             "at least CREATE_RW permissions."
    )

    #
    # User account commands
    #
    user_account_commands.add_argument(
        "--create_user", dest="create_user", action="store_true",
        help="Create a user with the permissions of user invoking the command."
    )

    user_account_commands.add_argument(
        "--delete_user", dest="delete_user", action="store_true",
        help="Delete user. Can only be invoked by CREATE_ADMIN."
    )
    user_account_commands.add_argument(
        "--permission", dest="perm", type=UserPerm.permission,
        choices=list(UserPerm),
        help="User perms"
    )

    try:
        return ClientAction(**vars(parser.parse_args()))
    except ValueError as error:
        parser.error(str(error))
