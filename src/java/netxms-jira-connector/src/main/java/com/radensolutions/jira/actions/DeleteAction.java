package com.radensolutions.jira.actions;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

import java.io.IOException;

public class DeleteAction implements ConnectorAction {

    private String key;

    public DeleteAction(String key) {
        this.key = key;
    }

    @Override
    public void execute(NXCSession session) throws IOException, NXCException {
        session.unlinkHelpdeskIssue(key);
    }
}
