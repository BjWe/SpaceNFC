#! /bin/sh

### BEGIN INIT INFO
# Provides:          snackshop
# Required-Start:    $local_fs $remote_fs
# Required-Stop:
# X-Start-Before:    rmnologin
# Default-Start:     2 3 4 5
# Default-Stop:
# Short-Description: Snackshop
# Description: Snackshop.
### END INIT INFO

# Actions
case "$1" in
    start)
        # START
        echo "start snackshop"
        cd /opt/spacenfc/
        screen -S snackshop -dm flutter-pi /home/pi/flutter_assets/
        ;;
    stop)
        # STOP
        echo "stop snackshop"
        killall flutter-pi
        ;;
    restart)
        # RESTART
        echo "restart snackshop"
        /etc/init.d/snackshop stop
        /etc/init.d/snackshop start
        ;;
esac

exit 0
