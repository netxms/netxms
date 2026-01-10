"""
Session and authentication management.
"""

from __future__ import annotations

import getpass
import json
import os
from datetime import datetime
from pathlib import Path
from typing import Any


def get_config_dir() -> Path:
    """
    Get the configuration directory path.

    Uses XDG_CONFIG_HOME on Linux, or ~/.config/nxai as fallback.

    Returns:
        Path to configuration directory
    """
    xdg_config = os.environ.get("XDG_CONFIG_HOME")
    if xdg_config:
        base = Path(xdg_config)
    else:
        base = Path.home() / ".config"

    config_dir = base / "nxai"
    config_dir.mkdir(parents=True, exist_ok=True)
    return config_dir


def get_sessions_file() -> Path:
    """Get path to sessions storage file."""
    return get_config_dir() / "sessions.json"


def _load_sessions() -> dict[str, Any]:
    """Load all sessions from storage."""
    sessions_file = get_sessions_file()
    if sessions_file.exists():
        try:
            return json.loads(sessions_file.read_text())
        except (json.JSONDecodeError, OSError):
            return {}
    return {}


def _save_sessions(sessions: dict[str, Any]) -> None:
    """Save all sessions to storage."""
    sessions_file = get_sessions_file()
    sessions_file.write_text(json.dumps(sessions, indent=2))
    # Set restrictive permissions (owner read/write only)
    sessions_file.chmod(0o600)


def load_session(server: str) -> dict[str, Any] | None:
    """
    Load saved session for a server.

    Args:
        server: Server hostname

    Returns:
        Session data dict with 'token' and 'expires' keys, or None if not found
    """
    sessions = _load_sessions()
    session = sessions.get(server)

    if session:
        # Check if token is expired
        expires = session.get("expires")
        if expires:
            try:
                expiry_time = datetime.fromisoformat(expires.replace("Z", "+00:00"))
                if expiry_time < datetime.now(expiry_time.tzinfo):
                    # Token expired, remove it
                    clear_session(server)
                    return None
            except (ValueError, TypeError):
                pass  # Can't parse expiry, assume valid

        return session

    return None


def save_session(server: str, token: str, expires: str | None = None) -> None:
    """
    Save session for a server.

    Args:
        server: Server hostname
        token: Authentication token
        expires: Optional expiration timestamp (ISO 8601)
    """
    sessions = _load_sessions()
    sessions[server] = {
        "token": token,
        "expires": expires,
        "saved_at": datetime.now().isoformat(),
    }
    _save_sessions(sessions)


def clear_session(server: str) -> None:
    """
    Clear saved session for a server.

    Args:
        server: Server hostname
    """
    sessions = _load_sessions()
    if server in sessions:
        del sessions[server]
        _save_sessions(sessions)


def clear_all_sessions() -> None:
    """Clear all saved sessions."""
    sessions_file = get_sessions_file()
    if sessions_file.exists():
        sessions_file.unlink()


def get_credentials(
    server: str,
    username: str | None = None,
    password: str | None = None,
    prompt_password: bool = True,
) -> tuple[str, str]:
    """
    Get credentials for a server, prompting if necessary.

    Priority order:
    1. Provided arguments
    2. Environment variables (NETXMS_USER, NETXMS_PASSWORD)
    3. Interactive prompt

    Args:
        server: Server hostname (for prompt display)
        username: Optional username
        password: Optional password
        prompt_password: Whether to prompt for password if not provided

    Returns:
        Tuple of (username, password)
    """
    # Get username
    if not username:
        username = os.environ.get("NETXMS_USER")
    if not username:
        username = input(f"Username for {server}: ")

    # Get password
    if not password:
        password = os.environ.get("NETXMS_PASSWORD")
    if not password and prompt_password:
        password = getpass.getpass(f"Password for {username}@{server}: ")

    return username, password or ""


def get_server(server: str | None = None) -> str:
    """
    Get server hostname.

    Args:
        server: Optional server from command line

    Returns:
        Server hostname

    Raises:
        ValueError: If no server specified
    """
    if server:
        return server

    server = os.environ.get("NETXMS_SERVER")
    if server:
        return server

    raise ValueError(
        "No server specified. Use --server or set NETXMS_SERVER environment variable."
    )
