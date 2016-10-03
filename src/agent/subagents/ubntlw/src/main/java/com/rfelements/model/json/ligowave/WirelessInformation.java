package com.rfelements.model.json.ligowave;

/**
 * @author Pichanič Ján
 */
public class WirelessInformation {

    private Peer[] peers;

    private Vap[] vaps;

    private Radios radios;

    public Peer[] getPeers() {
        return peers;
    }

    public void setPeers(Peer[] peers) {
        this.peers = peers;
    }

    public Vap[] getVaps() {
        return vaps;
    }

    public void setVaps(Vap[] vaps) {
        this.vaps = vaps;
    }

    public Radios getRadios() {
        return radios;
    }

    public void setRadios(Radios radios) {
        this.radios = radios;
    }
}
