# KudKernel - System modifier
# krasCGQ @ xda-developers

##################################
# /system/etc/thermal-engine.conf
THERMAL=/system/etc/thermal-engine.conf;

# Backup
test ! -f $THERMAL~ && cp -f $THERMAL $THERMAL~;

# Increase thermal limit a bit
sed -i s/set_point\ 43000/set_point\ 60000/g $THERMAL;
sed -i s/set_point_clr\ 55000/set_point_clr\ 60000/g $THERMAL;

# Use a little higher frequency limit when throttles
sed -i s/device_max_limit\ 800000/device_max_limit\ 960000/g $THERMAL;
