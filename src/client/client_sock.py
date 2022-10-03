import socket
import struct
from typing import Union
import contextlib

from client_classes import ClientRequest, RespHeader, ServerResponse, SUCCESS_RESPONSE


def _read_stream(conn: socket, size: Union[int, RespHeader], debug: bool = False) -> Union[bytes, int]:
    buffer = bytes()
    bytes_to_read = size if isinstance(size, int) else size.value

    while bytes_to_read != len(buffer):
        buffer += conn.recv((bytes_to_read - len(buffer)))

    if debug:
        print(' '.join('{:02x}'.format(x) for x in buffer), end=" ")

    if isinstance(size, RespHeader):
        if 1 == bytes_to_read:
            buffer = struct.unpack("!B", buffer)[0]
        elif 4 == bytes_to_read:
            buffer = struct.unpack("!I", buffer)[0]
        elif 8 == bytes_to_read:
            buffer = struct.unpack("!Q", buffer)[0]

    return buffer


def _socket_timedout(conn: socket.socket):
    i = conn.recv(1, socket.MSG_PEEK)
    payload = struct.unpack(">B", i)
    print("The timout func read ", payload)
    if i[0] == 2:
        return True
    return False


def connect(args: ClientRequest, conn: socket) -> ServerResponse:
    conn.send(args.client_request)

    return_code = _read_stream(conn, RespHeader.RETURN_CODE, args.debug)
    reserved = _read_stream(conn, RespHeader.RESERVED, args.debug)
    session_id = _read_stream(conn, RespHeader.SESSION_ID, args.debug)
    payload_len = _read_stream(conn, RespHeader.PAYLOAD_LEN, args.debug)
    msg_len = _read_stream(conn, RespHeader.MSG_LEN, args.debug)
    msg = _read_stream(conn, msg_len, args.debug).decode(encoding="utf-8")

    # Create the server response object
    resp = ServerResponse(args,
                          return_code,
                          reserved,
                          session_id,
                          payload_len,
                          msg_len,
                          msg)

    # If message was successful, extract the payload if it exists
    if SUCCESS_RESPONSE == return_code:

        stream_size = payload_len - (msg_len
                                     + RespHeader.MSG_LEN.value
                                     + RespHeader.SHA256DIGEST.value)

        # If there is data in the stream pull it
        if stream_size > 0:
            resp.digest = _read_stream(conn,
                                       RespHeader.SHA256DIGEST,
                                       args.debug)
            resp.payload = _read_stream(conn, stream_size, args.debug)

    # Needed to make a new line for the byte stream output
    if args.debug:
        print("\n")
    return resp


def make_connection(args: ClientRequest) -> ServerResponse:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as conn:
        conn.connect(args.socket)
        resp = connect(args, conn)
    return resp


@contextlib.contextmanager
def connection(client: ClientRequest) -> socket:
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect(client.socket)

    # Yield the connection to be used
    yield conn
    # Close the connection
    conn.close()
