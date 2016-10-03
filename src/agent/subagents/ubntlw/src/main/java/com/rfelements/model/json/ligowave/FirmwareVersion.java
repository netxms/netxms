package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class FirmwareVersion {

    private String backup;
    private String active;

    public String getBackup() {
        return backup;
    }

    public void setBackup(String backup) {
        this.backup = backup;
    }

    public String getActive() {
        return active;
    }

    public void setActive(String active) {
        this.active = active;
    }
}
