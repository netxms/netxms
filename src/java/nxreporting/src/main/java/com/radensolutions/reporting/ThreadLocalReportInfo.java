package com.radensolutions.reporting;

/**
 * Dirty hack
 */
public final class ThreadLocalReportInfo {
    private static final ThreadLocal<String> location = new ThreadLocal<String>();

    public static String getReportLocation() {
        return location.get();
    }

    public static void setReportLocation(final String location) {
        ThreadLocalReportInfo.location.set(location);
    }
}
