package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class Network {

    private Interface[] interfaces;

    private Boolean apTc;

    private String dhcp6mode;

    private String dhcpMode;

    private Route[] routes;

    private String[] dns;

    private Dns6 dns6;

    private Bridge[] bridges;

    public Interface[] getInterfaces() {
        return interfaces;
    }

    public void setInterfaces(Interface[] interfaces) {
        this.interfaces = interfaces;
    }

    public Boolean getApTc() {
        return apTc;
    }

    public void setApTc(Boolean apTc) {
        this.apTc = apTc;
    }

    public String getDhcp6mode() {
        return dhcp6mode;
    }

    public void setDhcp6mode(String dhcp6mode) {
        this.dhcp6mode = dhcp6mode;
    }

    public String getDhcpMode() {
        return dhcpMode;
    }

    public void setDhcpMode(String dhcpMode) {
        this.dhcpMode = dhcpMode;
    }

    public Route[] getRoutes() {
        return routes;
    }

    public void setRoutes(Route[] routes) {
        this.routes = routes;
    }

    public String[] getDns() {
        return dns;
    }

    public void setDns(String[] dns) {
        this.dns = dns;
    }

    public Dns6 getDns6() {
        return dns6;
    }

    public void setDns6(Dns6 dns6) {
        this.dns6 = dns6;
    }

    public Bridge[] getBridges() {
        return bridges;
    }

    public void setBridges(Bridge[] bridges) {
        this.bridges = bridges;
    }
}
