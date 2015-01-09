package com.radensolutions.jira;

import com.radensolutions.jira.actions.*;
import org.netxms.api.client.NetXMSClientException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.util.List;

public class NetxmsConnector {

    private static final Logger log = LoggerFactory.getLogger(NetxmsConnector.class);

    private final SettingsManager settingsManager;

    public NetxmsConnector(SettingsManager settingsManager) {
        this.settingsManager = settingsManager;
    }

    public void commentOnAlarm(String key, String comment) {
        if (comment != null) {
            executeAction(new CommentAction(key, comment));
        }
    }

    public boolean acknowledgeAlarm(String key) {
        return executeAction(new AcknowledgeAction(key));
    }

    public boolean resolveAlarm(String key) {
        return executeAction(new ResolveAction(key));
    }

    public boolean terminateAlarm(String key) {
        return executeAction(new TerminateAction(key));
    }

    public boolean removeHelpdeskReference(String key) {
        return executeAction(new DeleteAction(key));
    }

    private boolean executeAction(ConnectorAction action) {
        List<String> servers = settingsManager.getServers();
        String login = settingsManager.getLogin();
        String password = settingsManager.getPassword();
        boolean success = false;
        for (String server : servers) {
            ConnectionDetails connectionDetails = new ConnectionDetails(server).parse();
            String connAddress = connectionDetails.getAddress();
            int connPort = connectionDetails.getPort();
            NXCSession session = new NXCSession(connAddress, connPort, login, password);
            session.setIgnoreProtocolVersion(true);

            try {
                session.connect();
                log.debug("Connected to " + server);
                // do stuff
                action.execute(session);
                success = true;
                log.debug("Action executed on " + server);
            } catch (IOException e) {
                log.error("Connection failed", e);
            } catch (NetXMSClientException e) {
                if (e.getErrorCode() != RCC.INVALID_ALARM_ID) {
                    log.error("Connection failed", e);
                }
            }
            if (success) {
                break;
            }
        }
        return success;
    }

    public boolean testConnection(String server, String login, String password, StringBuffer errorMessage) {
        ConnectionDetails connectionDetails = new ConnectionDetails(server).parse();
        NXCSession session = new NXCSession(connectionDetails.getAddress(), connectionDetails.getPort(), login, password);
        boolean ret = false;
        try {
            session.connect();
            ret = true;
        } catch (IOException e) {
            errorMessage.append(e.getMessage());
        } catch (NetXMSClientException e) {
            errorMessage.append(e.getMessage());
        }
        return ret;
    }

    private class ConnectionDetails {
        private String connectionString;
        private String address;
        private int port;

        public ConnectionDetails(String connectionString) {
            this.connectionString = connectionString;
        }

        public String getAddress() {
            return address;
        }

        public int getPort() {
            return port;
        }

        public ConnectionDetails parse() {
            String[] split = connectionString.split(":");
            address = split[0];
            port = split.length > 1 ? new Integer(split[1]) : NXCSession.DEFAULT_CONN_PORT;
            return this;
        }
    }
}
