package com.rfelements.exception;
/**
 * @author Pichanič Ján
 */
public class CollectorException extends Exception {

	public CollectorException() {
	}

	public CollectorException(String message) {
		super(message);
	}

	public CollectorException(String message, Throwable cause) {
		super(message, cause);
	}

	public CollectorException(Throwable cause) {
		super(cause);
	}

	public CollectorException(String message, Throwable cause, boolean enableSuppression, boolean writableStackTrace) {
		super(message, cause, enableSuppression, writableStackTrace);
	}
}
