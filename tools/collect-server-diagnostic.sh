#!/bin/sh

#create folder

TS=`date +%Y%m%d-%H%M%S`
LOG_DIR=/tmp/netxms.info.$TS
mkdir $LOG_DIR

#OS level information:
OS_LOG=$LOG_DIR/os.lvl
touch $OS_LOG
#Host name
echo "Host name:" >> $OS_LOG
uname -a >> $OS_LOG
#OS version
echo "OS version:" >> $OS_LOG
if command -v oslevel; then
    oslevel -s >> $OS_LOG
fi
if command -v  machinfo; then
    machinfo >> $OS_LOG  
fi 
if command -v lsb_release; then
    lsb_release -idr >> $OS_LOG   
fi
if [ -f /etc/release ]; then
    cat /etc/release >> $OS_LOG 
fi
#Memory usage
echo "Memory usage:"  >> $OS_LOG
if command -v free; then
    free -m >> $OS_LOG
fi
if command -v svmon; then
    svmon -P >> $OS_LOG
fi
if command -v top; then
    top -b -n1 >> $OS_LOG
    if [ $? -ne 0 ]; then
        top -b >> $OS_LOG
    fi
fi
#Uptime
echo "Uptime:"  >> $OS_LOG
uptime >> $OS_LOG
#Process list
echo "Process list:"  >> $OS_LOG
ps -e >> $OS_LOG
#Open files (lsof output)
echo "Open files:"  >> $OS_LOG
SERVER_PID=-1
AGENT_PID=-1
if command -v pidof; then
    SERVER_PID=`pidof netxmsd`   
    AGENT_PID=`pidof nxagentd`   
elif command -v pgrep; then
    SERVER_PID=`pgrep -x netxmsd`
    AGENT_PID=`pgrep -x nxagentd`
else
    SERVER_PID=$(ps -eo pid,comm | awk '$2 == "netxmsd" {print $1}')
    SERVER_PID=$(ps -eo pid,comm | awk '$2 == "nxagentd" {print $1}')
fi

if command -v lsof; then
    lsof -p $SERVER_PID >> $OS_LOG
    lsof -p $AGENT_PID >> $OS_LOG
elif command -v pfiles; then
    pfiles $SERVER_PID >> $OS_LOG
    pfiles $AGENT_PID >> $OS_LOG
elif command -v procfiles; then
    procfiles $SERVER_PID >> $OS_LOG
    procfiles $AGENT_PID >> $OS_LOG
else
    if [ -f /proc/$SERVER_PID/fd ]; then
        ls -l /proc/$SERVER_PID/fd >> $OS_LOG
    fi
    if [ -f /proc/$AGENT_PID/fd ]; then
        ls -l /proc/$AGENT_PID/fd >> $OS_LOG
    fi    
fi
#interfaces ifconfig
echo "Interfaces:"  >> $OS_LOG
if command -v ifconfig; then
    ifconfig -a >> $OS_LOG
elif command -v ip; then
    ip address >> $OS_LOG
fi
#iprouts
echo "route:"  >> $OS_LOG
if command -v route; then
    route -n >> $OS_LOG
fi
if command -v netstat; then
    netstat -rn >> $OS_LOG
fi
#arpcache
echo "arpcache:"  >> $OS_LOG
arp -a  >> $OS_LOG

#Output of nxadm commands:
NXADM_LOG=$LOG_DIR/nxadm.lvl
touch $NXADM_LOG
#debug
echo "debug:"  >> $NXADM_LOG
./nxadm -c "debug"  >> $NXADM_LOG
#show dbcp
echo "show dbcp:"  >> $NXADM_LOG
./nxadm -c "show dbcp"  >> $NXADM_LOG
#show dbstats
echo "show dbstats:"  >> $NXADM_LOG
./nxadm -c "show dbstats"  >> $NXADM_LOG
#show flags
echo "show flags:"  >> $NXADM_LOG
./nxadm -c "show flags"  >> $NXADM_LOG
#show heap details
echo "show heap details:"  >> $NXADM_LOG
./nxadm -c "show heap details"  >> $NXADM_LOG
#show heap summary
echo "show heap summary:"  >> $NXADM_LOG
./nxadm -c "show heap summary"  >> $NXADM_LOG
#show index id
echo "show index id:"  >> $NXADM_LOG
./nxadm -c "show index id"  >> $NXADM_LOG
#show index interface
echo "show index interface:"  >> $NXADM_LOG
./nxadm -c "show index interface"  >> $NXADM_LOG
#show index nodeaddr
echo "show index nodeaddr:"  >> $NXADM_LOG
./nxadm -c "show index nodeaddr"  >> $NXADM_LOG
#show index subnet
echo "show index subnet:"  >> $NXADM_LOG
./nxadm -c "show index subnet"  >> $NXADM_LOG
#show index zone
echo "show index zone:"  >> $NXADM_LOG
./nxadm -c "show index zone"  >> $NXADM_LOG
#show modules
echo "show modules:"  >> $NXADM_LOG
./nxadm -c "show modules"  >> $NXADM_LOG
#show msgwq
echo "show msgwq:"  >> $NXADM_LOG
./nxadm -c "show msgwq"  >> $NXADM_LOG
#show objects
echo "show objects:"  >> $NXADM_LOG
./nxadm -c "show objects"  >> $NXADM_LOG
#show pe
echo "show pe:"  >> $NXADM_LOG
./nxadm -c "show pe"  >> $NXADM_LOG
#show pollers
echo "show pollers:"  >> $NXADM_LOG
./nxadm -c "show pollers"  >> $NXADM_LOG
#show queues
echo "show queues:"  >> $NXADM_LOG
./nxadm -c "show queues"  >> $NXADM_LOG
#show sessions
echo "show sessions:"  >> $NXADM_LOG
./nxadm -c "show sessions"  >> $NXADM_LOG
#show stats
echo "show stats:"  >> $NXADM_LOG
./nxadm -c "show stats"  >> $NXADM_LOG
#show tunnels
echo "show tunnels:"  >> $NXADM_LOG
./nxadm -c "show tunnels"  >> $NXADM_LOG
#show users
echo "show users:"  >> $NXADM_LOG
./nxadm -c "show users"  >> $NXADM_LOG
#show watchdog
echo "show watchdog:"  >> $NXADM_LOG
./nxadm -c "show watchdog"  >> $NXADM_LOG

#netxmsd, nxagentd log files
netxmsLog=`./netxmsd -l | tail -1`
cp $netxmsLog $LOG_DIR/
netxmsLog=`./nxagentd -l | tail -1`
cp $netxmsLog $LOG_DIR/

#zipDeleteFolder
tar -cvf $LOG_DIR.tar  $LOG_DIR
rm -rf $LOG_DIR