package com.radensolutions.jira;

import com.atlassian.sal.api.pluginsettings.PluginSettings;
import com.atlassian.sal.api.pluginsettings.PluginSettingsFactory;

import java.util.ArrayList;
import java.util.List;

public class SettingsManagerImpl implements SettingsManager {
    private static final String NAMESPACE = "netxms-";
    public static final String KEY_PASSWORD = NAMESPACE + "password";
    public static final String KEY_LOGIN = NAMESPACE + "login";
    public static final String KEY_SERVERS = NAMESPACE + "servers";
    public static final String KEY_ENABLED = NAMESPACE + "enabled";
    public static final String KEY_PROJECT = NAMESPACE + "project";
    public static final String KEY_JIRA_ACCOUNT = NAMESPACE + "jira-account";

    private final PluginSettingsFactory pluginSettingsFactory;

    public SettingsManagerImpl(PluginSettingsFactory pluginSettingsFactory) {
        this.pluginSettingsFactory = pluginSettingsFactory;
    }

    @Override
    public List<String> getServers() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        Object o = globalSettings.get(KEY_SERVERS);
        return o == null ? new ArrayList<String>(0) : (List<String>) o;
    }

    @Override
    public void setServers(List<String> servers) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_SERVERS, servers);
    }

    @Override
    public String getLogin() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        return (String) globalSettings.get(KEY_LOGIN);
    }

    @Override
    public void setLogin(String login) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_LOGIN, login);
    }

    @Override
    public String getPassword() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        return (String) globalSettings.get(KEY_PASSWORD);
    }

    @Override
    public void setPassword(String password) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_PASSWORD, password);
    }

    @Override
    public boolean isEnabled() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        String enabled = (String) globalSettings.get(KEY_ENABLED);
        return enabled != null && enabled.equals("YES");
    }

    @Override
    public void setEnabled(boolean enabled) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_ENABLED, enabled ? "YES" : "NO");
    }

    @Override
    public String getProjectKey() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        return (String) globalSettings.get(KEY_PROJECT);
    }

    @Override
    public void setProjectKey(String projectKey) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_PROJECT, projectKey);
    }

    @Override
    public String getJiraAccount() {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        return (String) globalSettings.get(KEY_JIRA_ACCOUNT);
    }

    @Override
    public void setJiraAccount(String jiraAccount) {
        PluginSettings globalSettings = pluginSettingsFactory.createGlobalSettings();
        globalSettings.put(KEY_JIRA_ACCOUNT, jiraAccount);
    }
}
