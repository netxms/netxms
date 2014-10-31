package com.radensolutions.reporting.service;

import org.netxms.base.NXCPMessage;

import java.io.IOException;

public interface Connector {

    void start() throws IOException;

    void stop();

    void sendBroadcast(NXCPMessage message);
}
