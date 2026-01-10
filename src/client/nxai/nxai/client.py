"""
NetXMS REST API client for AI chat functionality.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import httpx


class NetXMSError(Exception):
    """Base exception for NetXMS client errors."""

    def __init__(self, message: str, status_code: int | None = None):
        super().__init__(message)
        self.status_code = status_code


class AuthenticationError(NetXMSError):
    """Authentication failed."""
    pass


class NotFoundError(NetXMSError):
    """Resource not found."""
    pass


class AccessDeniedError(NetXMSError):
    """Access denied to resource."""
    pass


@dataclass
class Question:
    """AI question requiring user response."""

    id: int
    type: str  # "confirmation" or "multipleChoice"
    confirmation_type: int  # 0=approve_reject, 1=yes_no, 2=confirm_cancel
    text: str
    context: str
    options: list[str]
    expires_at: str

    @classmethod
    def from_dict(cls, data: dict[str, Any]) -> Question:
        return cls(
            id=data["id"],
            type=data["type"],
            confirmation_type=data.get("confirmationType", 0),
            text=data["text"],
            context=data.get("context", ""),
            options=data.get("options", []),
            expires_at=data.get("expiresAt", ""),
        )


@dataclass
class ChatResponse:
    """Response from sending a message."""

    response: str | None
    pending_question: Question | None


class NetXMSClient:
    """REST API client for NetXMS WebAPI."""

    DEFAULT_TIMEOUT = 30.0
    MESSAGE_TIMEOUT = 120.0  # LLM responses can be slow

    def __init__(self, server: str, token: str | None = None, verify_ssl: bool = True):
        """
        Initialize the client.

        Args:
            server: Server hostname or URL (e.g., "netxms.example.com" or "https://netxms.example.com:8443")
            token: Optional authentication token
            verify_ssl: Whether to verify SSL certificates
        """
        # Normalize server URL
        if not server.startswith(("http://", "https://")):
            server = f"https://{server}"
        self.base_url = server.rstrip("/")
        self.token = token
        self.verify_ssl = verify_ssl
        self._client: httpx.Client | None = None

    @property
    def client(self) -> httpx.Client:
        """Get or create HTTP client."""
        if self._client is None:
            self._client = httpx.Client(
                base_url=self.base_url,
                verify=self.verify_ssl,
                timeout=self.DEFAULT_TIMEOUT,
            )
        return self._client

    def _headers(self) -> dict[str, str]:
        """Get request headers."""
        headers = {"Content-Type": "application/json"}
        if self.token:
            headers["Authorization"] = f"Bearer {self.token}"
        return headers

    def _handle_error(self, response: httpx.Response) -> None:
        """Handle error responses."""
        try:
            data = response.json()
            message = data.get("reason", data.get("error", response.text))
        except Exception:
            message = response.text or f"HTTP {response.status_code}"

        if response.status_code == 401:
            raise AuthenticationError(message, response.status_code)
        elif response.status_code == 403:
            raise AccessDeniedError(message, response.status_code)
        elif response.status_code == 404:
            raise NotFoundError(message, response.status_code)
        else:
            raise NetXMSError(message, response.status_code)

    def login(self, username: str, password: str) -> str:
        """
        Authenticate and get a session token.

        Args:
            username: Username
            password: Password

        Returns:
            Session token

        Raises:
            AuthenticationError: If authentication fails
        """
        response = self.client.post(
            "/v1/login",
            json={"username": username, "password": password},
            headers={"Content-Type": "application/json"},
        )

        if not response.is_success:
            self._handle_error(response)

        data = response.json()
        self.token = data.get("token") or data.get("sessionHandle")
        return self.token

    def create_chat(
        self, incident_id: int | None = None, object_id: int | None = None
    ) -> int:
        """
        Create a new AI chat session.

        Args:
            incident_id: Optional incident ID to bind chat to
            object_id: Optional object ID to bind chat to

        Returns:
            Chat ID

        Raises:
            NetXMSError: If chat creation fails
        """
        payload: dict[str, Any] = {}
        if incident_id:
            payload["incidentId"] = incident_id
        if object_id:
            payload["objectId"] = object_id

        response = self.client.post(
            "/v1/ai/chat",
            json=payload if payload else None,
            headers=self._headers(),
        )

        if not response.is_success:
            self._handle_error(response)

        return response.json()["chatId"]

    def send_message(
        self, chat_id: int, message: str, context: dict[str, Any] | None = None
    ) -> ChatResponse:
        """
        Send a message to the AI assistant.

        Args:
            chat_id: Chat session ID
            message: Message text
            context: Optional context update

        Returns:
            ChatResponse with AI response and optional pending question

        Raises:
            NetXMSError: If sending fails
        """
        payload: dict[str, Any] = {"message": message}
        if context:
            payload["context"] = context

        response = self.client.post(
            f"/v1/ai/chat/{chat_id}/message",
            json=payload,
            headers=self._headers(),
            timeout=self.MESSAGE_TIMEOUT,
        )

        if not response.is_success:
            self._handle_error(response)

        data = response.json()
        pending_question = None
        if data.get("pendingQuestion"):
            pending_question = Question.from_dict(data["pendingQuestion"])

        return ChatResponse(
            response=data.get("response"),
            pending_question=pending_question,
        )

    def poll_question(self, chat_id: int) -> Question | None:
        """
        Poll for pending question from AI.

        Args:
            chat_id: Chat session ID

        Returns:
            Question if one is pending, None otherwise
        """
        response = self.client.get(
            f"/v1/ai/chat/{chat_id}/question",
            headers=self._headers(),
        )

        if not response.is_success:
            self._handle_error(response)

        data = response.json()
        if data.get("question"):
            return Question.from_dict(data["question"])
        return None

    def answer_question(
        self, chat_id: int, question_id: int, positive: bool, option: int = -1
    ) -> None:
        """
        Answer a pending question.

        Args:
            chat_id: Chat session ID
            question_id: Question ID
            positive: True for positive response (approve/yes/confirm)
            option: Selected option index for multiple choice (-1 for confirmations)
        """
        response = self.client.post(
            f"/v1/ai/chat/{chat_id}/answer",
            json={
                "questionId": question_id,
                "positive": positive,
                "selectedOption": option,
            },
            headers=self._headers(),
        )

        if not response.is_success:
            self._handle_error(response)

    def clear_chat(self, chat_id: int) -> None:
        """
        Clear chat history.

        Args:
            chat_id: Chat session ID
        """
        response = self.client.post(
            f"/v1/ai/chat/{chat_id}/clear",
            headers=self._headers(),
        )

        if not response.is_success:
            self._handle_error(response)

    def delete_chat(self, chat_id: int) -> None:
        """
        Delete chat session.

        Args:
            chat_id: Chat session ID
        """
        response = self.client.delete(
            f"/v1/ai/chat/{chat_id}",
            headers=self._headers(),
        )

        if not response.is_success:
            self._handle_error(response)

    def find_object(self, name: str) -> dict[str, Any] | None:
        """
        Find an object by name.

        Args:
            name: Object name to search for

        Returns:
            Object data if found, None otherwise
        """
        response = self.client.post(
            "/v1/objects/search",
            json={"name": name},
            headers=self._headers(),
        )

        if not response.is_success:
            if response.status_code == 404:
                return None
            self._handle_error(response)

        data = response.json()
        objects = data if isinstance(data, list) else data.get("objects", [])
        if objects:
            return objects[0]
        return None

    def close(self) -> None:
        """Close the HTTP client."""
        if self._client:
            self._client.close()
            self._client = None

    def __enter__(self) -> NetXMSClient:
        return self

    def __exit__(self, *args: Any) -> None:
        self.close()
