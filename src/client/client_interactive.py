from __future__ import annotations

import shlex
import getpass

from client_classes import ClientRequest
import client_sock


def _help(args: list[str]) -> None:
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


def _quit(*args: list[str]) -> None:
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


def shell(main: ClientRequest) -> None:
    main.self_password = get_password("password: ")
    _authenticate(main)

    _print_main_menu()
    run = True
    while run:
        cmd = shlex.split(input("> "))
        if cmd:
            cmd[0] = cmd[0].lower()
            if not CMDS.get(cmd[0]):
                print("[!] Invalid cmd. User \"help\" if you need guidance")
            else:
                callback = CMDS.get(cmd[0])
                callback.get("callback")(main, cmd)
        else:
            print()


ARGS = {
    "src": "Local PATH to reference",
    "dst": "Remote PATH to reference",
    "cmds": "Command name"
}
CMDS = {
    "get": {
        "help": "Gets a file from server [src] path and copies it into the "
                "client [dst] path",
        "args": ["r_dst", "r_src"],
    },
    "put": {
        "help": "Sends a file from client [src] path to be placed in the "
                "server [dst] path",
        "args": ["r_dst", "r_src"],
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
    },
    "l_delete": {
        "help": "Deletes file at local [src]",
        "args": ["r_src"],
    },
    "ls": {
        "help": "Lists remote directory contents",
        "args": ["dst"],
    },
    "l_ls": {
        "help": "List local directory contents as [src]",
        "args": ["src"],
    },
    "mkdir": {
        "help": "Makes directory at server [dst]",
        "args": ["r_dst"],
    },
    "l_mkdir": {
        "help": "Makes directory at client [src]",
        "args": ["r_src"],
    },
}
