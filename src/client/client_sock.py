import contextlib
import socket
import struct
from typing import Union

from client_classes import ClientRequest, RespHeader, ServerResponse, \
    SUCCESS_RESPONSE


@contextlib.contextmanager
def connection(client: ClientRequest) -> socket:
    """Context manager for keeping the socket open until the interactive
    session is complete"""
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.connect(client.socket)

    # Yield the connection to be used
    yield conn
    # Close the connection
    conn.close()


def make_connection(args: ClientRequest) -> ServerResponse:
    """Make a single connection and close the socket. This is used for
    the CLI"""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as conn:
        conn.connect(args.socket)
        resp = connect(args, conn)
    return resp


def connect(client: ClientRequest, conn: socket) -> ServerResponse:
    """
    Use the connected socket to send a ClientRequest

    :param client: ClientRequest object with connection information
    :param conn: Connected socket
    :return: Response from server
    """
    conn.send(client.client_request)

    return_code = _read_stream(conn, RespHeader.RETURN_CODE, client.debug)
    reserved = _read_stream(conn, RespHeader.RESERVED, client.debug)
    session_id = _read_stream(conn, RespHeader.SESSION_ID, client.debug)
    payload_len = _read_stream(conn, RespHeader.PAYLOAD_LEN, client.debug)
    msg_len = _read_stream(conn, RespHeader.MSG_LEN, client.debug)
    msg = _read_stream(conn, msg_len, client.debug).decode(encoding="utf-8")

    # Create the server response object
    resp = ServerResponse(client,
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
                                       client.debug)
            resp.payload = _read_stream(conn, stream_size, client.debug)

    # Needed to make a new line for the byte stream output
    if client.debug:
        print("\n")

    client.session = resp.session_id
    return resp


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
