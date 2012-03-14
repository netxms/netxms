package org.netxms.agent;

import org.netxms.base.NXCPMessage;

public interface MessageConsumer {

    void processMessage(NXCPMessage message);

}
