#!/bin/sh

#export LD_LIBRARY_PATH=/progs/lib/
#Here the mount cmd, It is because the fs is readonly, So for display of snapshot pic, mount another directory here.
export PATH=/progs/bin/:$PATH
#./bin/thttpd -d /progs/html/ -c /cgi-bin/\* -u root
#./bin/rtspServerForJovision /etc/conf.d/jovision/pipe/ live%d.264 3&

###################################################
#Here These commond, is working for the thttpd bug.
#
# The thtpd bug :
# When thttpd running, If set time year add 1, thttpd
# will do sth. and ipcam work bad.
#
#date -s 071309151991.00
#wget -P /tmp/ http://127.0.0.1/index.html
#rm /tmp/index.html
#hwclock -s -u

if ! mountpoint /progs/html/cgi-bin/snapshot
then
	mount /tmp /progs/html/cgi-bin/snapshot
fi

./bin/sctrl
reboot

#sctrl退出后要清理thttpd和udhcpc
#killall thttpd udhcpc
