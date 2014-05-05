package com.radensolutions.reporting.application;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

public class StandaloneLauncher {
    private static final Logger log = LoggerFactory.getLogger(StandaloneLauncher.class);
    private final Object shutdownLatch = new Object();

    public static void main(String[] args) {
        try {
            new StandaloneLauncher().realMain(args);
        } catch (Exception e) {
            log.error("Application Error", e);
        }
    }

    private void realMain(String[] args) throws IOException {
        validateConfigurationFiles();
        final ReportingServer reportingServer = ReportingServerFactory.getApp();
        Runtime.getRuntime().addShutdownHook(new Thread() {
            @Override
            public void run() {
                synchronized (StandaloneLauncher.this.shutdownLatch) {
                    StandaloneLauncher.this.shutdownLatch.notify();
                }
            }
        });

        reportingServer.getConnector().start();
        log.info("Compiling report definitions");
        reportingServer.getReportManager().compileDeployedReports();
        log.info("Done compiling report definitions");

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

        System.out.println("Clean Shutdown");

        reportingServer.getConnector().stop();
    }

    private void validateConfigurationFiles() {
        if (ClassLoader.getSystemResource("nxreporting.properties") == null) {
            throw new RuntimeException("Configuration file \"nxreporting.properties\" not found");
        }
    }
}
