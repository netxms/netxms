package com.radensolutions.reporting;

import com.radensolutions.reporting.service.Connector;
import com.radensolutions.reporting.service.ReportManager;
import com.radensolutions.reporting.service.ServerSettings;
import net.sf.jasperreports.engine.DefaultJasperReportsContext;
import net.sf.jasperreports.engine.query.QueryExecuterFactory;
import org.apache.commons.daemon.Daemon;
import org.apache.commons.daemon.DaemonContext;
import org.apache.commons.daemon.DaemonInitException;
import org.apache.commons.dbcp.BasicDataSource;
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
        jrContext.setProperty(QueryExecuterFactory.QUERY_EXECUTER_FACTORY_PREFIX + "nxcl", "com.radensolutions.reporting.custom.NXCLQueryExecutorFactoryDummy");

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
}
