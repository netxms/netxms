package com.radensolutions.reporting.infrastructure.impl;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

import org.netxms.base.CompatTools;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.Scope;

import com.radensolutions.reporting.application.ReportingServer;
import com.radensolutions.reporting.application.ReportingServerFactory;
import com.radensolutions.reporting.application.Session;
import com.radensolutions.reporting.infrastructure.Connector;

/**
 * TODO: REFACTOR!!!
 */

@Scope(value = "singleton")
public class TcpConnector implements Connector
{

	public static final int PORT = 4710;
	private static final int FILE_BUFFER_SIZE = 128 * 1024; // 128k
	private static final Logger logger = LoggerFactory.getLogger(TcpConnector.class);
	private final List<SessionWorker> workers = new ArrayList<SessionWorker>();
	private ServerSocket serverSocket;
	private Thread listenerThread;

	@Override
	public void start() throws IOException
	{
		serverSocket = new ServerSocket(PORT);
		listenerThread = new Thread(new Runnable()
		{
			@Override
			public void run()
			{
				while (!Thread.currentThread().isInterrupted())
				{
					try
					{
						final Socket socket = serverSocket.accept();
						final SessionWorker worker = new SessionWorker(socket);
						synchronized (workers)
						{
							workers.add(worker);
						}
						new Thread(worker).start();
					} catch (IOException e)
					{
						e.printStackTrace();
					}
				}
			}
		});
		listenerThread.start();
	}

	@Override
	public void stop()
	{
		listenerThread.interrupt();
		for (SessionWorker worker : workers)
		{
			worker.stop();
		}
		try
		{
			listenerThread.join();
		} catch (InterruptedException e)
		{
			// ignore
		}
	}

	@Override
	public void sendBroadcast(NXCPMessage message)
	{
		synchronized (workers)
		{
			// TODO: in theory, could block accepting new connection while
			// sending message over stale connection
			for (Iterator<SessionWorker> iterator = workers.iterator(); iterator.hasNext();)
			{
				SessionWorker worker = iterator.next();
				if (worker.isActive())
				{
					worker.sendMessage(message);
				}
				else
				{
					iterator.remove(); // remove failed connections
				}
			}
		}
	}

	@Override
	public String toString()
	{
		return "TcpConnector{" + "serverSocket=" + serverSocket + '}';
	}

	private class SessionWorker implements Runnable
	{
		public static final int BUFFER_SIZE = 4096;
		final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(BUFFER_SIZE);
		private final Socket socket;
		private final Session session;
		private final BufferedInputStream inputStream;
		private final BufferedOutputStream outputStream;
		private boolean active = true;

		public SessionWorker(Socket socket) throws IOException
		{
			this.socket = socket;
			inputStream = new BufferedInputStream(socket.getInputStream());
			outputStream = new BufferedOutputStream(socket.getOutputStream());

			final ReportingServer app = ReportingServerFactory.getApp();
			session = app.getSession(TcpConnector.this);
		}

		@Override
		public void run()
		{
			while (!Thread.currentThread().isInterrupted())
			{
				try
				{
					final NXCPMessage message = messageReceiver.receiveMessage(inputStream, null);
					if (message != null)
					{
						if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
						{
							logger.debug("Request: " + message.toString());
						}
						ByteArrayOutputStream fileStream = new ByteArrayOutputStream();
						final NXCPMessage reply = session.processMessage(message, fileStream);
						if (reply != null)
						{
							if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE)
							{
								logger.debug("Reply: " + reply.toString());
							}
							sendMessage(reply);
							if (fileStream.size() > 0)
							{
								logger.debug("File data found, sending");
								final byte[] bytes = fileStream.toByteArray();
								sendFileData(message.getMessageId(), bytes);
							}
						}
					}
				} catch (IOException e)
				{
					logger.info("Communication error", e);
					stop();
				} catch (NXCPException e)
				{
					logger.info("Invalid message received", e);
					stop();
				}
			}
		}

		public void stop()
		{
			Thread.currentThread().interrupt();
			active = false;
			try
			{
				socket.close();
			} catch (IOException e)
			{
				// ignore
			}
		}

		public boolean sendMessage(NXCPMessage message)
		{
			try
			{
				outputStream.write(message.createNXCPMessage());
				outputStream.flush();
				return true;
			} catch (IOException e)
			{
				logger.error("Communication failure", e);
				stop();
			}
			return false;
		}

		private void sendFileData(final long requestId, final byte[] data) throws IOException
		{
			NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
			msg.setBinaryMessage(true);

			boolean success = false;
			final byte[] buffer = new byte[FILE_BUFFER_SIZE];
			long bytesSent = 0;
			final ByteArrayInputStream inputStream = new ByteArrayInputStream(data);
			while (true)
			{
				final int bytesRead = inputStream.read(buffer);
				if (bytesRead < FILE_BUFFER_SIZE)
				{
					msg.setEndOfFile(true);
				}

				msg.setBinaryData(CompatTools.arrayCopy(buffer, bytesRead));
				sendMessage(msg);

				if (bytesRead < FILE_BUFFER_SIZE)
				{
					success = true;
					break;
				}
			}

//			if (!success)
//			{
//				NXCPMessage abortMessage = new NXCPMessage(NXCPCodes.CMD_ABORT_FILE_TRANSFER, requestId);
//				abortMessage.setBinaryMessage(true);
//				sendMessage(abortMessage);
//				waitForRCC(abortMessage.getMessageId());
//			}
		}

		private boolean isActive()
		{
			return active;
		}
	}
}
