package com.radensolutions.reporting.service;

import org.netxms.base.NXCPMessage;

public interface Session {
    /**
     * Process incoming NXCP message and generate reply
     *
     * @param message    input message
     * @param fileStream output stream for outgoing file
     * @return reply
     */
    public MessageProcessingResult processMessage(NXCPMessage message);

    /**
     * Send NXCP message to shserver
     *
     * @param code
     * @param data
     */
    public void sendNotify(int code, int data);
}
