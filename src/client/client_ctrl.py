from dataclasses import dataclass
from typing import Union

from client_classes import ClientRequest, ActionType
from client_sock import ServerResponse


@dataclass
class DirList:
    f_type: str
    _size: int
    f_name: str

    def __str__(self):
        return f"{self.f_type} {self.f_size} {self.f_name}"

    @property
    def f_size(self) -> str:
        """Print the value of the size as "human-readable" """
        suffix = "B"
        for unit in ["", "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi"]:
            if abs(self._size) < 1024.0:
                return f"{self._size:0.0f}{unit}{suffix}"
            self._size /= 1024.0


def _do_list_ldir(args: ClientRequest) -> None:
    contents = ""
    for file in args._src.iterdir():
        # Only ready file and dirs
        if file.is_file() or file.is_dir():
            _type = "[F]" if file.is_file() else "[D]"
            contents += f"{_type}:{file.stat().st_size}:{file.name}\n"

    _parse_dir(contents)


def _parse_dir(array: Union[str, bytes]) -> None:
    """
    Break the stream of characters that represents the directory listing
    and format it to print to the screen.

    :param array: String array in the format of `f_type:f_size:f_name\n`
    """
    if isinstance(array, bytes):
        array = array.decode(encoding="utf-8")

    dir_objs = []
    for file in array.split("\n"):
        if "" == file:
            break

        _type, size, name = file.split(":")
        if not size.isdigit():
            raise ValueError("File size is not of type int")
        dir_objs.append(DirList(_type, int(size), name))

    dir_objs.sort(key=lambda x: (x.f_type, -x._size))
    for i in dir_objs:
        print(f"{i.f_type} {i.f_size:>6} {i.f_name}")


def _do_lmkdir(args: ClientRequest) -> None:
    try:
        args._src.mkdir()
        print("[+] Create directory")
    except FileNotFoundError:
        print("[!] Path is missing parent directories. Make those directories "
              "first")
    except FileExistsError:
        print("[!] The directory you are attempting to create already exists")
    except Exception as error:
        print(f"[!] {error}")


def _do_ldelete(args: ClientRequest) -> None:
    try:
        if args._src.is_file():
            args._src.unlink()
            print("[!] Deleted file")
        else:
            args._src.rmdir()
            print("[!] Deleted directory")

    except FileNotFoundError:
        print("[!] File not found")
    except Exception as error:
        print(f"[!] {error}")


def parse_action(resp: ServerResponse) -> None:
    if not resp.successful:
        print(f"[!] {resp.msg}")
        return

    if ActionType.LS == resp.action:
        if resp.valid_hash:
            _parse_dir(resp.payload)

    elif ActionType.L_LS == resp.action:
        _do_list_ldir(resp.request)



