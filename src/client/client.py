from cli_parser import get_args
import sys


def main() -> None:
    try:
        args = get_args(sys.argv[1:])
        print(args.host)
    except Exception as error:
        print("hi")
        print(error)


if __name__ == "__main__":
    main()
