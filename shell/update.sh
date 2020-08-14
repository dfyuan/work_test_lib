#!/system/bin/sh
header=`cat /data/payload_properties.txt`
update_engine_client --payload="file:///storage/emulated/0/Movies/payload.bin" --update --headers="${header}"
