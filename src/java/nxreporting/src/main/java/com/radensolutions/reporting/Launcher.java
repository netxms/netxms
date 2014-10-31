package com.radensolutions.reporting;

import com.radensolutions.reporting.service.Connector;
import com.radensolutions.reporting.service.ReportManager;
import com.radensolutions.reporting.service.ServerSettings;
import org.apache.commons.dbcp.BasicDataSource;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.beans.factory.config.BeanDefinition;
import org.springframework.beans.factory.support.RootBeanDefinition;
import org.springframework.context.annotation.AnnotationConfigApplicationContext;

import java.io.IOException;
import java.util.Set;

public class Launcher {
    private static final Logger log = LoggerFactory.getLogger(Launcher.class);
    private static final Object shutdownLatch = new Object();

    public static void main(String[] args) {
        try {
            if (args.length > 0 && args[0].equalsIgnoreCase("start")) {
                start();
            } else if (args.length > 0 && args[0].equalsIgnoreCase("stop")) {
                stop();
            } else {
                registerShutdownHook();
                start();
            }
        } catch (IOException e) {
            log.error("Application error", e);
        }
    }

    private static void start() throws IOException {
        log.info("Starting up");

        log.debug("Creating context");
        AnnotationConfigApplicationContext context = new AnnotationConfigApplicationContext();
        context.register(AppContextConfig.class);
        context.refresh();
        context.registerShutdownHook();

        registerReportingDataSources(context);

        Connector connector = context.getBean(Connector.class);
        connector.start();
        log.info("Connector started");

        ReportManager reportManager = context.getBean(ReportManager.class);
        reportManager.deploy();
        log.info("All reports successful deployed");

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

        log.info("Shutdown initiated");
        connector.stop();
        log.info("Connector stopped");
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

    private static void stop() {
        synchronized (shutdownLatch) {
            shutdownLatch.notify();
        }
    }

    private static void registerShutdownHook() {
        Runtime.getRuntime().addShutdownHook(new Thread() {
            @Override
            public void run() {
                Launcher.stop();
            }
        });
    }

}
