from cli_parser import get_args
import sys


def main() -> None:
    args = None
    try:
        args = get_args(sys.argv[1:])
    except Exception as error:
        exit(error)

    print(args.action)


if __name__ == "__main__":
    main()
