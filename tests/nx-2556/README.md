One-command deployment of disposable network of containers under Incus hypervisor.
Configured to test NX-2556 changes, but can be adapted to test many scenarios.

```
 $ time make clean; time make all.wg
...
for x in aa bb gw; do incus exec wg-network-setup-$x -- wg; done
interface: aa
  public key: 43/JdhhcQsyJssH+rpL8gV3U/G7TA2wf54VVkH+jPUg=
  private key: (hidden)
  listening port: 60823

peer: iKV5EhNsLpK6afP+qfm9/zWig9p6dbRK9M6dTkeMmiQ=
  endpoint: [fe80::216:3eff:fedd:2684%eth0]:51820
  allowed ips: 192.168.1.0/24, 192.168.2.0/24
  latest handshake: 2 seconds ago
  transfer: 92 B received, 328 B sent
  persistent keepalive: every 25 seconds
interface: bb
  public key: HCqEWp2F4AcZm6ry+5eVYmQCDErD3RNLvIb7/35xZUY=
  private key: (hidden)
  listening port: 40898

peer: iKV5EhNsLpK6afP+qfm9/zWig9p6dbRK9M6dTkeMmiQ=
  endpoint: 10.107.72.243:51820
  allowed ips: 192.168.1.0/24, 192.168.2.0/24
  latest handshake: 1 second ago
  transfer: 92 B received, 328 B sent
  persistent keepalive: every 25 seconds
interface: gw
  public key: iKV5EhNsLpK6afP+qfm9/zWig9p6dbRK9M6dTkeMmiQ=
  private key: (hidden)
  listening port: 51820

peer: HCqEWp2F4AcZm6ry+5eVYmQCDErD3RNLvIb7/35xZUY=
  endpoint: 10.107.72.211:40898
  allowed ips: 192.168.2.2/32
  latest handshake: 2 seconds ago
  transfer: 180 B received, 92 B sent

peer: 43/JdhhcQsyJssH+rpL8gV3U/G7TA2wf54VVkH+jPUg=
  endpoint: [fe80::216:3eff:fe39:5d83%eth0]:60823
  allowed ips: 192.168.1.2/32
  latest handshake: 3 seconds ago
  transfer: 180 B received, 92 B sent
touch all.networked

real    1m43.795s
user    0m3.412s
sys     0m3.164s
```

```
 $ time make clean; time make all.netxms.installed
...
incus exec wg-network-setup-gw -- nxget -p4700 127.0.0.1 Net.WireguardInterfaces
| INDEX | NAME | PUBLIC_KEY                                   | LISTEN_PORT |
| 0     | gw   | CUASJh+UttipCbXok+89fdUGDhWzMn0W6cXZgbnsWnc= | 51820       |
incus exec wg-network-setup-gw -- nxget -p4700 127.0.0.1 Net.WireguardPeers
| INDEX | INTERFACE | PEER_PUBLIC_KEY                              | ENDPOINT                        | ALLOWED_IPS    | HANDSHAKE_TIMESTAMP |
| 0     | gw        | qvaHowx/+tZhQMiRXNlXPMZKq0F9spUTYmZiIwv8Xxc= | [fe80::216:3eff:fe2c:1fe7]:8333 | 192.168.1.2/32 | 1721958239          |
| 1     | gw        | Q5YDFcA9/QlbLjSBcns+R9X4qZ9I8rVvH0F+pAmTOho= | 10.107.72.117:54705             | 192.168.2.2/32 | 1721958240          |
touch wg-network-setup-gw.netxms.installed
make[1]: Leaving directory '/home/autkin/wg-network-setup'
touch all.netxms.installed

real    7m26.239s
user    0m12.883s
sys     0m19.126s
```
