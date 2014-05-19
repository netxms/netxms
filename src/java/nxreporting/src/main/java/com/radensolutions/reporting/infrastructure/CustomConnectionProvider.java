package com.radensolutions.reporting.infrastructure;

import org.netxms.base.EncryptedPassword;
import org.quartz.SchedulerException;
import org.quartz.utils.ConnectionProvider;
import org.quartz.utils.PoolingConnectionProvider;

import java.io.IOException;
import java.security.NoSuchAlgorithmException;
import java.sql.Connection;
import java.sql.SQLException;
import java.util.Properties;

public class CustomConnectionProvider implements ConnectionProvider {

    private String driver;
    private String URL;
    private String user;
    private String password;
    private String encryptedPassword;
    private int maxConnections;
    private String validationQuery;
    private int idleConnectionValidationSeconds;
    private String validateOnCheckout;
    private int discardIdleConnectionsSeconds;

    private PoolingConnectionProvider connectionProvider;

    @Override
    public Connection getConnection() throws SQLException {
        if (connectionProvider == null) {
            Properties properties = new Properties();
            if (driver != null) properties.setProperty(PoolingConnectionProvider.DB_DRIVER, driver);
            if (URL != null) properties.setProperty(PoolingConnectionProvider.DB_URL, URL);
            if (user != null) properties.setProperty(PoolingConnectionProvider.DB_USER, user);
            if (password != null) properties.setProperty(PoolingConnectionProvider.DB_PASSWORD, password);
            if (encryptedPassword != null) {
                try {
                    properties.setProperty(PoolingConnectionProvider.DB_PASSWORD, EncryptedPassword.decrypt(user, encryptedPassword));
                } catch (NoSuchAlgorithmException e) {
                    throw new SQLException("Can't decrypt password", e);
                } catch (IOException e) {
                    throw new SQLException("Can't decrypt password", e);
                }
            }
            if (maxConnections != 0)
                properties.setProperty(PoolingConnectionProvider.DB_MAX_CONNECTIONS, Integer.toString(maxConnections));
            if (validationQuery != null)
                properties.setProperty(PoolingConnectionProvider.DB_VALIDATION_QUERY, validationQuery);
            if (idleConnectionValidationSeconds != 0)
                properties.setProperty(PoolingConnectionProvider.DB_IDLE_VALIDATION_SECONDS, Integer.toString(idleConnectionValidationSeconds));
            if (validateOnCheckout != null)
                properties.setProperty(PoolingConnectionProvider.DB_VALIDATE_ON_CHECKOUT, validateOnCheckout);
            if (discardIdleConnectionsSeconds != 0) {
                // PoolingConnectionProvider.DB_DISCARD_IDLE_CONNECTIONS_SECONDS is private for unknown reason
                properties.setProperty("discardIdleConnectionsSeconds", Integer.toString(discardIdleConnectionsSeconds));
            }
            try {
                connectionProvider = new PoolingConnectionProvider(properties);
            } catch (SchedulerException e) {
                throw new SQLException("Can't create PoolingConnectionProvider", e);
            }
        }
        return connectionProvider.getConnection();
    }

    @Override
    public void initialize() throws SQLException {
    }

    @Override
    public void shutdown() throws SQLException {
        if (connectionProvider != null) {
            connectionProvider.shutdown();
            connectionProvider = null;
        }
    }

    public void setDriver(String driver) {
        this.driver = driver;
    }

    public void setURL(String URL) {
        this.URL = URL;
    }

    public void setUser(String user) {
        this.user = user;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public void setEncryptedPassword(String encryptedPassword) {
        this.encryptedPassword = encryptedPassword;
    }

    public void setMaxConnections(int maxConnections) {
        this.maxConnections = maxConnections;
    }

    public void setValidationQuery(String validationQuery) {
        this.validationQuery = validationQuery;
    }

    public void setIdleConnectionValidationSeconds(int idleConnectionValidationSeconds) {
        this.idleConnectionValidationSeconds = idleConnectionValidationSeconds;
    }

    public void setValidateOnCheckout(String validateOnCheckout) {
        this.validateOnCheckout = validateOnCheckout;
    }

    public void setDiscardIdleConnectionsSeconds(int discardIdleConnectionsSeconds) {
        this.discardIdleConnectionsSeconds = discardIdleConnectionsSeconds;
    }
}
