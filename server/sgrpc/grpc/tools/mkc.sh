#!/bin/sh

#find json/ -name \*.json |xargs ./gengrpc ipc
#	cp generate/* ../generate/ -rf
#exit

IPCJSON='
	json/account.json
	json/alarm.json
	json/audio.json
	json/connect.json
	json/devinfo.json
	json/ifconfig.json
	json/image.json
	json/log.json
	json/mdetect.json
	json/osd.json
	json/privacy.json
	json/ptz.json
	json/record.json
	json/storage.json
	json/stream.json
	json/remoteplay.json
	json/ivp.json
	'
./gengrpc ipc $IPCJSON
rm src_ipc/ -rf
mv generate/ src_ipc/

#存储服务器
CVRJSON='
	json/discovery.json
	json/devinfo.json
	json/channel.json
	json/device.json
	json/mdadm.json
	vms_json/stream_server.json
	vms_json/storage_server.json
	vms_json/webserver.json
	'
./gengrpc cvr $CVRJSON
rm src_cvr/ -rf
mv generate/ src_cvr/

#VMS设备配置
VDSJSON='
	json/account.json
	json/devinfo.json
	json/ifconfig.json
	json/matrix.json
	json/channel.json
	vms_json/stream_server.json
	vms_json/storage_server.json
	json/discovery.json
	'
./gengrpc vds $VDSJSON
rm src_vds/ -rf
mv generate/ src_vds/


VMSJSON='
	json/account.json
	json/alarm.json
	json/audio.json
	json/connect.json
	json/devinfo.json
	json/ifconfig.json
	json/image.json
	json/ivp.json
	json/log.json
	json/matrix.json
	json/mdetect.json
	json/osd.json
	json/privacy.json
	json/ptz.json
	json/record.json
	json/remoteplay.json
	json/storage.json
	json/stream.json
	'
./gengrpc vms $VMSJSON
rm src_vms/ -rf
mv generate/ src_vms/
