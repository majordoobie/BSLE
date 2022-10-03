from __future__ import annotations

import shlex
import getpass
from pathlib import Path
from socket import socket

from client_classes import ClientRequest
import client_ctrl
import client_sock


def _parse_args(args: list[str]) -> dict:
    """
    Parse the user input and raise exceptions when the user:
        - provides too many arguments
        - does not fulfill the required arguments

    The amount of valid arguments is calculated by fetching the commands
    args list.
        Example:
            "mkdir": {
                "help": "Makes directory at server [dst]",
                "args": ["r_dst"],
                "callback": _mkdir,

    The args list contains the arguments that the command or "callback" takes.
    If the argument begins with "r_" then the argument is mandatory,
    otherwise the argument is optional.

    The response dictionary contains the args for the callback with
    the value set by the use.
        Example:
            { "r_dst": "some/path" }

    :param args: List of user input tokens
    :return: Dictionary containing the
    """

    arg_dict = {}

    cmd_name = args[0]
    cmd_pld = CMDS.get(cmd_name)

    args_collected = 1 # counting the cmd as an arg
    for index, arg in enumerate(cmd_pld.get("args", [])):
        arg_dict[arg] = None
        try:
            arg_dict[arg] = args[index + 1]
            args_collected += 1
        except IndexError:
            pass

    if len(args) > args_collected:
        raise ValueError("[!] Too many arguments for this command provided. "
                         f"User `help {cmd_name}` for more information")

    for arg, value in arg_dict.items():
        if arg.startswith("r_"):
            if value is None:
                _, arg = arg.split("r_")
                raise ValueError(f"[!] Command {cmd_name} requires {arg}. Use "
                                 f"`help {cmd_name}` for more information")
    return arg_dict


def _get(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    dst = cmd_args.get("r_dst")
    src = cmd_args.get("r_src")
    client.set_get(dst, Path(src))
    resp = client_sock.make_connection(client)
    client_ctrl.parse_action(resp)


def _put(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    dst = cmd_args.get("r_dst")
    src = cmd_args.get("r_src")
    client.set_put(dst, Path(src))
    resp = client_sock.make_connection(client)
    client_ctrl.parse_action(resp)


def _delete(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    dst = cmd_args.get("r_dst")
    client.set_delete(dst)
    resp = client_sock.make_connection(client)
    client_ctrl.parse_action(resp)


def _l_delete(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return
    src = Path(cmd_args.get("r_src"))
    client.set_locals(src)
    client_ctrl.do_ldelete(client)


def _ls(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    dst = cmd_args.get("dst")
    dst = dst if dst else "/"
    client.set_ls(dst)
    resp = client_sock.make_connection(client)
    client_ctrl.parse_action(resp)


def _l_ls(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    src = cmd_args.get("src")
    src = Path(src) if src else Path(".")
    client.set_locals(src)
    client_ctrl.do_list_ldir(client)


def _mkdir(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    dst = cmd_args.get("r_dst")
    client.set_mkdir(dst)
    resp = client_sock.make_connection(client)
    client_ctrl.parse_action(resp)


def _l_mkdir(client: ClientRequest, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    src = Path(cmd_args.get("r_src"))
    client.set_locals(src)
    client_ctrl.do_lmkdir(client)
    return


def _help(client: ClientRequest, args: list[str]) -> None:
    if len(args) > 2:
        print(f"[!] Too many arguments for command. Use help {args[0]} "
              f"for more guidance")
    if 1 == len(args):
        _print_main_menu()
    else:
        if CMDS.get(args[1].lower()):
            cmd_name = args[1].lower()
            cmd_pld = CMDS.get(cmd_name)

            req_args = []
            opt_args = []
            for arg in cmd_pld.get("args", []):
                if arg.startswith("r_"):
                    _, arg = arg.split("_")
                    req_args.append(arg)
                else:
                    opt_args.append(arg)

            req_arg_str = " ".join(req_args)
            opt_arg_str = " ".join(opt_args)
            arg_out = ""

            if req_args:
                arg_out += req_arg_str

            if opt_arg_str:
                arg_out += "[OPT: "
                arg_out += opt_arg_str
                arg_out += "]"

            arg_desc = ""
            for arg in req_args + opt_args:
                arg_desc += f"       - {arg:<8}{ARGS.get(arg)}"
                arg_desc += "\n"
            print(
                f"CMD:  {cmd_name} {arg_out}\n"
                f"HELP: {cmd_pld.get('help')}\n"
                f"{arg_desc}"
            )


def _quit(*args) -> None:
    exit()


def _print_main_menu() -> None:
    print("Help menu...")
    for cmd_name, cmd in CMDS.items():
        print(f"{f'{cmd_name}:':<10}{cmd.get('help')}\n")
    print("\n")


def get_password(msg: str) -> str:
    return getpass.getpass(msg)


def _interact(client: ClientRequest) -> None:
    """
    While in a valid session, continuously ask the user for input

    :param client: Initial client request object
    """
    _print_main_menu()
    while True:
        cmd = shlex.split(input("\n> "))
        if cmd:
            cmd[0] = cmd[0].lower()
            if not CMDS.get(cmd[0]):
                print("[!] Invalid command. Use \"help\" for a list of "
                      "commands.")
            else:
                callback = CMDS.get(cmd[0])
                callback.get("callback")(client, cmd)
        else:
            print()


def shell(client: ClientRequest) -> None:
    """
    Drop into an interactive shell with the server

    :param client: Client object holding the initial connection configuration
    """
    while True:
        client.self_password = get_password("password: ")
        client.set_auth_headers()
        try:
            # Upon session timeouts the socket will close and the
            # user will get asked to re-authenticate
            resp = client_sock.make_connection(client)
            if not resp.successful:
                exit(f"[!] {resp.msg}")
            client.session = resp.session_id
            _interact(client)

        # Occurs when the active connection times out (10 seconds)
        except TimeoutError:
            client.session = 0
            pass
        except KeyboardInterrupt:
            exit("later")
        except Exception as error:
            exit(error)


ARGS = {
    "src": "Local PATH to reference",
    "dst": "Remote PATH to reference",
    "cmds": "Command name"
}
CMDS = {
    "get": {
        "help": "Gets a file from server [dst] path and copies it into the "
                "client [src] path\n\t  Example: get remote/file.txt local/dir",
        "args": ["r_dst", "r_src"],
        "callback": _get,
    },
    "put": {
        "help": "Sends a file from client [src] path to be placed in the "
                "server [dst] path.\n\t  Example: put source_file.txt "
                "dest/folder",
        "args": ["r_src", "r_dst"],
        "callback": _put,
    },
    "help": {
        "help": "Displays this help menu",
        "args": ["cmds"],
        "callback": _help,
    },
    "quit": {
        "help": "Exits interactive mode",
        "args": [],
        "callback": _quit,
    },
    "delete": {
        "help": "Deletes file at server [dst]",
        "args": ["r_dst"],
        "callback": _delete,
    },
    "l_delete": {
        "help": "Deletes file at local [src]",
        "args": ["r_src"],
        "callback": _l_delete,
    },
    "ls": {
        "help": "Lists remote directory contents",
        "args": ["dst"],
        "callback": _ls,
    },
    "l_ls": {
        "help": "List local directory contents as [src]",
        "args": ["src"],
        "callback": _l_ls,
    },
    "mkdir": {
        "help": "Makes directory at server [dst]",
        "args": ["r_dst"],
        "callback": _mkdir,
    },
    "l_mkdir": {
        "help": "Makes directory at client [src]",
        "args": ["r_src"],
        "callback": _l_mkdir,
    },
}
