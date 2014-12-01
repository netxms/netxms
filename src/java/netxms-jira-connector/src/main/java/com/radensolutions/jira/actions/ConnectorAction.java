package com.radensolutions.jira.actions;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

import java.io.IOException;

public interface ConnectorAction {
    void execute(NXCSession session) throws IOException, NXCException;
}
