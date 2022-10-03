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
    Parse the args provided by the user

    :param args: List of arguments split from the command line
    :return: Dictionary of arguments parsed
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

def _get(client: ClientRequest, conn: socket, args: list[str]) -> None:
    pass
def _put(client: ClientRequest, conn: socket, args: list[str]) -> None:
    pass
def _delete(client: ClientRequest, conn: socket, args: list[str]) -> None:
    pass

def _l_delete(client: ClientRequest, conn: socket, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return
    src = Path(cmd_args.get("r_src"))
    client.set_locals(src)
    client_ctrl.do_ldelete(client)


def _ls(client: ClientRequest, conn: socket, args: list[str]) -> None:
    pass


def _l_ls(client: ClientRequest, conn: socket, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    src = cmd_args.get("src")
    src = Path(src) if src else Path(".")
    client.set_locals(src)
    client_ctrl.do_list_ldir(client)

def _mkdir(client: ClientRequest, conn: socket, args: list[str]) -> None:
    pass


def _l_mkdir(client: ClientRequest, conn: socket, args: list[str]) -> None:
    try:
        cmd_args = _parse_args(args)
    except ValueError as error:
        print(error)
        return

    src = Path(cmd_args.get("r_src"))
    client.set_locals(src)
    client_ctrl.do_lmkdir(client)


def _help(client: ClientRequest, conn: socket, args: list[str]) -> None:
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


def _quit(client: ClientRequest, conn: socket, args: list[str]) -> None:
    exit()


def _print_main_menu() -> None:
    print("Help menu...")
    for cmd_name, cmd in CMDS.items():
        print(f"{cmd_name:<10}{cmd.get('help')}")
    print("\n")


def get_password(msg: str) -> str:
    return getpass.getpass(msg)


def _authenticate(main: ClientRequest) -> None:
    main.set_auth_headers()
    resp = client_sock.make_connection(main)
    if resp.successful:
        main.session = resp.session_id
        print("[+] Welcome back!\n")
        return
    exit(f"[!] {resp.msg}")


def shell(client: ClientRequest) -> None:
    client.self_password = get_password("password: ")
    client.set_auth_headers()
    with client_sock.connection(client) as conn:
        resp = client_sock.connect(client, conn)
        if not resp.successful:
            exit(f"[!] {resp.msg}")

        client.session = resp.session_id
        print("[+] Welcome back!\n")
        _print_main_menu()
        run = True
        while run:
            cmd = shlex.split(input("\n> "))
            if cmd:
                cmd[0] = cmd[0].lower()
                if not CMDS.get(cmd[0]):
                    print("[!] Invalid cmd. User \"help\" if you need guidance")
                else:
                    callback = CMDS.get(cmd[0])
                    callback.get("callback")(client, conn, cmd)
            else:
                print()


ARGS = {
    "src": "Local PATH to reference",
    "dst": "Remote PATH to reference",
    "cmds": "Command name"
}
CMDS = {
    "get": {
        "help": "Gets a file from server [dst] path and copies it into the "
                "client [src] path\nExample: get remote/file.txt local/dir",
        "args": ["r_dst", "r_src"],
        "callback": _get,
    },
    "put": {
        "help": "Sends a file from client [src] path to be placed in the "
                "server [dst] path. \nExample: put source_file.txt dest/folder",
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
