package com.radensolutions.jira;

import java.util.List;

public interface SettingsManager {
    List<String> getServers();

    void setServers(List<String> servers);

    String getLogin();

    void setLogin(String login);

    String getPassword();

    void setPassword(String password);

    boolean isEnabled();

    void setEnabled(boolean enabled);

    String getProjectKey();

    void setProjectKey(String projectKey);

    String getJiraAccount();

    void setJiraAccount(String name);
}
