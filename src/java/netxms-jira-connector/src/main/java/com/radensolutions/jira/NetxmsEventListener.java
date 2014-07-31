package com.radensolutions.jira;

import com.atlassian.event.api.EventListener;
import com.atlassian.event.api.EventPublisher;
import com.atlassian.jira.event.issue.IssueEvent;
import com.atlassian.jira.event.type.EventType;
import com.atlassian.jira.issue.Issue;
import com.atlassian.jira.issue.comments.Comment;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.DisposableBean;
import org.springframework.beans.factory.InitializingBean;

public class NetxmsEventListener implements InitializingBean, DisposableBean {

    private static final Logger log = LoggerFactory.getLogger(NetxmsEventListener.class);

    private final EventPublisher eventPublisher;
    private final NetxmsConnector connector;
    private final SettingsManager settingsManager;

    public NetxmsEventListener(EventPublisher eventPublisher, SettingsManager settingsManager) {
        this.eventPublisher = eventPublisher;
        this.settingsManager = settingsManager;
        connector = new NetxmsConnector(settingsManager);
    }

    @Override
    public void destroy() throws Exception {
        log.debug("Unregister listener");
        eventPublisher.unregister(this);
    }

    @Override
    public void afterPropertiesSet() throws Exception {
        log.debug("Register listener");
        eventPublisher.register(this);
    }

    @EventListener
    public void onIssueEvent(IssueEvent issueEvent) {
        if (!settingsManager.isEnabled()) {
            log.debug("Plugin is disabled, ignoring event");
            return;
        }

        Long eventTypeId = issueEvent.getEventTypeId();
        Issue issue = issueEvent.getIssue();

        log.debug("Issue {} event: {}", issue.getId(), eventTypeId);

        String name = issueEvent.getUser().getName();
        if (name.equalsIgnoreCase(settingsManager.getJiraAccount())) {
            return;
        }

        String issueProjectKey = issueEvent.getProject().getKey();
        String configuredProjectKey = settingsManager.getProjectKey();
        if (issueProjectKey.equalsIgnoreCase(configuredProjectKey)) {
            log.debug("Project key matched");
            String comment = getCommentText(issueEvent.getComment());

            if (eventTypeId.equals(EventType.ISSUE_COMMENTED_ID) || eventTypeId.equals(EventType.ISSUE_COMMENT_EDITED_ID)) {
                connector.commentOnAlarm(issue.getKey(), comment);
            } else if (eventTypeId.equals(EventType.ISSUE_DELETED_ID)) {
                connector.removeHelpdeskReference(issue.getKey());
            }
        } else {
            log.debug("Issue project key ({}) do not match configured ({}", issueProjectKey, configuredProjectKey);
        }
    }

    private String getCommentText(Comment comment) {
        if (comment != null) {
            return comment.getAuthor() + ": " + comment.getBody();
        }
        return null;
    }
}
