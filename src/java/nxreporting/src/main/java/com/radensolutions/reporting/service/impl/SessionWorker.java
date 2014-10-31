package com.radensolutions.reporting.service.impl;

import com.radensolutions.reporting.service.Session;
import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPException;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCPMessageReceiver;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.context.annotation.Scope;
import org.springframework.stereotype.Component;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.util.Arrays;

@Component
@Scope(BeanDefinition.SCOPE_PROTOTYPE)
class SessionWorker implements Runnable {
    private static final Logger log = LoggerFactory.getLogger(SessionWorker.class);

    final NXCPMessageReceiver messageReceiver = new NXCPMessageReceiver(262144, 4194304);   // 256KB, 4MB
    private final Socket socket;
    private final BufferedInputStream inputStream;
    private final BufferedOutputStream outputStream;
    @Autowired
    private Session session;
    private boolean active = true;

    public SessionWorker(Socket socket) throws IOException {
        this.socket = socket;
        inputStream = new BufferedInputStream(socket.getInputStream());
        outputStream = new BufferedOutputStream(socket.getOutputStream());
    }

    @Override
    public void run() {
        while (!Thread.currentThread().isInterrupted()) {
            try {
                final NXCPMessage message = messageReceiver.receiveMessage(inputStream, null);
                if (message != null) {
                    if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE) {
                        log.debug("Request: " + message.toString());
                    }
                    ByteArrayOutputStream fileStream = new ByteArrayOutputStream();
                    final NXCPMessage reply = session.processMessage(message, fileStream);
                    if (reply != null) {
                        if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE) {
                            log.debug("Reply: " + reply.toString());
                        }
                        sendMessage(reply);
                        if (fileStream.size() > 0) {
                            log.debug("File data found, sending");
                            final byte[] bytes = fileStream.toByteArray();
                            sendFileData(message.getMessageId(), bytes);
                        }
                    }
                }
            } catch (IOException e) {
                log.info("Communication error", e);
                stop();
            } catch (NXCPException e) {
                log.info("Invalid message received", e);
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
            log.error("Communication failure", e);
            stop();
        }
        return false;
    }

    /**
     * @param requestId
     * @param data
     * @throws java.io.IOException
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

    boolean isActive() {
        return active;
    }
}
