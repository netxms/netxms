package com.radensolutions.jira.workflow;

import com.atlassian.jira.issue.MutableIssue;
import com.atlassian.jira.workflow.function.issue.AbstractJiraFunctionProvider;
import com.opensymphony.module.propertyset.PropertySet;
import com.opensymphony.workflow.WorkflowException;
import com.radensolutions.jira.NetxmsConnector;
import com.radensolutions.jira.SettingsManager;

import java.util.Map;

public class UpdateAlarmFunction extends AbstractJiraFunctionProvider {

    public static final String ACKNOWLEDGED = "acknowledged";
    public static final String RESOLVED = "resolved";
    public static final String TERMINATED = "terminated";
    private final NetxmsConnector connector;
    private SettingsManager settingsManager;

    public UpdateAlarmFunction(SettingsManager settingsManager) {
        this.settingsManager = settingsManager;
        if (settingsManager.isEnabled()) {
            connector = new NetxmsConnector(settingsManager);
        } else {
            connector = null;
        }
    }

    @Override
    public void execute(Map transientVars, Map args, PropertySet ps) throws WorkflowException {
        if (connector == null) {
            // Plugin is disabled
            return;
        }

        MutableIssue issue = getIssue(transientVars);

        if (!issue.getProjectObject().getKey().equalsIgnoreCase(settingsManager.getProjectKey())) {
            // invalid project
            return;
        }

        if (args.containsKey(UpdateAlarmFunctionFactory.KEY)) {
            String operation = ((String) args.get(UpdateAlarmFunctionFactory.KEY));
            String issueKey = issue.getKey();
            boolean ret = true;
            if (operation.equals(ACKNOWLEDGED)) {
                ret = connector.acknowledgeAlarm(issueKey);
            } else if (operation.equals(RESOLVED)) {
                ret = connector.resolveAlarm(issueKey);
            } else if (operation.equals(TERMINATED)) {
                ret = connector.terminateAlarm(issueKey);
            }

            if (!ret) {
                throw new WorkflowException("Unable to change NetXMS alarm state");
            }
        }
    }
}
