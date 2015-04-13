package com.radensolutions.reporting.service;

import org.apache.commons.configuration.ConfigurationException;
import org.apache.commons.configuration.HierarchicalConfiguration;
import org.apache.commons.configuration.HierarchicalINIConfiguration;
import org.apache.commons.configuration.XMLConfiguration;
import org.netxms.base.EncryptedPassword;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.stereotype.Repository;

import java.util.*;

@Repository
public class ServerSettings {

    public static final String DC_ID_SYSTEM = "system";
    public static final String DC_ID_SCHEDULER = "scheduler";
    public static final String DC_ID_REPORTING = "report";
    private static final Logger log = LoggerFactory.getLogger(ServerSettings.class);
    private final HierarchicalConfiguration configuration;
    private final Map<String, DataSourceConfig> dataSources = new HashMap<String, DataSourceConfig>();

    public ServerSettings() throws ConfigurationException {
        this("nxreporting.xml");
    }

    public ServerSettings(String filename) throws ConfigurationException {
        configuration = new XMLConfiguration(filename);

        loadServerConfig();
        loadDataSources();

        if (dataSources.size() > 0 && !dataSources.containsKey(DC_ID_SYSTEM)) {
            dataSources.put(DC_ID_SCHEDULER, dataSources.values().iterator().next());
        }
        DataSourceConfig systemConfig = dataSources.get(DC_ID_SYSTEM);
        if (!dataSources.containsKey(DC_ID_SCHEDULER)) {
            dataSources.put(DC_ID_SCHEDULER, systemConfig);
        }
        if (!dataSources.containsKey(DC_ID_REPORTING)) {
            dataSources.put(DC_ID_REPORTING, systemConfig);
        }
    }

    private void loadServerConfig() {
        String filename = configuration.getString("netxmsdConfig");
        HierarchicalINIConfiguration netxmsConfig;
        try {
            netxmsConfig = new HierarchicalINIConfiguration(filename);
        } catch (ConfigurationException e) {
            // ignore errors and stop processing
            return;
        }

        String driver = netxmsConfig.getString("DBDriver");
        String server = netxmsConfig.getString("DBServer");
        String name = netxmsConfig.getString("DBName");
        String login = netxmsConfig.getString("DBLogin");
        String password = netxmsConfig.getString("DBPassword");

        DatabaseType type = lookupType(driver);
        if (type != null && name != null && login != null && password != null) {
            String url = null;
            switch (type) {
                case POSTGRESQL:
                    url = "jdbc:postgresql://" + server + "/" + name;
                    break;
                case ORACLE:
                    url = "jdbc:oracle:thin:@" + server + ":1521:" + name;
                    break;
                case MSSQL:
                    url = "jdbc:sqlserver://" + server + ";DatabaseName=" + name;
                    break;
                case MYSQL:
                    url = "jdbc:mysql://" + server + "/" + name;
                    break;
                case INFORMIX:
                    url = "jdbc:informix-sqli://" + server + "/" + name;
                    break;
            }
            DataSourceConfig config = new DataSourceConfig(type, url, login, password);
            dataSources.put(DC_ID_SYSTEM, config);
            dataSources.put(DC_ID_SCHEDULER, config);
            dataSources.put(DC_ID_REPORTING, config);
        }
    }

    private void loadDataSources() {
        List<HierarchicalConfiguration> dataSources = configuration.configurationsAt("datasources.datasource");
        for (HierarchicalConfiguration dataSource : dataSources) {
            String id = dataSource.getString("id", "system");
            String type = dataSource.getString("type");

            DatabaseType databaseType = lookupType(type);
            if (databaseType != null) {
                String dialect = dataSource.getString("dialect", databaseType.getDialect());
                String driver = dataSource.getString("driverClassName", databaseType.getDriver());
                String url = dataSource.getString("url");
                String username = dataSource.getString("username");
                String password = dataSource.getString("password");
                String encryptedPassword = dataSource.getString("encryptedPassword");
                if (username != null && encryptedPassword != null) {
                    try {
                        password = EncryptedPassword.decrypt(username, encryptedPassword);
                    } catch (Exception e) {
                        log.error("Unable to decrypt password for datasource \"" + id + "\"");
                    }
                }

                if (dialect != null && driver != null && url != null && username != null && password != null) {
                    DataSourceConfig dataSourceConfig = new DataSourceConfig(databaseType, url, username, password);
                    dataSourceConfig.setDialect(dialect);
                    dataSourceConfig.setDriver(driver);
                    this.dataSources.put(id, dataSourceConfig);
                }
            }
        }

    }

    private DatabaseType lookupType(String key) {
        if (key == null) {
            return null;
        }
        String lowerKey = key.toLowerCase();

        if (lowerKey.equals("postgresql") || lowerKey.contains("pgsql")) {
            return DatabaseType.POSTGRESQL;
        } else if (lowerKey.contains("oracle")) {
            return DatabaseType.ORACLE;
        } else if (lowerKey.contains("mssql")) {
            return DatabaseType.MSSQL;
        } else if (lowerKey.contains("mysql")) {
            return DatabaseType.MYSQL;
        } else if (lowerKey.contains("informix")) {
            return DatabaseType.INFORMIX;
        }
        return null;
    }

    public String getWorkspace() {
        return configuration.getString("workspace");
    }

    public String getSmtpServer() {
        return configuration.getString("smtp.server", "127.0.0.1");
    }

    public String getSmtpFrom() {
        return configuration.getString("smtp.from", "noreply@local");
    }

    public DataSourceConfig getDataSourceConfig(String name) {
        return dataSources.get(name);
    }

    public Set<String> getReportingDataSources() {
        Set<String> set = new HashSet<String>(dataSources.keySet());
        set.remove(ServerSettings.DC_ID_SCHEDULER);
        set.remove(ServerSettings.DC_ID_SYSTEM);
        return set;
    }

    public String getNetxmsServer() {
        return configuration.getString("netxms.server", "127.0.0.1");
    }

    public String getNetxmsLogin() {
        return configuration.getString("netxms.login", "admin");
    }

    public String getNetxmsPassword() {
        String password = null;
        if (configuration.containsKey("netxms.encryptedPassword")) {
            try {
                password = EncryptedPassword.decrypt(getNetxmsLogin(), configuration.getString("netxms.encryptedPassword"));
            } catch (Exception e) {
                log.error("Unable to decrypt netxms.encryptedPassword", e);
            }
        }
        return password != null ? password : configuration.getString("netxms.password", "");
    }

    private enum DatabaseType {
        POSTGRESQL("org.postgresql.Driver", "org.hibernate.dialect.PostgreSQL9Dialect"),
        ORACLE("oracle.jdbc.driver.OracleDriver", "org.hibernate.dialect.Oracle10gDialect"),
        MSSQL("com.microsoft.sqlserver.jdbc.SQLServerDriver", "org.hibernate.dialect.SQLServerDialect"),
        MYSQL("com.mysql.jdbc.Driver", "org.hibernate.dialect.MySQLDialect"),
        INFORMIX("com.informix.jdbc.IfxDriver", "org.hibernate.dialect.InformixDialect");

        private final String driver;
        private final String dialect;

        DatabaseType(String driver, String dialect) {
            this.driver = driver;
            this.dialect = dialect;
        }

        public String getDriver() {
            return driver;
        }

        public String getDialect() {
            return dialect;
        }
    }

    public class DataSourceConfig {
        private String dialect;
        private String driver;
        private String url;
        private String username;
        private String password;
        private String quartzDriverDelegate;

        public DataSourceConfig(DatabaseType type, String url, String username, String password) {
            this.dialect = type.getDialect();
            this.driver = type.getDriver();
            this.url = url;
            this.username = username;
            this.password = password;

            switch (type) {
                case POSTGRESQL:
                    quartzDriverDelegate = "org.quartz.impl.jdbcjobstore.PostgreSQLDelegate";
                    break;
                case ORACLE:
                    quartzDriverDelegate = "org.quartz.impl.jdbcjobstore.oracle.OracleDelegate";
                    break;
                case MSSQL:
                    quartzDriverDelegate = "org.quartz.impl.jdbcjobstore.MSSQLDelegate";
                    break;
                case MYSQL:
                    quartzDriverDelegate = "org.quartz.impl.jdbcjobstore.StdJDBCDelegate";
                    break;
                case INFORMIX:
                    quartzDriverDelegate = "org.quartz.impl.jdbcjobstore.StdJDBCDelegate";
                    break;
            }
        }

        public String getDialect() {
            return dialect;
        }

        public void setDialect(String dialect) {
            this.dialect = dialect;
        }

        public String getDriver() {
            return driver;
        }

        public void setDriver(String driver) {
            this.driver = driver;
        }

        public String getUrl() {
            return url;
        }

        public String getUsername() {
            return username;
        }

        public String getPassword() {
            return password;
        }

        public String getQuartzDriverDelegate() {
            return quartzDriverDelegate;
        }
    }
}
