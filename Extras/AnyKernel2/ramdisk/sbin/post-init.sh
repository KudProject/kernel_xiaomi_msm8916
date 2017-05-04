#!/system/bin/sh
#
# /sbin/post-init.sh
# krasCGQ's post-init script
# by krasCGQ @ xda-developers

# Unset BusyBox variable (IMPORTANT!)
unset BB;

# Detect BusyBox locations and automatically use the detected one
for i in /sbin /system/xbin /su/xbin /magisk/busybox-ndk/system/bin /data/magisk /dev/magisk; do
	if [ -f $i/busybox ]; then
		BB=$i/busybox;
		NOBB=0;
		break;
	fi;
done;

# Environment variables
INIT=/system/etc/init.d;
LP=com.android.vending.billing.InAppBillingService.LOCK;

# Remount root and system read-write
mount -t rootfs -o rw,remount rootfs;
mount -o rw,remount -t auto /system;

# Stop Google Service and restart it on boot (dorimanx)
if [ "$($BB pidof com.google.android.gms | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms);
fi;
if [ "$($BB pidof com.google.android.gms.unstable | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.unstable);
fi;
if [ "$($BB pidof com.google.android.gms.persistent | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.persistent);
fi;
if [ "$($BB pidof com.google.android.gms.wearable | wc -l)" -eq "1" ]; then
	$BB kill $($BB pidof com.google.android.gms.wearable);
fi;

# Force uninstall Lucky Patcher and reboot
# F*ck you Lucky Patcher users!
for i in /data/app/${LP}-*; do
	until [ ! -d "$i" ]; do
		rm -rf $i
	done;
done;
if [ -d "/data/data/$LP" ]; then
	rm -rf /data/data/$LP && reboot;
fi;

# Create init.d folder and set permissions
if [ ! -d "$INIT" ]; then
	mkdir $INIT;
fi;
chmod 755 $INIT;
chown root.root $INIT;

# Run init.d scripts
if [ "$NOBB" == 0 ]; then
	$BB run-parts $INIT;
elif [ -f "/system/bin/run-parts" ] || [ -f "/system/xbin/run-parts" ]; then
	run-parts $INIT;
else
	for i in $INIT/*; do
		sh $i;
	done;
fi;

# Sleep for 30 seconds (duh!)
sleep 30;

# Google Play services wakelock fix (requires su)
su -c "pm enable com.google.android.gms/.update.SystemUpdateActivity"
su -c "pm enable com.google.android.gms/.update.SystemUpdateService"
su -c "pm enable com.google.android.gms/.update.SystemUpdateService$ActiveReceiver"
su -c "pm enable com.google.android.gms/.update.SystemUpdateService$Receiver"
su -c "pm enable com.google.android.gms/.update.SystemUpdateService$SecretCodeReceiver"
su -c "pm enable com.google.android.gsf/.update.SystemUpdateActivity"
su -c "pm enable com.google.android.gsf/.update.SystemUpdatePanoActivity"
su -c "pm enable com.google.android.gsf/.update.SystemUpdateService"
su -c "pm enable com.google.android.gsf/.update.SystemUpdateService$Receiver"
su -c "pm enable com.google.android.gsf/.update.SystemUpdateService$SecretCodeReceiver"

echo "All scripts are executed! Exiting...";
exit 0;
