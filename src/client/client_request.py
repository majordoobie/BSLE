from __future__ import annotations

from enum import Enum, auto
from pathlib import Path
from typing import Optional


class ActionType(Enum):
    SHELL = auto()
    DELETE_USER = auto()
    L_LS = auto()
    L_DELETE = auto()
    L_MKDIR = auto()
    LS = auto()
    MKDIR = auto()
    DELETE = auto()
    PUT = auto()
    CREATE_USER = auto()


class DependencyAction(Enum):
    """
    The action type Enums have a value that specifies the dependency of that
    action.
    0 - No dependency
    1 - Depends on --src
    2 - Depends on --dst
    3 - Depends on --src and --dst
    4 - Depends on --perm
    """
    SHELL = 0
    DELETE_USER = 0
    L_LS = 1
    L_DELETE = 1
    L_MKDIR = 1
    LS = 2
    MKDIR = 2
    DELETE = 2
    PUT = 3
    CREATE_USER = 4


class UserPerm(Enum):
    READ = auto()
    READ_WRITE = auto()
    ADMIN = auto()

    def __str__(self):
        """Provides an easier to read output when using -h"""
        return self.name

    @staticmethod
    def permission(perm: str):
        """Called from argparse.parse_args(). Takes the cli argument and
        attempts to return a UserPerm object if it exists."""
        try:
            return UserPerm[perm.upper()]
        except KeyError:
            raise ValueError()


class ClientRequest:
    def __init__(self, host: str,
                 port: int,
                 username: str,
                 src: Optional[Path],
                 dst: Optional[str],
                 perm: Optional[UserPerm],
                 session_id: Optional[int] = 0,
                 **kwargs) -> None:
        """
        Create a "dataclass" containing the configuration and action required
        to communicate with the server.

        Args:
            host (str): IP of the server
            port (int): Port of the server
            username (str): Username used to either authenticate or to action
                on when using commands such as "--create_user"
            src (:obj:`pathlib.Path`, optional): Path to the source file to
                reference
            dst (:obj:`pathlib.Path`, optional): Path to the server file to
                reference
            perm (:obj:`UserPerm`, optional): Permission to set to user when
                using "--create_user"
            **kwargs (dict): Remainder of optional arguments passed.
        """
        self.host = host
        self.port = port
        self.username = username
        self.src = src
        self.dst = dst g
        self.perm = perm
        self.session_id = session_id

        self.action: [ActionType] = None
        self._parse_kwargs(kwargs)

    @property
    def socket(self) -> tuple[str, int]:
        return self.host, self.port

    @property
    def client_request(self):


    def _parse_kwargs(self, kwargs) -> None:
        """
        Parse the remaining arguments and ensure that only a single action
        is specified. If more than a single argument is passed in, raise
        an error

        Args:
            kwargs (dict): Remainder of arguments passed in

        Raises:
            ValueError: If more than a single action is passed in or if the
                action is missing dependencies such as --src or --dst
        """
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

        dep_value = 0
        for name, member in DependencyAction.__members__.items():
            if name == self.action.name:
                dep_value = member.value
                break

        if dep_value == 1:
            if self.src is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--src\" argument")

        elif dep_value == 2:
            if self.dst is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--dst\" argument")

        elif dep_value == 3:
            if self.dst is None or self.src is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--src\" and \"--dst\" argument")

        elif dep_value == 4:
            if self.perm is None:
                raise ValueError(f"[!] Command \"--{self.action.name.lower()}\""
                                 f" requires \"--perm\" argument")

