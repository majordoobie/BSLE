from __future__ import annotations

import argparse
from pathlib import Path
from typing import Optional
from enum import Enum, auto


class ActionType(Enum):
    SHELL = 0
    LS = 2
    MKDIR = 2
    DELETE = 2
    PUT = 3
    L_LS = 1
    L_DELETE = 1
    L_MKDIR = 1
    CREATE_USER = 0
    DELETE_USER = 0


class ClientAction:
    def __init__(self, host: str, port: int, username: str, src: Optional[Path],
                 dst: Optional[Path], **kwargs):
        self.host = host
        self.port = port
        self.username = username
        self.src = src
        self.dst = dst
        self.action: [ActionType] = None
        self._parse_kwargs(kwargs)

    def _parse_kwargs(self, kwargs):
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

        if self.action.value == 2:
            if self.dst is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--dst\" argument")

        if self.action.value == 3:
            if self.dst is None or self.src is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--src\" and \"--dst\" argument")



def _local_path(path: str) -> Path:
    path = Path(path)

    if path.exists():
        if path.is_file():
            return path
        raise argparse.ArgumentTypeError(f"f[!] File \"{path}\" is not a file")
    raise argparse.ArgumentTypeError(f"[!] File \"{path}\" does not exist")


def get_args(args: list[str]) -> ClientAction:
    parser = argparse.ArgumentParser(
        description="File transfer application uses a CLI or interactive menu "
                    "to perform file transfers between the client and server."
    )

    parser.add_argument(
        "-i", "--server-ip", dest="host", default="127.0.0.1", metavar="",
        required=True,
        help="Specify the address of the server. (Default: %(default)s)"
    )
    parser.add_argument(
        "-p", "--server-port", dest="port", default=31337, metavar="",
        required=True,
        help="Specify the port to connect to. (Default: %(default)s)"
    )
    parser.add_argument(
        "-U", "--username", dest="username", type=str, metavar="",
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
    local_commands.add_argument(
        "--l_delete", dest="l_delete", action="store_true",
        help="Delete file at client directory. Can only be invoked by users "
             "with at least CREATE_RW permissions"
    )
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

    try:
        return ClientAction(**vars(parser.parse_args(args)))
    except ValueError as error:
        parser.error(error)
