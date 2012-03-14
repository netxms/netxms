package org.netxms.agent;

import org.netxms.base.NXCPMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;

public class PipeConnector implements Connector {

    private final Logger log = LoggerFactory.getLogger(PipeConnector.class);

    private volatile boolean shutdown;
    private Thread workerThread;
    private Worker worker;
    private MessageConsumer messageConsumer;

    private String pipeName;


    private class Worker implements Runnable {

        private RandomAccessFile pipe;
        private static final int MESSAGE_BUFFER_SIZE = 8192;

        @Override
        public void run() {
            while (!shutdown) {
                if (pipe == null) {
                    connect();
                }

                if (pipe != null) {
                    try {
                        if (pipe.length() > 0L) {
                            final byte[] bytes = new byte[MESSAGE_BUFFER_SIZE];
                            final int bytesRead = pipe.read(bytes);
                            log.debug("Got {} bytes from pipe", bytesRead);
                            if (bytesRead > 0) {
                                final NXCPMessage message = new NXCPMessage(bytes);
                                messageConsumer.processMessage(message);
                            }
                        } else {
                            sleep(100L);
                        }
                    } catch (IOException e) {
                        log.info("Cannot read from pipe");
                        pipe = null;
                    }
                }
            }

            if (pipe != null) {
                try {
                    pipe.close();
                } catch (IOException e) {
                    log.error("Cannot close pipe");
                }
            }
        }

        private void sleep(final long time) {
            try {
                Thread.sleep(time);
            } catch (InterruptedException e) {
                // ignore
            }
        }

        private void connect() {
            try {
                //noinspection IOResourceOpenedButNotSafelyClosed
                pipe = new RandomAccessFile(pipeName, "rw");
                log.debug("Pipe {} connected", pipeName);
            } catch (FileNotFoundException e) {
                pipe = null;
                log.info("Cannot open pipe {}, retrying", pipeName);
                sleep(1000L);
            }
        }

        public boolean sendMessage(final NXCPMessage message) {
            boolean ret = false;

            if (pipe != null) {
                try {
                    final byte[] data = message.createNXCPMessage();
                    pipe.write(data);
                    ret = true;
                } catch (IOException e) {
                    ret = false;
                    try {
                        pipe.close();
                    } catch (IOException e1) {
                        log.error("Cannot close pipe");
                    }
                    pipe = null;
                }
            }

            return ret;
        }
    }

    public PipeConnector(final String name) {
        pipeName = "\\\\\\\\.\\\\pipe\\\\nxagentd.subagent." + name;
    }

    @Override
    public void setMessageConsumer(final MessageConsumer consumer) {
        messageConsumer = consumer;
    }

    @Override
    public boolean sendMessage(final NXCPMessage message) {
        log.debug("Sending message {}, id={}", message.getMessageCode(), message.getMessageId());
        return worker.sendMessage(message);
    }

    @Override
    public void start() {
        worker = new Worker();
        workerThread = new Thread(worker);
        workerThread.start();
    }

    @Override
    public void stop() {
        shutdown = true;
        try {
            workerThread.join();
        } catch (InterruptedException e) {
            // ignore
        }
    }
}
