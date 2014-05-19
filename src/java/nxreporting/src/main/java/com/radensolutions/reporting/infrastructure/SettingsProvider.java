package com.radensolutions.reporting.infrastructure;

public interface SettingsProvider {
    String getValue(String key);

    Integer getIntValue(String key);
}
