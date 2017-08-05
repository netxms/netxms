package com.rfelements;

import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;

public class Logger {

    private final Class<?> clazz;

    private Logger(Class<?> clazz) {
        this.clazz = clazz;
    }

    public static Logger getInstance(Class<?> clazz) {
        return new Logger(clazz);
    }

    public void e(final String message) {
       Platform.writeLog(LogLevel.ERROR, "[" + clazz.getName() + "] " + message);
    }

    public void w(final String message) {
       Platform.writeLog(LogLevel.WARNING, "[" + clazz.getName() + "] " + message);
    }

    public void i(final String message) {
       Platform.writeLog(LogLevel.INFO, "[" + clazz.getName() + "] " + message);
    }
}
