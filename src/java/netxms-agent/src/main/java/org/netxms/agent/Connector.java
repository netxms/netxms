package org.netxms.agent;

import org.netxms.base.NXCPMessage;

public interface Connector {

    void setMessageConsumer(MessageConsumer consumer);

    boolean sendMessage(NXCPMessage message);

    void start();

    void stop();
}
