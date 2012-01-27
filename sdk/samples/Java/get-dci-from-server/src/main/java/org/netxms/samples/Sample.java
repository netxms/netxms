package org.netxms.samples;

import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciValue;

import java.io.IOException;

public class Sample {
    private static final long NODE_ID = 11L;
    private static final String SERVER = "127.0.0.1";
    private static final String LOGIN = "admin";
    private static final String PASSWORD = "netxms";

    public static void main(final String[] args) throws NetXMSClientException, IOException {
        final NXCSession session = new NXCSession(SERVER, LOGIN, PASSWORD);
        session.connect();
        final DciValue[] lastValues = session.getLastValues(NODE_ID);
        for (final DciValue lastValue : lastValues) {
            System.out.printf("%s == %s%n", lastValue.getName(), lastValue.getValue());
        }
        session.disconnect();
    }

}