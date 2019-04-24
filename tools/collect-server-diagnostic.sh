#!/bin/sh

#create folder

ts=`date +%Y%m%d-%H%M%S`;
mkdir /tmp/netxms.info.$ts;

#OS level information:
touch /tmp/netxms.info.$ts/os.lvl;
#Host name
echo "Host name:" >> /tmp/netxms.info.$ts/os.lvl;
uname -a >> /tmp/netxms.info.$ts/os.lvl;
#OS version
echo "OS version:" >> /tmp/netxms.info.$ts/os.lvl;
if hash oslevel > /dev/null 2>&1; then
    oslevel -s >> /tmp/netxms.info.$ts/os.lvl;
fi
if hash machinfo > /dev/null 2>&1; then
    machinfo >> /tmp/netxms.info.$ts/os.lvl;  
fi 
if hash lsb_release > /dev/null 2>&1; then
    lsb_release -idr >> /tmp/netxms.info.$ts/os.lvl;   
fi
if hash lsb_release > /dev/null 2>&1; then
    lsb_release -idr >> /tmp/netxms.info.$ts/os.lvl;   
fi
if [ -f /etc/release ]; then
    cat /etc/release >> /tmp/netxms.info.$ts/os.lvl; 
fi
#Memory usage
echo "Memory usage:"  >> /tmp/netxms.info.$ts/os.lvl;
free -m >> /tmp/netxms.info.$ts/os.lvl;
#Uptime
echo "Uptime:"  >> /tmp/netxms.info.$ts/os.lvl;
uptime >> /tmp/netxms.info.$ts/os.lvl;
#Process list
echo "Process list:"  >> /tmp/netxms.info.$ts/os.lvl;
ps -e >> /tmp/netxms.info.$ts/os.lvl;
#Open files (lsof output)
echo "Open files:"  >> /tmp/netxms.info.$ts/os.lvl;
lsof >> /tmp/netxms.info.$ts/os.lvl;
#interfaces ifconfig
echo "Interfaces:"  >> /tmp/netxms.info.$ts/os.lvl;
ifconfig -a >> /tmp/netxms.info.$ts/os.lvl;
#iprouts
echo "route:"  >> /tmp/netxms.info.$ts/os.lvl;
route >> /tmp/netxms.info.$ts/os.lvl;
#arpcache
echo "arpcache:"  >> /tmp/netxms.info.$ts/os.lvl;
arp -a  >> /tmp/netxms.info.$ts/os.lvl;

#Output of nxadm commands:
touch /tmp/netxms.info.$ts/nxadm.lvl;
#debug
echo "debug:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "debug"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show dbcp
echo "show dbcp:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show dbstats"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show dbstats
echo "show dbstats:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show dbstats"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show flags
echo "show flags:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show flags"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show heap details
echo "show heap details:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show heap details"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show heap summary
echo "show heap summary:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show heap summary"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show index id
echo "show index id:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show index id"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show index interface
echo "show index interface:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show index interface"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show index nodeaddr
echo "show index nodeaddr:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show index nodeaddr"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show index subnet
echo "show index subnet:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show index subnet"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show index zone
echo "show index zone:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show index zone"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show modules
echo "show modules:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show modules"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show msgwq
echo "show msgwq:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show msgwq"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show objects
echo "show objects:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show objects"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show pe
echo "show pe:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show pe"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show pollers
echo "show pollers:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show pollers"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show queues
echo "show queues:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show queues"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show sessions
echo "show sessions:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show sessions"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show stats
echo "show stats:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show stats"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show tunnels
echo "show tunnels:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show tunnels"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show users
echo "show users:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show users"  >> /tmp/netxms.info.$ts/nxadm.lvl;
#show watchdog
echo "show watchdog:"  >> /tmp/netxms.info.$ts/nxadm.lvl;
./nxadm -c "show watchdog"  >> /tmp/netxms.info.$ts/nxadm.lvl;

#netxmsd, nxagentd log files
netxmsLog=`./netxmsd -l | tail -1`
cp $netxmsLog /tmp/netxms.info.$ts/
netxmsLog=`./nxagentd -l | tail -1`
cp $netxmsLog /tmp/netxms.info.$ts/

#zipDeleteFolder
tar -cvf /tmp/netxms.info.$ts.tar  /tmp/netxms.info.$ts
rm -rf /tmp/netxms.info.$ts