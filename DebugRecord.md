# UnifySDK Debug Record

## Downgrade version from 1.7.0 to 1.6.0
1) Stop services
```
  sudo systemctl stop uic-zigpc uic-upvl uic-gms uic-image-provider uic-dev-gui uic-eed uic-nal uic-upti-cap uic-upti-writer 2>/dev/null
```

2)  Purge the exact installed packages
```
   sudo apt purge -y \
     libunify \
     uic-dev-gui \
     uic-eed \
     uic-gms \
     uic-image-provider \
     uic-nal \
     uic-upti-cap \
     uic-upti-writer \
     uic-upvl \
     uic-zigpc
```
  Then:
```
   sudo apt autoremove --purge -y
```
3) Clean old Unify runtime state
```
   sudo rm -rf /etc/uic/*
   sudo rm -rf /var/lib/uic/*
```

4) Install the older version of Unify SDK 1.6.0
```
sudo apt install ./*.deb
```
5) Verify the installed version
```
dpkg -l | grep -E 'uic-|libunify'
```

## Fix " Serial port open failed: Permission denied, ERROR: sl_zigbee_ezsp_error_handler 0x28'" issue

1) Check the owner and group of the serial port
```
ls -l /dev/ttyACM0
```
The output is like this:
```
crw-rw---- 1 root plugdev 166, 0 Jun 17 10:00 /dev/ttyACM0
```
The group owner is plugdev, but the uic user is not in the plugdev group

2) Add contents in /etc/systemd/system/uic-zigpc.service.d/override.conf
```
 [Service]
 SupplementaryGroups=plugdev
```

## Enable Well-Known Key Support in Unify SDK
 In /etc/uic/uic.cfg:
```
 zigpc:
   tc_use_well_known_key: true
```

## Fix issue "Failed to load address-table info from datastore"

Rebuild/reflash the NCP with a larger address table, specifically:
```
   EMBER_ADDRESS_TABLE_SIZE=32
```
  Then restart ZigPC.

## The issue "zigPC crash when discover devices which has a large number of endpoints or large clusters"

the root cause is in zigpc_gateway_notify.c, ZigPC blindly copies all discovered clusters:
```
   endpoint_discovered.endpoint.cluster_count = endpointInfo->inClusterCount;
   for (size_t cluster_index = 0; cluster_index < endpointInfo->inClusterCount; cluster_index++) {
     endpoint_discovered.endpoint.cluster_list[cluster_index].cluster_id
       = endpointInfo->inClusterList[cluster_index];
   }
```
however, the cluster list is defined as 16 in in zigpc_common_zigbee.h:
```
#define ZIGPC_CLUSTER_LIST_SIZE 16
```
