package com.radensolutions.reporting.infrastructure.impl;

import com.radensolutions.reporting.application.ReportingServer;
import com.radensolutions.reporting.application.ReportingServerFactory;
import com.radensolutions.reporting.application.Session;
import com.radensolutions.reporting.infrastructure.Connector;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.context.annotation.Scope;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;

/**
 * TODO: REFACTOR!!!
 */

@Scope(value = "singleton")
public class TcpConnector implements Connector {
    public static final int PORT = 4710;

    private static final Logger logger = LoggerFactory.getLogger(TcpConnector.class);

    private final List<SessionWorker> workers = new ArrayList<SessionWorker>();
    private ServerSocket serverSocket;
    private Thread listenerThread;

    /* (non-Javadoc)
     * @see com.radensolutions.reporting.infrastructure.Connector#start()
     */
    @Override
    public void start() throws IOException {
        serverSocket = new ServerSocket(PORT);
        listenerThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (!Thread.currentThread().isInterrupted()) {
                    try {
                        final Socket socket = serverSocket.accept();
                        final SessionWorker worker = new SessionWorker(socket);
                        synchronized (workers) {
                            workers.add(worker);
                        }
                        new Thread(worker).start();
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                }
            }
        });
        listenerThread.start();
    }

    /* (non-Javadoc)
     * @see com.radensolutions.reporting.infrastructure.Connector#stop()
     */
    @Override
    public void stop() {
        listenerThread.interrupt();
        for (SessionWorker worker : workers) {
            worker.stop();
        }
        try {
            listenerThread.join();
        } catch (InterruptedException e) {
            // ignore
        }
    }

    @Override
    public void sendBroadcast(NXCPMessage message) {
        synchronized (workers) {
            // TODO: in theory, could block accepting new connection while
            // sending message over stale connection
            for (Iterator<SessionWorker> iterator = workers.iterator(); iterator.hasNext(); ) {
                SessionWorker worker = iterator.next();
                if (worker.isActive()) {
                    worker.sendMessage(message);
                } else {
                    iterator.remove(); // remove failed connections
                }
            }
        }
    }

    /* (non-Javadoc)
     * @see java.lang.Object#toString()
     */
    @Override
    public String toString() {
        return "TcpConnector{" + "serverSocket=" + serverSocket + '}';
    }

    private class SessionWorker implements Runnable {
        final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(262144, 4194304);   // 256KB, 4MB
        private final Socket socket;
        private final Session session;
        private final BufferedInputStream inputStream;
        private final BufferedOutputStream outputStream;
        private boolean active = true;

        public SessionWorker(Socket socket) throws IOException {
            this.socket = socket;
            inputStream = new BufferedInputStream(socket.getInputStream());
            outputStream = new BufferedOutputStream(socket.getOutputStream());

            final ReportingServer app = ReportingServerFactory.getApp();
            session = app.getSession(TcpConnector.this);
        }

        /* (non-Javadoc)
         * @see java.lang.Runnable#run()
         */
        @Override
        public void run() {
            while (!Thread.currentThread().isInterrupted()) {
                try {
                    final NXCPMessage message = messageReceiver.receiveMessage(inputStream, null);
                    if (message != null) {
                        if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE) {
                            logger.debug("Request: " + message.toString());
                        }
                        ByteArrayOutputStream fileStream = new ByteArrayOutputStream();
                        final NXCPMessage reply = session.processMessage(message, fileStream);
                        if (reply != null) {
                            if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE) {
                                logger.debug("Reply: " + reply.toString());
                            }
                            sendMessage(reply);
                            if (fileStream.size() > 0) {
                                logger.debug("File data found, sending");
                                final byte[] bytes = fileStream.toByteArray();
                                sendFileData(message.getMessageId(), bytes);
                            }
                        }
                    }
                } catch (IOException e) {
                    logger.info("Communication error", e);
                    stop();
                } catch (NXCPException e) {
                    logger.info("Invalid message received", e);
                    stop();
                }
            }
        }

        public void stop() {
            Thread.currentThread().interrupt();
            active = false;
            try {
                socket.close();
            } catch (IOException e) {
                // ignore
            }
        }

        public boolean sendMessage(NXCPMessage message) {
            try {
                outputStream.write(message.createNXCPMessage());
                outputStream.flush();
                return true;
            } catch (IOException e) {
                logger.error("Communication failure", e);
                stop();
            }
            return false;
        }

        /**
         * @param requestId
         * @param data
         * @throws IOException
         */
        private void sendFileData(final long requestId, final byte[] data) throws IOException {
            NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
            msg.setBinaryMessage(true);

            for (int pos = 0; pos < data.length; pos += 16384) {
                int len = Math.min(16384, data.length - pos);
                msg.setBinaryData(Arrays.copyOfRange(data, pos, len));
                sendMessage(msg);
            }
        }

        private boolean isActive() {
            return active;
        }
    }
}
