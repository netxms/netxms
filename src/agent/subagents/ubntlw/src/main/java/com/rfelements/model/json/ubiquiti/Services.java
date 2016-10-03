package com.rfelements.model.json.ubiquiti;

/**
 * @author Pichanič Ján
 */
public class Services {

    private Boolean dhcpc;

    private Boolean dhcpd;

    private Boolean pppoe;

    public Boolean getDhcpc() {
        return dhcpc;
    }

    public void setDhcpc(Boolean dhcpc) {
        this.dhcpc = dhcpc;
    }

    public Boolean getDhcpd() {
        return dhcpd;
    }

    public void setDhcpd(Boolean dhcpd) {
        this.dhcpd = dhcpd;
    }

    public Boolean getPppoe() {
        return pppoe;
    }

    public void setPppoe(Boolean pppoe) {
        this.pppoe = pppoe;
    }
}
