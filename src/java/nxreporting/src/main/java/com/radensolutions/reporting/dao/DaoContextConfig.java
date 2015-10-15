package com.radensolutions.reporting.dao;

import com.mchange.v2.c3p0.ComboPooledDataSource;
import com.radensolutions.reporting.service.ServerSettings;
import org.hibernate.SessionFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.beans.factory.annotation.Qualifier;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.ComponentScan;
import org.springframework.context.annotation.Configuration;
import org.springframework.orm.hibernate4.HibernateTransactionManager;
import org.springframework.orm.hibernate4.LocalSessionFactoryBuilder;
import org.springframework.transaction.annotation.EnableTransactionManagement;

import javax.annotation.PostConstruct;
import javax.sql.DataSource;
import java.beans.PropertyVetoException;
import java.util.Properties;

@Configuration
@ComponentScan("com.radensolutions.reporting")
@EnableTransactionManagement
public class DaoContextConfig {

    private static final Logger log = LoggerFactory.getLogger(DaoContextConfig.class);

    @Autowired
    private ServerSettings settings;

    @PostConstruct
    public void init() {
        log.debug("DaoContextConfig initialized.");
    }

    @Bean
    @Qualifier("systemDataSource")
    public DataSource dataSource() throws PropertyVetoException {
        ComboPooledDataSource dataSource = new ComboPooledDataSource();
        ServerSettings.DataSourceConfig dataSourceConfig = settings.getDataSourceConfig(ServerSettings.DC_ID_SYSTEM);
        dataSource.setDriverClass(dataSourceConfig.getDriver());
        dataSource.setJdbcUrl(dataSourceConfig.getUrl());
        dataSource.setUser(dataSourceConfig.getUsername());
        dataSource.setPassword(dataSourceConfig.getPassword());
        dataSource.setTestConnectionOnCheckout(true);
        dataSource.setCheckoutTimeout(3000);
        dataSource.setIdleConnectionTestPeriod(300);
        return dataSource;
    }

    private Properties hibernateProperties() {
        Properties properties = new Properties();
        ServerSettings.DataSourceConfig dataSourceConfig = settings.getDataSourceConfig(ServerSettings.DC_ID_SYSTEM);
        properties.setProperty("hibernate.dialect", dataSourceConfig.getDialect());
        properties.setProperty("hibernate.show_sql", "true");
        return properties;
    }

    @Autowired
    @Bean
    public SessionFactory sessionFactory(DataSource dataSource) {
        LocalSessionFactoryBuilder sessionBuilder = new LocalSessionFactoryBuilder(dataSource);
        sessionBuilder.addProperties(hibernateProperties());
        sessionBuilder.scanPackages("com.radensolutions.reporting.model");
        return sessionBuilder.buildSessionFactory();
    }

    @Autowired
    @Bean
    public HibernateTransactionManager transactionManager(SessionFactory sessionFactory) {
        return new HibernateTransactionManager(sessionFactory);
    }
}
