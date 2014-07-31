package com.radensolutions.jira.actions;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;

import java.io.IOException;

public class CommentAction implements ConnectorAction {

    private final String key;
    private final String comment;

    public CommentAction(String key, String comment) {
        this.key = key;
        this.comment = comment;
    }

    @Override
    public void execute(NXCSession session) throws IOException, NXCException {
        session.createAlarmComment(key, comment);
    }
}
