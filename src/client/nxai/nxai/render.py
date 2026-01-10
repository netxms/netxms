"""
Terminal output rendering with adaptive rich/plain formatting.
"""

from __future__ import annotations

import sys
from contextlib import contextmanager
from typing import TYPE_CHECKING, Generator

from rich.console import Console
from rich.markdown import Markdown
from rich.panel import Panel
from rich.status import Status
from rich.style import Style
from rich.text import Text

if TYPE_CHECKING:
    from nxai.client import Question


class Renderer:
    """Adaptive terminal output renderer."""

    def __init__(self, force_plain: bool = False):
        """
        Initialize the renderer.

        Args:
            force_plain: Force plain text output even if terminal supports rich
        """
        self.force_plain = force_plain
        self.console = Console(
            force_terminal=not force_plain if sys.stdout.isatty() else False,
            no_color=force_plain,
        )

    def is_interactive(self) -> bool:
        """Check if we're in an interactive terminal."""
        return sys.stdout.isatty() and not self.force_plain

    def print(self, *args, **kwargs) -> None:
        """Print to console."""
        self.console.print(*args, **kwargs)

    def render_welcome(self) -> None:
        """Render welcome message."""
        if self.is_interactive():
            self.console.print()
            self.console.print(
                Panel(
                    "[bold blue]Iris[/bold blue] - NetXMS AI Assistant\n\n"
                    "Type your questions or commands. Use [bold]/help[/bold] for available commands.",
                    title="Welcome",
                    border_style="blue",
                )
            )
            self.console.print()
        else:
            print("Iris - NetXMS AI Assistant")
            print("Type your questions or commands. Use /help for available commands.")
            print()

    def render_response(self, text: str) -> None:
        """
        Render AI assistant response with markdown formatting.

        Args:
            text: Response text (may contain markdown)
        """
        if self.is_interactive():
            self.console.print()
            # Add "Iris>" prefix
            self.console.print(Text("Iris> ", style=Style(color="blue", bold=True)), end="")
            # Render markdown
            md = Markdown(text)
            self.console.print(md)
            self.console.print()
        else:
            print()
            print(f"Iris> {text}")
            print()

    def render_user_prompt(self) -> str:
        """Get the user prompt string."""
        if self.is_interactive():
            return ""  # prompt_toolkit handles this
        return "You> "

    def render_user_input(self, text: str) -> None:
        """
        Render user input in dim style for scrollback distinction.

        Args:
            text: User's input text
        """
        if self.is_interactive():
            self.console.print(f"[dim]You> {text}[/dim]")
        else:
            print(f"You> {text}")

    def render_error(self, message: str) -> None:
        """
        Render an error message.

        Args:
            message: Error message
        """
        if self.is_interactive():
            self.console.print(f"[bold red]Error:[/bold red] {message}")
        else:
            print(f"Error: {message}", file=sys.stderr)

    def render_warning(self, message: str) -> None:
        """
        Render a warning message.

        Args:
            message: Warning message
        """
        if self.is_interactive():
            self.console.print(f"[bold yellow]Warning:[/bold yellow] {message}")
        else:
            print(f"Warning: {message}", file=sys.stderr)

    def render_status(self, message: str) -> None:
        """
        Render a status message.

        Args:
            message: Status message
        """
        if self.is_interactive():
            self.console.print(f"[dim]{message}[/dim]")
        else:
            print(message)

    def render_success(self, message: str) -> None:
        """
        Render a success message.

        Args:
            message: Success message
        """
        if self.is_interactive():
            self.console.print(f"[bold green]{message}[/bold green]")
        else:
            print(message)

    def render_question(self, question: Question) -> None:
        """
        Render a question from the AI.

        Args:
            question: Question object
        """
        if self.is_interactive():
            # Build question content
            content = f"[bold]{question.text}[/bold]"
            if question.context:
                content += f"\n\n[dim]Context:[/dim] {question.context}"

            if question.type == "multipleChoice" and question.options:
                content += "\n\n[dim]Options:[/dim]"
                for i, opt in enumerate(question.options, 1):
                    content += f"\n  {i}. {opt}"
            else:
                # Confirmation type
                labels = self._get_confirmation_labels(question.confirmation_type)
                content += f"\n\n[dim]Enter[/dim] [green]{labels[0]}[/green] [dim]or[/dim] [red]{labels[1]}[/red]"

            self.console.print()
            self.console.print(
                Panel(
                    content,
                    title="[bold blue]Iris is asking[/bold blue]",
                    border_style="blue",
                )
            )
        else:
            print()
            print(f"Iris is asking: {question.text}")
            if question.context:
                print(f"Context: {question.context}")
            if question.type == "multipleChoice" and question.options:
                print("Options:")
                for i, opt in enumerate(question.options, 1):
                    print(f"  {i}. {opt}")
            else:
                labels = self._get_confirmation_labels(question.confirmation_type)
                print(f"Enter '{labels[0]}' or '{labels[1]}'")

    def _get_confirmation_labels(self, confirmation_type: int) -> tuple[str, str]:
        """Get positive/negative labels for confirmation type."""
        if confirmation_type == 0:  # APPROVE_REJECT
            return ("approve", "reject")
        elif confirmation_type == 1:  # YES_NO
            return ("yes", "no")
        else:  # CONFIRM_CANCEL
            return ("confirm", "cancel")

    def render_help(self) -> None:
        """Render help message."""
        help_text = """
[bold]Available Commands:[/bold]

  [cyan]/help[/cyan]              Show this help message
  [cyan]/quit[/cyan], [cyan]/exit[/cyan]      Exit the chat
  [cyan]/clear[/cyan]             Clear chat history
  [cyan]/object <name>[/cyan]     Set object context
  [cyan]/incident <id>[/cyan]     Set incident context
  [cyan]/status[/cyan]            Show current session info

[bold]Keyboard Shortcuts:[/bold]

  [cyan]Ctrl+C[/cyan]             Cancel current request
  [cyan]Ctrl+D[/cyan]             Exit the chat
"""
        if self.is_interactive():
            self.console.print(Panel(help_text.strip(), title="Help", border_style="cyan"))
        else:
            print(help_text)

    @contextmanager
    def show_spinner(self, message: str = "Thinking...") -> Generator[None, None, None]:
        """
        Show a spinner while waiting for a response.

        Args:
            message: Message to display with spinner
        """
        if self.is_interactive():
            with Status(f"[bold blue]{message}[/bold blue]", console=self.console):
                yield
        else:
            print(message)
            yield

    def render_connection_status(
        self, server: str, username: str, context: str | None = None
    ) -> None:
        """
        Render connection status.

        Args:
            server: Server hostname
            username: Connected username
            context: Optional context description
        """
        if self.is_interactive():
            msg = f"Connected to [bold]{server}[/bold] as [bold]{username}[/bold]"
            if context:
                msg += f"\nContext: [cyan]{context}[/cyan]"
            self.console.print(msg)
        else:
            msg = f"Connected to {server} as {username}"
            if context:
                msg += f" (Context: {context})"
            print(msg)
