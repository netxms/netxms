package com.radensolutions.reporting.service;

import org.netxms.base.NXCPMessage;

public interface Session {

    /**
     * Process incoming NXCP message and generate reply
     *
     * @param message    input message
     * @return reply
     */
    MessageProcessingResult processMessage(NXCPMessage message);

    /**
     * Send NXCP message to server
     *
     * @param code
     * @param data
     */
    void sendNotify(int code, int data);
}
