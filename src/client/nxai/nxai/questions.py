"""
Question handling and user prompts.
"""

from __future__ import annotations

from typing import TYPE_CHECKING

from prompt_toolkit import prompt
from prompt_toolkit.validation import ValidationError, Validator

if TYPE_CHECKING:
    from nxai.client import NetXMSClient, Question
    from nxai.render import Renderer


class ConfirmationValidator(Validator):
    """Validator for confirmation inputs."""

    def __init__(self, positive: str, negative: str):
        self.positive = positive.lower()
        self.negative = negative.lower()
        self.valid_inputs = {
            self.positive,
            self.negative,
            self.positive[0],  # First letter shortcuts
            self.negative[0],
        }

    def validate(self, document) -> None:
        text = document.text.strip().lower()
        if text and text not in self.valid_inputs:
            raise ValidationError(
                message=f"Please enter '{self.positive}' or '{self.negative}'"
            )


class MultipleChoiceValidator(Validator):
    """Validator for multiple choice inputs."""

    def __init__(self, num_options: int):
        self.num_options = num_options

    def validate(self, document) -> None:
        text = document.text.strip()
        if not text:
            return
        try:
            choice = int(text)
            if choice < 1 or choice > self.num_options:
                raise ValidationError(
                    message=f"Please enter a number between 1 and {self.num_options}"
                )
        except ValueError:
            raise ValidationError(message="Please enter a number")


def get_confirmation_labels(confirmation_type: int) -> tuple[str, str]:
    """
    Get positive/negative labels for confirmation type.

    Args:
        confirmation_type: 0=approve_reject, 1=yes_no, 2=confirm_cancel

    Returns:
        Tuple of (positive_label, negative_label)
    """
    if confirmation_type == 0:  # APPROVE_REJECT
        return ("approve", "reject")
    elif confirmation_type == 1:  # YES_NO
        return ("yes", "no")
    else:  # CONFIRM_CANCEL
        return ("confirm", "cancel")


def prompt_confirmation(question: Question, renderer: Renderer) -> bool:
    """
    Prompt user for confirmation.

    Args:
        question: Question object
        renderer: Renderer for output

    Returns:
        True if positive response, False otherwise
    """
    positive, negative = get_confirmation_labels(question.confirmation_type)

    if renderer.is_interactive():
        validator = ConfirmationValidator(positive, negative)
        try:
            response = prompt(
                f"[{positive}/{negative}]: ",
                validator=validator,
            ).strip().lower()
        except (EOFError, KeyboardInterrupt):
            return False
    else:
        response = input(f"[{positive}/{negative}]: ").strip().lower()

    # Check if positive
    return response in (positive, positive[0])


def prompt_multiple_choice(question: Question, renderer: Renderer) -> int:
    """
    Prompt user for multiple choice selection.

    Args:
        question: Question object with options
        renderer: Renderer for output

    Returns:
        Selected option index (0-based), or -1 if cancelled
    """
    if not question.options:
        return -1

    num_options = len(question.options)

    if renderer.is_interactive():
        validator = MultipleChoiceValidator(num_options)
        try:
            response = prompt(
                "Enter choice (1-{}): ".format(num_options),
                validator=validator,
            ).strip()
        except (EOFError, KeyboardInterrupt):
            return -1
    else:
        response = input("Enter choice (1-{}): ".format(num_options)).strip()

    try:
        choice = int(response)
        if 1 <= choice <= num_options:
            return choice - 1  # Convert to 0-based index
    except ValueError:
        pass

    return -1


def handle_question(
    client: NetXMSClient,
    chat_id: int,
    question: Question,
    renderer: Renderer,
) -> None:
    """
    Handle a question from the AI, prompting user and sending response.

    Args:
        client: NetXMS client
        chat_id: Chat session ID
        question: Question to handle
        renderer: Renderer for output
    """
    # Display the question
    renderer.render_question(question)

    # Get user response
    if question.type == "multipleChoice":
        selected = prompt_multiple_choice(question, renderer)
        positive = selected >= 0
        option = selected
    else:
        positive = prompt_confirmation(question, renderer)
        option = -1

    # Send response to server
    try:
        client.answer_question(chat_id, question.id, positive, option)
        if positive:
            if question.type == "multipleChoice" and option >= 0:
                renderer.render_status(f"Selected: {question.options[option]}")
            else:
                labels = get_confirmation_labels(question.confirmation_type)
                renderer.render_status(f"Responded: {labels[0]}")
        else:
            labels = get_confirmation_labels(question.confirmation_type)
            renderer.render_status(f"Responded: {labels[1]}")
    except Exception as e:
        renderer.render_error(f"Failed to send response: {e}")
