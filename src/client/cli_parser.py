from __future__ import annotations

import argparse
from pathlib import Path

from client_classes import ClientRequest, UserPerm


def get_args() -> ClientRequest:
    """
    Parse CLI arguments and return a ClientAction dataclass

    Returns:
        ClientRequest: Return ClientAction object
    """
    parser = argparse.ArgumentParser(
        description="File transfer application uses a CLI or interactive menu "
                    "to perform file transfers between the client and server."
    )

    parser.add_argument("--debug", dest="debug", action="store_true",
                        help=argparse.SUPPRESS)

    parser.add_argument(
        "-i", "--server-ip", dest="host", default="127.0.0.1", metavar="[HOST]",
        help="Specify the address of the server. (Default: %(default)s)"
    )
    parser.add_argument(
        "-p", "--server-port", dest="port", type=int, default=31337,
        metavar="[PORT]",
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
        "--src", dest="src", type=Path, metavar="[SRC]",
        help="Source file to reference"
    )
    parser.add_argument(
        "--dst", dest="dst", type=str, metavar="[DST]",
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

    remote_commands.add_argument(
        "--get", dest="get", action="store_true",
        help="Copy file from server directory to client directory.")

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
        "--create_user", dest="create_user", type=str, metavar="[USR]",
        help="Create a user with the permissions of user invoking the command."
    )

    user_account_commands.add_argument(
        "--delete_user", dest="delete_user", type=str, metavar="[USR]",
        help="Delete user. Can only be invoked by CREATE_ADMIN."
    )
    user_account_commands.add_argument(
        "--permission", dest="perm", type=UserPerm.permission,
        choices=list(UserPerm), default=UserPerm.READ,
        help="Permissions of the user to set when creating them "
             "(Default: %(default)s)"
    )

    args = parser.parse_args()
    if args.debug:
        for k,v in vars(parser.parse_args()).items():
            print(f"{k:<20}{v}")

    try:
        return ClientRequest(**vars(args))
    except ValueError as error:
        parser.error(f"[!] {str(error)}")
