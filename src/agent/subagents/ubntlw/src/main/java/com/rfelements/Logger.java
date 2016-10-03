package com.rfelements;

import org.netxms.agent.SubAgent;

public class Logger {

    private final Class clazz;

    private Logger(Class clazz) {
        this.clazz = clazz;
    }

    public static Logger getInstance(Class clazz) {
        return new Logger(clazz);
    }

    public void e(final String message) {
        SubAgent.writeLog(SubAgent.LogLevel.ERROR, "[" + clazz.getName() + "] " + message);
    }

    public void w(final String message) {
        SubAgent.writeLog(SubAgent.LogLevel.WARNING, "[" + clazz.getName() + "] " + message);
    }

    public void i(final String message) {
        SubAgent.writeLog(SubAgent.LogLevel.INFO, "[" + clazz.getName() + "] " + message);
    }
}
