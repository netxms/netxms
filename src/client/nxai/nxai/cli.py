"""
Command-line interface entry point.
"""

from __future__ import annotations

import argparse
import sys

from nxai import __version__
from nxai.client import AuthenticationError, NetXMSClient, NetXMSError
from nxai.chat import ChatSession
from nxai.render import Renderer
from nxai.session import (
    clear_session,
    get_credentials,
    get_server,
    load_session,
    save_session,
)


def parse_args() -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        prog="nxai",
        description="NetXMS AI Assistant CLI - Interactive chat with Iris",
        epilog="Environment variables: NETXMS_SERVER, NETXMS_USER, NETXMS_PASSWORD",
    )

    # Connection options
    parser.add_argument(
        "-s", "--server",
        help="NetXMS server hostname or URL (e.g., netxms.local or https://netxms.local:8443)",
        metavar="HOST",
    )
    parser.add_argument(
        "--port",
        type=int,
        help="WebAPI port (default: 443 for HTTPS)",
        metavar="PORT",
    )
    parser.add_argument(
        "-u", "--user",
        help="Username for authentication",
        metavar="USER",
    )
    parser.add_argument(
        "-p", "--password",
        help="Password (use environment variable NETXMS_PASSWORD instead for security)",
        metavar="PASS",
    )

    # Context options
    context_group = parser.add_mutually_exclusive_group()
    context_group.add_argument(
        "-n", "--node",
        help="Node name for context",
        metavar="NAME",
    )
    context_group.add_argument(
        "-o", "--object",
        help="Object ID for context",
        type=int,
        metavar="ID",
    )
    context_group.add_argument(
        "-i", "--incident",
        help="Incident ID for context",
        type=int,
        metavar="ID",
    )

    # Display options
    parser.add_argument(
        "--plain",
        action="store_true",
        help="Force plain text output (no colors or formatting)",
    )
    parser.add_argument(
        "--no-verify-ssl",
        action="store_true",
        help="Disable SSL certificate verification",
    )

    # Session options
    parser.add_argument(
        "--no-save-session",
        action="store_true",
        help="Don't save session token for reuse",
    )
    parser.add_argument(
        "--clear-session",
        action="store_true",
        help="Clear saved session token and exit",
    )

    # Meta options
    parser.add_argument(
        "-V", "--version",
        action="version",
        version=f"%(prog)s {__version__}",
    )

    return parser.parse_args()


def main() -> int:
    """Main entry point."""
    args = parse_args()
    renderer = Renderer(force_plain=args.plain)

    # Handle clear session
    if args.clear_session:
        try:
            server = get_server(args.server)
            clear_session(server)
            renderer.render_status(f"Session cleared for {server}")
            return 0
        except ValueError as e:
            renderer.render_error(str(e))
            return 1

    # Get server
    try:
        server = get_server(args.server)
        # Append port if specified and not already in URL
        if args.port and ":" not in server.split("//")[-1]:
            if server.startswith(("http://", "https://")):
                server = f"{server}:{args.port}"
            else:
                server = f"{server}:{args.port}"
    except ValueError as e:
        renderer.render_error(str(e))
        return 1

    # Try to load existing session
    token = None
    if not args.no_save_session:
        session_data = load_session(server)
        if session_data:
            token = session_data.get("token")
            renderer.render_status(f"Using saved session for {server}")

    # Create client
    client = NetXMSClient(
        server=server,
        token=token,
        verify_ssl=not args.no_verify_ssl,
    )

    try:
        # Authenticate if needed
        if not token:
            username, password = get_credentials(
                server,
                username=args.user,
                password=args.password,
            )

            renderer.render_status(f"Connecting to {server}...")

            try:
                token = client.login(username, password)
                if not args.no_save_session:
                    save_session(server, token)
                    renderer.render_status("Session saved.")
            except AuthenticationError as e:
                renderer.render_error(f"Authentication failed: {e}")
                return 1

        # Resolve node name to object ID if provided
        object_id = args.object
        if args.node:
            renderer.render_status(f"Looking up node '{args.node}'...")
            obj = client.find_object(args.node)
            if obj:
                object_id = obj.get("id") or obj.get("objectId")
                renderer.render_status(f"Found: {args.node} (ID: {object_id})")
            else:
                renderer.render_error(f"Node not found: {args.node}")
                return 1

        # Create chat session
        chat = ChatSession(
            client=client,
            renderer=renderer,
            server=server,
            incident_id=args.incident,
            object_id=object_id,
        )

        # Start chat (create on server)
        renderer.render_status("Starting chat session...")
        chat.start()

        # Run REPL
        try:
            chat.run()
        finally:
            chat.cleanup()

        return 0

    except NetXMSError as e:
        renderer.render_error(str(e))
        return 1

    except KeyboardInterrupt:
        renderer.print()
        renderer.render_status("Interrupted.")
        return 130

    except Exception as e:
        renderer.render_error(f"Unexpected error: {e}")
        return 1

    finally:
        client.close()


if __name__ == "__main__":
    sys.exit(main())
