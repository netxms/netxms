package com.radensolutions.reporting.service.impl;

import com.radensolutions.reporting.service.Connector;
import org.netxms.base.NXCPMessage;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.context.ApplicationContext;
import org.springframework.stereotype.Service;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

/**
 * TODO: REFACTOR!!!
 */

@Service
public class TcpConnector implements Connector {
    public static final int PORT = 4710;

    private static final Logger logger = LoggerFactory.getLogger(TcpConnector.class);

    private final List<SessionWorker> workers = new ArrayList<SessionWorker>();
    private ServerSocket serverSocket;
    private Thread listenerThread;

    @Autowired
    private ApplicationContext context; // TODO: remove it

    @Override
    public void start() throws IOException {
        serverSocket = new ServerSocket(PORT);
        listenerThread = new Thread(new Runnable() {
            @Override
            public void run() {
                while (!Thread.currentThread().isInterrupted()) {
                    try {
                        final Socket socket = serverSocket.accept();
                        SessionWorker worker = context.getBean(SessionWorker.class, socket);
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
            // TODO: in theory, could block accepting new connection while sending message over stale connection
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

    @Override
    public String toString() {
        return "TcpConnector{" + "serverSocket=" + serverSocket + '}';
    }

}
