package com.radensolutions.jira.actions;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

import java.io.IOException;

public class AcknowledgeAction implements ConnectorAction {
    private String key;

    public AcknowledgeAction(String key) {
        this.key = key;
    }

    @Override
    public void execute(NXCSession session) throws IOException, NXCException {
        session.acknowledgeAlarm(key);
    }
}
