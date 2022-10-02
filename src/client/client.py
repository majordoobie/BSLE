import getpass
import struct
import socket

import cli_parser
import client_sock
import client_ctrl


def _socket_timedout(conn: socket.socket):
    i = conn.recv(1, socket.MSG_PEEK)
    payload = struct.unpack(">B", i)
    print("The timout func read ", payload)
    if i[0] == 2:
        return True
    return False


def _get_password(msg: str) -> str:
    return getpass.getpass(msg)


def main() -> None:
    args = None
    try:
        args = cli_parser.get_args()
    except Exception as error:
        exit(error)

    args.self_password = _get_password(f"[Enter password for {args.self_username}]\n> ")
    if args.require_other_password:
        args.other_password = _get_password(f"[Enter password for {args.other_username}]\n> ")

    if args.debug:
        print(args)

    resp = client_sock.make_connection(args)
    client_ctrl.parse_action(resp)


if __name__ == "__main__":
    main()
