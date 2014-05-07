package com.radensolutions.reporting.infrastructure.impl;

import com.radensolutions.reporting.infrastructure.SettingsProvider;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

public class PropertyFileSettingsProvider implements SettingsProvider {

    org.slf4j.Logger log = LoggerFactory.getLogger(PropertyFileSettingsProvider.class);

    public PropertyFileSettingsProvider() {
        InputStream stream = ClassLoader.getSystemResourceAsStream("nxreporting.properties");
        Properties properties = new Properties();
        try {
            properties.load(stream);
        } catch (IOException e) {
            log.error("Can't load configuration file");
        }
    }

    @Override
    public String getValue(String key) {
        return null;
    }

    @Override
    public Integer getIntValue(String key) {
        return null;
    }
}
