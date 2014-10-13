package com.radensolutions.reporting.application;

public class ReportingServerFactory {
    private static final ReportingServerFactory factory = new ReportingServerFactory();
    private ReportingServer reportingServer;

    private ReportingServerFactory() {
        // empty private constructor
    }

    public static ReportingServer getApp() {
        return factory.getReportingServer();
    }

    private ReportingServer getReportingServer() {
        if (reportingServer == null) {
            synchronized (this) {
                if (reportingServer == null)
                    reportingServer = new ReportingServer();
            }
        }

        return reportingServer;
    }
}
