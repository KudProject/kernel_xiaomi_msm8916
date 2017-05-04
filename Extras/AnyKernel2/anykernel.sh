# AnyKernel2 Ramdisk Mod Script
# osm0sis @ xda-developers

## AnyKernel setup
# EDIFY properties
do.devicecheck=1
do.modules=0
do.cleanup=1
do.cleanuponabort=1
device.name=ido

# shell variables
block=/dev/block/mmcblk0p22;

## AnyKernel methods (DO NOT CHANGE)
# import patching functions/variables - see for reference
. /tmp/anykernel/tools/ak2-core.sh;

## AnyKernel permissions
# set permissions for included ramdisk files
chmod -R 755 $ramdisk;
chmod 755 $ramdisk/sbin/post-init.sh;

## AnyKernel install
dump_boot;

# begin ramdisk changes

####################################################
# Backup all original files before applying changes
backup_file init.rc
backup_file init.qcom.power.rc

###################
# Applying changes
# Import KudKernel init scripts
insert_line init.rc 'KudKernel' before 'on early-init' '# Import KudKernel init scripts';
insert_line init.rc 'init.kud.rc' after '# Import KudKernel init scripts' 'import /init.kud.rc';

# Use LineageOS values for interactive permissions
replace_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/timer_rate' '    # Use LineageOS values for interactive permissions';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/timer_rate';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/timer_slack';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/timer_slack';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/min_sample_time';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/min_sample_time';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/hispeed_freq';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/target_loads';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/target_loads';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/go_hispeed_load';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/above_hispeed_delay';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/boost';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/boost';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/boostpulse';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/input_boost';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/input_boost';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/boostpulse_duration';
remove_line init.rc '    chown system system /sys/devices/system/cpu/cpufreq/interactive/io_is_busy';
remove_line init.rc '    chmod 0660 /sys/devices/system/cpu/cpufreq/interactive/io_is_busy';

# Also use exact permissions for powersave cluster
insert_line init.rc 'chown system system /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq' after '    chown system system /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' '    chown system system /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq';
insert_line init.rc 'chmod 0660 /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq' after '    chmod 0660 /sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq' '    chmod 0660 /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq';

# Use consistent values on performance cluster
replace_line init.qcom.power.rc 'write /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay "19000 1113600:39000"' 'write /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay 39000';

# Set a bit lower minimum frequencies
replace_line init.qcom.power.rc '    write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 960000' '    write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 400000';
replace_line init.qcom.power.rc '    write /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq 800000' '    write /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq 400000';
replace_line init.qcom.power.rc '    setprop ro.min_freq_0 960000' '    setprop ro.min_freq_0 400000';
replace_line init.qcom.power.rc '    setprop ro.min_freq_4 800000' '    setprop ro.min_freq_4 400000';

# Do we care about Qualcomm's interactive tuning?
replace_line init.qcom.power.rc '    write /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads "1 960000:85 1113600:90 1344000:80"' '    write /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads 80';
replace_line init.qcom.power.rc '    write /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads "1 800000:90"' '    write /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads 80';

# Powersave CPU governor is disabled, use ondemand
replace_line init.qcom.power.rc '    write /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor "powersave"' '    write /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor "ondemand"';

# end ramdisk changes

write_boot;

## end install

