import struct
from enum import Enum
import socket
from typing import Union

from client_request import ClientRequest

class RespHeader(Enum):
    RETURN_CODE = 1
    RESERVED = 1
    SESSION_ID = 4
    PAYLOAD_LEN = 8
    MSG_LEN = 1


def _read_stream(conn: socket, size: Union[int, RespHeader]) -> Union[bytes, int]:
    buffer = bytes()
    read_bytes = 0
    bytes_to_read = size if isinstance(size, int) else size.value

    while read_bytes != bytes_to_read:
        buffer += conn.recv(1)
        read_bytes += 1

    print(' '.join('{:02x}'.format(x) for x in buffer), end=" ")

    if isinstance(size, RespHeader):
        if 1 == bytes_to_read:
            buffer = struct.unpack("!B", buffer)[0]
        elif 4 == bytes_to_read:
            buffer = struct.unpack("!I", buffer)[0]
        elif 8 == bytes_to_read:
            buffer = struct.unpack("!Q", buffer)[0]

    return buffer


def make_connection(args: ClientRequest) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as conn:
        conn.connect(args.socket)
        conn.send(args.client_request)

        return_code = _read_stream(conn, RespHeader.RETURN_CODE)
        _ = _read_stream(conn, RespHeader.RESERVED)
        session_id = _read_stream(conn, RespHeader.SESSION_ID)
        payload_len = _read_stream(conn, RespHeader.PAYLOAD_LEN)
        msg_len = _read_stream(conn, RespHeader.MSG_LEN)

        msg = _read_stream(conn, msg_len)
        payload = _read_stream(conn, payload_len - (msg_len + RespHeader.MSG_LEN.value))

        print(msg)
        print(payload)
