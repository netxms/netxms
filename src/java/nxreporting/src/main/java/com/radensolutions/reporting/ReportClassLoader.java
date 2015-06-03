package com.radensolutions.reporting;

import java.net.URL;
import java.net.URLClassLoader;

public class ReportClassLoader extends URLClassLoader {
    public ReportClassLoader(URL[] urls, ClassLoader parent) {
        super(urls, parent);
    }

    @Override
    public Class<?> loadClass(String name) throws ClassNotFoundException {
        if (!"report.DataSource".equals(name)) {
            return super.loadClass(name);
        }
        return findClass(name);
    }
}
