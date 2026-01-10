"""
Main chat session and REPL logic.
"""

from __future__ import annotations

from typing import Any

from prompt_toolkit import PromptSession
from prompt_toolkit.completion import Completer, Completion
from prompt_toolkit.history import FileHistory
from prompt_toolkit.shortcuts import CompleteStyle
from prompt_toolkit.styles import Style

from nxai.client import NetXMSClient, NetXMSError
from nxai.commands import CommandHandler, QuitRequested
from nxai.questions import handle_question
from nxai.render import Renderer
from nxai.session import get_config_dir


SLASH_COMMANDS = {
    "/help": "Show available commands",
    "/quit": "Exit the chat",
    "/exit": "Exit the chat",
    "/q": "Exit the chat",
    "/clear": "Clear chat history",
    "/object": "Set object context by name",
    "/incident": "Set incident context by ID",
    "/status": "Show current session info",
}


class SlashCommandCompleter(Completer):
    """Completer for slash commands."""

    def get_completions(self, document, complete_event):
        text = document.text_before_cursor

        # Only complete if line starts with /
        if not text.startswith("/"):
            return

        # Get the word being typed
        word = text.lstrip("/")

        for cmd, description in SLASH_COMMANDS.items():
            cmd_name = cmd[1:]  # Remove leading /
            if cmd_name.startswith(word):
                yield Completion(
                    cmd,
                    start_position=-len(text),
                    display_meta=description,
                )


class ChatSession:
    """Interactive chat session with Iris."""

    def __init__(
        self,
        client: NetXMSClient,
        renderer: Renderer,
        server: str,
        incident_id: int | None = None,
        object_id: int | None = None,
    ):
        """
        Initialize chat session.

        Args:
            client: NetXMS API client
            renderer: Output renderer
            server: Server hostname (for display)
            incident_id: Optional incident ID for context
            object_id: Optional object ID for context
        """
        self.client = client
        self.renderer = renderer
        self.server = server
        self.chat_id: int = 0
        self.context: dict[str, Any] | None = None

        # Set initial context
        if incident_id:
            self.context = {"incidentId": incident_id}
        elif object_id:
            self.context = {"objectId": object_id}

        # Command handler
        self.command_handler = CommandHandler(self)

        # Prompt session with history and completion
        history_file = get_config_dir() / "history"
        self.prompt_session: PromptSession = PromptSession(
            history=FileHistory(str(history_file)),
            completer=SlashCommandCompleter(),
            complete_style=CompleteStyle.READLINE_LIKE,
            style=Style.from_dict({
                "prompt": "bold cyan",
                # Readline-style completion styling
                "readline-like-completions.completion": "#0087ff",
            }),
        )

    def start(self) -> None:
        """Start the chat session by creating a chat on the server."""
        incident_id = None
        object_id = None

        if self.context:
            incident_id = self.context.get("incidentId")
            object_id = self.context.get("objectId")

        self.chat_id = self.client.create_chat(
            incident_id=incident_id,
            object_id=object_id,
        )

    def run(self) -> None:
        """Run the main REPL loop."""
        self.renderer.render_welcome()

        while True:
            try:
                # Get user input
                if self.renderer.is_interactive():
                    user_input = self.prompt_session.prompt(
                        [("class:prompt", "You> ")],
                    )
                else:
                    user_input = input(self.renderer.render_user_prompt())

                user_input = user_input.strip()

                if not user_input:
                    continue

                # Replace bright prompt with dim version in scrollback
                if self.renderer.is_interactive():
                    # Move up, clear line, print dim version
                    print("\x1b[1A\x1b[2K", end="")
                    self.renderer.render_user_input(user_input)

                # Handle slash commands
                if self.command_handler.handle(user_input):
                    continue

                # Send message to AI
                self.send_message(user_input)

            except QuitRequested:
                self.renderer.render_status("Goodbye!")
                break

            except EOFError:
                # Ctrl+D
                self.renderer.print()
                self.renderer.render_status("Goodbye!")
                break

            except KeyboardInterrupt:
                # Ctrl+C - just show new prompt
                self.renderer.print()
                continue

    def send_message(self, message: str) -> None:
        """
        Send a message to the AI and handle response.

        Args:
            message: User message
        """
        try:
            with self.renderer.show_spinner("Thinking..."):
                response = self.client.send_message(
                    self.chat_id,
                    message,
                    context=self.context,
                )

            # Clear context after sending (only send once)
            # Keep tracking it but don't resend
            context_sent = self.context is not None

            # Render response
            if response.response:
                self.renderer.render_response(response.response)

            # Handle pending question if any
            if response.pending_question:
                handle_question(
                    self.client,
                    self.chat_id,
                    response.pending_question,
                    self.renderer,
                )

        except KeyboardInterrupt:
            self.renderer.print()
            self.renderer.render_warning("Request cancelled.")

        except NetXMSError as e:
            self.renderer.render_error(str(e))

        except Exception as e:
            self.renderer.render_error(f"Unexpected error: {e}")

    def cleanup(self) -> None:
        """Clean up resources and delete chat session."""
        if self.chat_id:
            try:
                self.client.delete_chat(self.chat_id)
            except Exception:
                pass  # Ignore cleanup errors
