package com.rfelements.model.json.ubiquiti;

/**
 * @author Pichanič Ján
 */
public class Firewall {

    private Integer iptables;

    private Integer ebtables;

    public Integer getIptables() {
        return iptables;
    }

    public void setIptables(Integer iptables) {
        this.iptables = iptables;
    }

    public Integer getEbtables() {
        return ebtables;
    }

    public void setEbtables(Integer ebtables) {
        this.ebtables = ebtables;
    }
}
