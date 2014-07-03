package com.radensolutions.reporting.application;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;

public class ServiceLauncher {

    private static final Logger log = LoggerFactory.getLogger(ServiceLauncher.class);
    private static final Object shutdownLatch = new Object();

    public static void start(String[] args) throws IOException {
        final ReportingServer reportingServer = ReportingServerFactory.getApp();

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

        System.out.println("Shutdown initiated");
        reportingServer.getConnector().stop();
        System.out.println("Clean Shutdown");
    }

    public static void stop(String[] args) {
        synchronized (shutdownLatch) {
            shutdownLatch.notify();
        }
   }

    public static void main(String[] args) throws IOException {
        if ("start".equals(args[0])) {
            start(args);
        } else if ("stop".equals(args[0])) {
            stop(args);
        }
    }
}