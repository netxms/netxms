"""
Slash command handling.
"""

from __future__ import annotations

from typing import TYPE_CHECKING, Callable

if TYPE_CHECKING:
    from nxai.chat import ChatSession


class QuitRequested(Exception):
    """Raised when user requests to quit."""
    pass


class CommandHandler:
    """Handler for slash commands."""

    def __init__(self, session: ChatSession):
        """
        Initialize command handler.

        Args:
            session: Chat session to operate on
        """
        self.session = session
        self._commands: dict[str, Callable[..., None]] = {
            "help": self.cmd_help,
            "quit": self.cmd_quit,
            "exit": self.cmd_quit,
            "q": self.cmd_quit,
            "clear": self.cmd_clear,
            "object": self.cmd_object,
            "incident": self.cmd_incident,
            "status": self.cmd_status,
        }

    def handle(self, input_text: str) -> bool:
        """
        Handle a potential slash command.

        Args:
            input_text: User input text

        Returns:
            True if input was handled as a command, False otherwise

        Raises:
            QuitRequested: If user requested to quit
        """
        if not input_text.startswith("/"):
            return False

        # Parse command and arguments
        parts = input_text[1:].split(maxsplit=1)
        if not parts:
            return False

        command = parts[0].lower()
        args = parts[1] if len(parts) > 1 else ""

        # Look up command
        handler = self._commands.get(command)
        if handler is None:
            self.session.renderer.render_error(
                f"Unknown command: /{command}. Use /help for available commands."
            )
            return True

        # Execute command
        try:
            if args:
                handler(args)
            else:
                handler()
        except TypeError:
            # Command doesn't accept arguments but got some
            handler()

        return True

    def cmd_help(self) -> None:
        """Show help message."""
        self.session.renderer.render_help()

    def cmd_quit(self) -> None:
        """Quit the chat session."""
        raise QuitRequested()

    def cmd_clear(self) -> None:
        """Clear chat history."""
        try:
            self.session.client.clear_chat(self.session.chat_id)
            self.session.renderer.render_success("Chat history cleared.")
        except Exception as e:
            self.session.renderer.render_error(f"Failed to clear chat: {e}")

    def cmd_object(self, name: str = "") -> None:
        """
        Set object context.

        Args:
            name: Object name to search for
        """
        if not name:
            if self.session.context and "objectId" in self.session.context:
                self.session.context = None
                self.session.renderer.render_status("Object context cleared.")
            else:
                self.session.renderer.render_error(
                    "Usage: /object <name> - Set object context"
                )
            return

        # Search for object
        try:
            obj = self.session.client.find_object(name)
            if obj:
                object_id = obj.get("id") or obj.get("objectId")
                object_name = obj.get("name") or obj.get("objectName", name)
                self.session.context = {"objectId": object_id}
                self.session.renderer.render_success(
                    f"Context set to: {object_name} (ID: {object_id})"
                )
            else:
                self.session.renderer.render_error(f"Object not found: {name}")
        except Exception as e:
            self.session.renderer.render_error(f"Failed to find object: {e}")

    def cmd_incident(self, incident_id: str = "") -> None:
        """
        Set incident context.

        Args:
            incident_id: Incident ID
        """
        if not incident_id:
            if self.session.context and "incidentId" in self.session.context:
                self.session.context = None
                self.session.renderer.render_status("Incident context cleared.")
            else:
                self.session.renderer.render_error(
                    "Usage: /incident <id> - Set incident context"
                )
            return

        try:
            iid = int(incident_id)
            self.session.context = {"incidentId": iid}
            self.session.renderer.render_success(f"Context set to incident: {iid}")
        except ValueError:
            self.session.renderer.render_error("Invalid incident ID. Must be a number.")

    def cmd_status(self) -> None:
        """Show current session status."""
        renderer = self.session.renderer

        renderer.print()
        renderer.print("[bold]Session Status[/bold]")
        renderer.print(f"  Server: {self.session.server}")
        renderer.print(f"  Chat ID: {self.session.chat_id}")

        if self.session.context:
            if "objectId" in self.session.context:
                renderer.print(f"  Context: Object ID {self.session.context['objectId']}")
            elif "incidentId" in self.session.context:
                renderer.print(f"  Context: Incident ID {self.session.context['incidentId']}")
        else:
            renderer.print("  Context: None")

        renderer.print()
