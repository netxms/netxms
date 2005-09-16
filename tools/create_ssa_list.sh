#!/bin/sh

TIMESTAMP=`date`
LIST=`echo $1 | tr a-z A-Z`

echo "/* static_subagents.cpp  Generated at $TIMESTAMP */"
cat << EOT
#include <nxagentd.h>

extern "C"
{
EOT

for sa in $LIST; do
   echo "   BOOL NxSubAgentInit_$sa(NETXMS_SUBAGENT_INFO **, TCHAR *);"
done

cat << EOT
}

void InitStaticSubagents(void)
{
EOT

for sa in $LIST; do
   echo "   InitSubAgent(NULL, \"static:$sa\", NxSubAgentInit_$sa);"
done

echo "}"

exit 0
