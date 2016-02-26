package com.radensolutions.reporting;

import com.radensolutions.reporting.model.Notification;
import com.radensolutions.reporting.model.ReportResult;
import com.radensolutions.reporting.service.Connector;
import com.radensolutions.reporting.service.ReportManager;
import com.radensolutions.reporting.service.ServerSettings;
import net.sf.jasperreports.engine.DefaultJasperReportsContext;
import net.sf.jasperreports.engine.query.QueryExecuterFactory;
import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonInitException;
import org.apache.commons.dbcp.BasicDataSource;
import org.hibernate.boot.MetadataSources;
import org.hibernate.boot.registry.StandardServiceRegistryBuilder;
import org.hibernate.boot.spi.MetadataImplementor;
import org.hibernate.tool.hbm2ddl.SchemaExport;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.beans.factory.support.RootBeanDefinition;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;

import java.io.IOException;
import java.util.Set;

public class Launcher implements Daemon {

    private static final Logger log = LoggerFactory.getLogger(Launcher.class);
    private static final Object shutdownLatch = new Object();
    private AnnotationConfigApplicationContext context;

    public static void main(String[] args) throws Exception {
        //        generateSchemas();
        Launcher launcher = new Launcher();
        registerShutdownHook();
        launcher.init(null);
        launcher.start();
        while (true) {
            synchronized (shutdownLatch) {
                try {
                    shutdownLatch.wait();
                    break;
                } catch (InterruptedException e) {
                    // ignore and wait again
                }
            }
        }
        launcher.stop();
    }

    private static void generateSchemas() {
        generateSchema("postgres", "org.hibernate.dialect.PostgreSQL9Dialect");
        generateSchema("mysql", "org.hibernate.dialect.MySQLDialect");
        generateSchema("mssql", "org.hibernate.dialect.SQLServerDialect");
        generateSchema("oracle", "org.hibernate.dialect.Oracle10gDialect");
        generateSchema("informix", "org.hibernate.dialect.InformixDialect");
    }

    private static void generateSchema(String name, String dialect) {
        MetadataSources metadata = new MetadataSources(
                new StandardServiceRegistryBuilder().applySetting("hibernate.dialect", dialect).build());

        metadata.addAnnotatedClass(ReportResult.class);
        metadata.addAnnotatedClass(Notification.class);
        SchemaExport export = new SchemaExport((MetadataImplementor) metadata.buildMetadata());
        export.setOutputFile("sql/" + name + "/nxreporting.sql");
        export.setFormat(true);
        export.execute(true, false, false, false);

    }

    private static void registerShutdownHook() {
        Runtime.getRuntime().addShutdownHook(new Thread() {
            @Override
            public void run() {
                synchronized (shutdownLatch) {
                    shutdownLatch.notify();
                }
            }
        });
    }

    private static void registerReportingDataSources(AnnotationConfigApplicationContext context) {
        ServerSettings settings = context.getBean(ServerSettings.class);
        Set<String> registeredDataSources = settings.getReportingDataSources();
        for (String name : registeredDataSources) {
            BeanDefinition definition = new RootBeanDefinition(BasicDataSource.class);
            ServerSettings.DataSourceConfig dataSourceConfig = settings.getDataSourceConfig(name);
            definition.getPropertyValues().add("driverClassName", dataSourceConfig.getDriver());
            definition.getPropertyValues().add("url", dataSourceConfig.getUrl());
            definition.getPropertyValues().add("username", dataSourceConfig.getUsername());
            definition.getPropertyValues().add("password", dataSourceConfig.getPassword());
            context.registerBeanDefinition(name, definition);
        }
    }

    @Override
    public void init(DaemonContext daemonContext) throws DaemonInitException, Exception {
        log.info("Initializing daemon");

        context = new AnnotationConfigApplicationContext();
        context.register(AppContextConfig.class);
        context.refresh();
        context.registerShutdownHook();

        registerReportingDataSources(context);

        DefaultJasperReportsContext jrContext = DefaultJasperReportsContext.getInstance();
        //        jrContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl", "com.radensolutions.reporting.custom.NXCLQueryExecutorFactory");
        jrContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl",
                              "com.radensolutions.reporting.custom.NXCLQueryExecutorFactoryDummy");

        ReportManager reportManager = context.getBean(ReportManager.class);
        log.info("Report deployment started");
        reportManager.deploy();
        log.info("All reports successful deployed");
    }

    @Override
    public void start() throws IOException {
        log.info("Starting up");

        Connector connector = context.getBean(Connector.class);
        connector.start();
        log.info("Connector started");
    }

    @Override
    public void stop() {
        log.info("Shutdown initiated");
        Connector connector = context.getBean(Connector.class);
        connector.stop();
        log.info("Connector stopped");
    }

    @Override
    public void destroy() {

    }
}
