from cli_parser import get_args


def main() -> None:
    args = None
    try:
        args = get_args()
        print(args.action)
    except Exception as error:
        exit(error)

if __name__ == "__main__":
    main()
