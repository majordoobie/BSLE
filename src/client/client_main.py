import client_cli
import client_sock
import client_ctrl
import client_interactive as cli


def main() -> None:
    args = None
    try:
        args = client_cli.get_args()
    except Exception as error:
        exit(f"[!] {error}")

    if args.shell_mode:
        cli.shell(args)
    else:
        if args.debug:
            print(args)

        args.self_password = cli.get_password(
            f"[Enter password for {args.self_username}]\n> ")
        if args.require_other_password:
            args.other_password = cli.get_password(
                f"[Enter password for {args.other_username}]\n> ")

        resp = client_sock.make_connection(args)
        client_ctrl.parse_action(resp)


if __name__ == "__main__":
    main()
