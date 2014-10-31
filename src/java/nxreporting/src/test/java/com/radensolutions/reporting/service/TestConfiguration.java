package com.radensolutions.reporting.service;

import junit.framework.TestCase;

public class TestConfiguration extends TestCase {
    public void testWorkspace() throws Exception {
        String workspace = new ServerSettings("config1.xml").getWorkspace();
        assertNotNull(workspace);
        assertEquals("\"workspace\" do not match", "/path/to/workspace", workspace);
    }

    public void testDatasource() throws Exception {
        ServerSettings serverSettings = new ServerSettings("config_datasource.xml");
        assertNull(serverSettings.getDataSourceConfig("nonexisting"));
        ServerSettings.DataSourceConfig dsConfig = serverSettings.getDataSourceConfig("system");
        assertNotNull(dsConfig);
        assertEquals("org.hibernate.dialect.PostgreSQL9Dialect", dsConfig.getDialect());
        assertEquals("jdbc:postgresql://127.0.0.1/netxms", dsConfig.getUrl());
        assertEquals("user", dsConfig.getUsername());
        assertEquals("pass", dsConfig.getPassword());
    }

    public void testEncryptedPassword() throws Exception {
        ServerSettings serverSettings = new ServerSettings("config_datasource.xml");
        ServerSettings.DataSourceConfig dsConfig = serverSettings.getDataSourceConfig("encrypted");
        assertNotNull(dsConfig);
        assertEquals("user", dsConfig.getUsername());
        assertEquals("pass", dsConfig.getPassword());
    }

    public void testTypes() throws Exception {
        ServerSettings serverSettings = new ServerSettings("config_datasource.xml");
        ServerSettings.DataSourceConfig dsConfigPostgres = serverSettings.getDataSourceConfig("postgresql");
        assertNotNull(dsConfigPostgres);
        assertEquals("org.hibernate.dialect.PostgreSQL9Dialect", dsConfigPostgres.getDialect());
        assertEquals("org.postgresql.Driver", dsConfigPostgres.getDriver());

        ServerSettings.DataSourceConfig dsConfigOracle = serverSettings.getDataSourceConfig("oracle");
        assertNotNull(dsConfigOracle);
        assertEquals("org.hibernate.dialect.Oracle10gDialect", dsConfigOracle.getDialect());
        assertEquals("oracle.jdbc.driver.OracleDriver", dsConfigOracle.getDriver());

        ServerSettings.DataSourceConfig dsConfigMsSql = serverSettings.getDataSourceConfig("mssql");
        assertNotNull(dsConfigMsSql);
        assertEquals("org.hibernate.dialect.SQLServerDialect", dsConfigMsSql.getDialect());
        assertEquals("com.microsoft.sqlserver.jdbc.SQLServerDriver", dsConfigMsSql.getDriver());

        ServerSettings.DataSourceConfig dsConfigMySql = serverSettings.getDataSourceConfig("mysql");
        assertNotNull(dsConfigMySql);
        assertEquals("org.hibernate.dialect.MySQLDialect", dsConfigMySql.getDialect());
        assertEquals("com.mysql.jdbc.Driver", dsConfigMySql.getDriver());

        ServerSettings.DataSourceConfig dsConfigInformix = serverSettings.getDataSourceConfig("informix");
        assertNotNull(dsConfigInformix);
        assertEquals("org.hibernate.dialect.InformixDialect", dsConfigInformix.getDialect());
        assertEquals("com.informix.jdbc.IfxDriver", dsConfigInformix.getDriver());
    }

    public void testNetxmsdConf() throws Exception {
        ServerSettings serverSettings = new ServerSettings("config1.xml");
        ServerSettings.DataSourceConfig system = serverSettings.getDataSourceConfig(ServerSettings.DC_ID_SYSTEM);
        assertNotNull("'system' datasource not defined", system);
        assertEquals("org.hibernate.dialect.PostgreSQL9Dialect", system.getDialect());
        assertEquals("org.postgresql.Driver", system.getDriver());
        ServerSettings.DataSourceConfig quartz = serverSettings.getDataSourceConfig(ServerSettings.DC_ID_SCHEDULER);
        assertNotNull("'quartz' datasource not defined", quartz);
        assertEquals("org.hibernate.dialect.PostgreSQL9Dialect", quartz.getDialect());
        assertEquals("org.postgresql.Driver", quartz.getDriver());
        ServerSettings.DataSourceConfig jasper = serverSettings.getDataSourceConfig(ServerSettings.DC_ID_REPORTING);
        assertNotNull("'jasper' datasource not defined", jasper);
        assertEquals("org.hibernate.dialect.PostgreSQL9Dialect", jasper.getDialect());
        assertEquals("org.postgresql.Driver", jasper.getDriver());
    }
}
