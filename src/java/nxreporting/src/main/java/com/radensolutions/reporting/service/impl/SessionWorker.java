package com.radensolutions.reporting.service.impl;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;
import org.netxms.base.CompatTools;
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
import com.radensolutions.reporting.service.MessageProcessingResult;
import com.radensolutions.reporting.service.Session;

@Component
@Scope(BeanDefinition.SCOPE_PROTOTYPE)
class SessionWorker implements Runnable {
    private static final Logger log = LoggerFactory.getLogger(SessionWorker.class);

    private static final int FILE_BUFFER_SIZE = 128 * 1024; // 128k

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
                    final MessageProcessingResult result = session.processMessage(message);
                    if (result.response != null) {
                        if (message.getMessageCode() != NXCPCodes.CMD_KEEPALIVE) {
                            log.debug("Reply: " + result.response.toString());
                        }
                        sendMessage(result.response);
                        if (result.file != null) {
                            log.debug("File data found, sending");
                            FileInputStream s = null;
                            try
                            {
                               s = new FileInputStream(result.file);
                               sendFileStream(message.getMessageId(), s);
                            }
                            catch(IOException e)
                            {
                               log.error("Unexpected I/O exception while sending rendered file");
                            }
                            if (s != null)
                               s.close();
                            result.file.delete();
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
    * 
    * @param requestId
    * @param inputStream
    * @throws IOException
    */
    public void sendFileStream(final long requestId, final InputStream inputStream) throws IOException
    {
       NXCPMessage msg = new NXCPMessage(NXCPCodes.CMD_FILE_DATA, requestId);
       msg.setBinaryMessage(true);

       boolean success = false;
       final byte[] buffer = new byte[FILE_BUFFER_SIZE];
       while(true)
       {         
          final int bytesRead = inputStream.read(buffer);
          if (bytesRead < FILE_BUFFER_SIZE)
          {
             msg.setEndOfFile(true);
          }

          msg.setBinaryData(bytesRead  == -1 ? new byte[0] : CompatTools.arrayCopy(buffer, bytesRead));
          sendMessage(msg);

          if (bytesRead < FILE_BUFFER_SIZE)
          {
             success = true;
             break;
          }
       }

       if (!success)
       {
          NXCPMessage abortMessage = new NXCPMessage(NXCPCodes.CMD_ABORT_FILE_TRANSFER, requestId);
          abortMessage.setBinaryMessage(true);
          sendMessage(abortMessage);
       }
    }

    /**
    * @return
    */
   boolean isActive() 
    {
        return active;
    }
}
