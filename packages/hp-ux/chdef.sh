#!/bin/sh

ed $HOME/.sw/defaults << EOT
1,\$s/^# *\(.*= *\)\/var\/spool\/sw *\$/\1\/tmp\/sw/
1,\$s/^# *\(swpackage.package_in_place\).*/\1 = true
w
q
EOT
