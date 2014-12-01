package com.radensolutions.jira.actions;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

import java.io.IOException;

public class ResolveAction implements ConnectorAction {
    private String key;

    public ResolveAction(String key) {
        this.key = key;
    }

    @Override
    public void execute(NXCSession session) throws IOException, NXCException {
        session.resolveAlarm(key);
    }
}
