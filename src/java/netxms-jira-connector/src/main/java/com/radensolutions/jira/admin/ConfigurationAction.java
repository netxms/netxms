package com.radensolutions.jira.admin;

import com.atlassian.jira.security.xsrf.RequiresXsrfCheck;
import com.atlassian.jira.web.action.JiraWebActionSupport;
import com.radensolutions.jira.NetxmsConnector;
import com.radensolutions.jira.SettingsManager;

import java.util.ArrayList;
import java.util.List;

public class ConfigurationAction extends JiraWebActionSupport {

    private SettingsManager settingsManager;
    private String login;
    private String password = "";
    private String server1 = "";
    private String server2 = "";
    private String server3 = "";
    private String forced;
    private String enabled;
    private String projectKey = "";
    private String jiraAccount = "";

    private boolean success;

    public String getLogin() {
        return login;
    }

    public void setLogin(String login) {
        this.login = login;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public String getServer1() {
        return server1;
    }

    public void setServer1(String server) {
        this.server1 = server;
    }

    public String getServer2() {
        return server2;
    }

    public void setServer2(String server) {
        this.server2 = server;
    }

    public String getServer3() {
        return server3;
    }

    public void setServer3(String server) {
        this.server3 = server;
    }

    public String getEnabled() {
        return enabled;
    }

    public void setEnabled(String enabled) {
        this.enabled = enabled;
    }

    public void setForced(String forced) {
        this.forced = forced;
    }

    public void setSuccess(boolean success) {
        this.success = success;
    }

    public String getProjectKey() {
        return projectKey;
    }

    public void setProjectKey(String projectKey) {
        this.projectKey = projectKey;
    }

    public String getJiraAccount() {
        return jiraAccount;
    }

    public void setJiraAccount(String jiraAccount) {
        this.jiraAccount = jiraAccount;
    }

    public ConfigurationAction(SettingsManager settingsManager) {
        this.settingsManager = settingsManager;
        List<String> servers = settingsManager.getServers();
        if (servers != null) {
            switch (servers.size()) {
                case 3:
                    server3 = servers.get(2);
                case 2:
                    server2 = servers.get(1);
                case 1:
                    server1 = servers.get(0);
            }
        }
        login = settingsManager.getLogin();
        password = settingsManager.getPassword();
        enabled = settingsManager.isEnabled() ? "on" : "off";
        projectKey = settingsManager.getProjectKey();
        jiraAccount = settingsManager.getJiraAccount();
    }

    @Override
    protected void doValidation() {
        if ("on".equals(forced)) {
            // ignore configuration check
            forced = "";
            return;
        }

        if (projectKey.isEmpty()) {
            addErrorMessage("Project Key is not set");
        }

        if (server1.isEmpty() && server2.isEmpty() && server3.isEmpty()) {
            addErrorMessage("Server is not set");
            return;
        }
        if (login.isEmpty()) {
            addErrorMessage("Login is not set");
        }
        if (jiraAccount.isEmpty()) {
            addErrorMessage("Jira Account is not set");
        }

        if (enabled.equals("on")) {
            checkConnection(server1);
            checkConnection(server2);
            checkConnection(server3);
        }
    }

    private void checkConnection(String server) {
        if (!server.isEmpty()) {
            StringBuffer error = new StringBuffer();
            if (!new NetxmsConnector(null).testConnection(server, login, password, error)) {
                addErrorMessage("Connection attempt to " + server + " failed: " + error.toString());
            }
        }
    }

    @Override
    @RequiresXsrfCheck
    protected String doExecute() throws Exception {
        ArrayList<String> servers = new ArrayList<String>();
        if (!server1.isEmpty()) {
            servers.add(server1);
        }
        if (!server2.isEmpty()) {
            servers.add(server2);
        }
        if (!server3.isEmpty()) {
            servers.add(server3);
        }
        settingsManager.setServers(servers);
        settingsManager.setLogin(login);
        settingsManager.setPassword(password);
        settingsManager.setEnabled(enabled.equals("on"));
        settingsManager.setProjectKey(projectKey);
        settingsManager.setJiraAccount(jiraAccount);
        success = true;
        return super.doDefault();
    }

    @SuppressWarnings("UnusedDeclaration")
    public boolean isSuccess() {
        return success;
    }
}
