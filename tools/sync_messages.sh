#!/bin/bash
clear
PATH_TO_NETXMS=$1
#echo "Good morning, world."

java -jar ./Converter.jar $PATH_TO_NETXMS 
python sync_message_files.py $PATH_TO_NETXMS/src/java/netxms-eclipse
echo "Mesage files synchronized for eclipse netxms part"
python sync_message_files.py $PATH_TO_NETXMS/webui/webapp
echo "Mesage files synchronized for web netxms part"


