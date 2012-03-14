package org.netxms.agent;

public enum ParameterType {

    INT(0),
    UINT(1),
    INT64(2),
    UINT64(3),
    STRING(4),
    FLOAT(5),
    NULL(6);

    private int value;

    ParameterType(final int value) {
        this.value = value;
    }

    public int getValue() {
        return value;
    }
}
