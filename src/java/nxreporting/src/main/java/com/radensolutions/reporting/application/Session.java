package com.radensolutions.reporting.application;

import org.netxms.base.NXCPMessage;

import java.io.OutputStream;

public interface Session
{
    /**
     * Process incoming NXCP message and generate reply
     *
     * @param message    input message
     * @param fileStream output stream for outgoing file
     * @return reply
     */
    public NXCPMessage processMessage(NXCPMessage message, OutputStream fileStream);

    /**
     * Send NXCP message to shserver
     * 
     * @param code
     * @param data
     */
    public void sendNotify(int code, int data);
}
