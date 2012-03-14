package org.netxms.agent.transport;

import org.netxms.agent.Connector;
import org.netxms.agent.MessageConsumer;
import org.netxms.base.NXCPMessage;

public class SocketConnector implements Connector {

    public SocketConnector(final int port) {
    }

    @Override
    public void setMessageConsumer(final MessageConsumer consumer) {
    }

    @Override
    public boolean sendMessage(final NXCPMessage message) {
        return false;
    }

    @Override
    public void start() {
    }

    @Override
    public void stop() {
    }
}
