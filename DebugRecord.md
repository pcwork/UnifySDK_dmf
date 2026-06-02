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

## Fix Docker build errors for UnifySDK (debian:bookworm)

### Error 1: LLVM LTO version mismatch

**Symptom:**
```
bfd plugin: LLVM gold plugin has failed to create LTO module:
Opaque pointers are only supported in -opaque-pointers mode
(Producer: 'LLVM16.0.5-rust-1.71.0-stable' Reader: 'LLVM 14.0.6')
```

**Root cause:** Rust 1.71.0 ships with LLVM 16, but `debian:bookworm` provides LLVM 14 gold linker plugin. Cargo.toml has `lto = true` enabled, causing the version conflict.

**Fix:** Set environment variable in the Docker container before building:
```
export RUSTFLAGS="-C linker-plugin-lto=false"
```
This tells Rust to perform LTO internally instead of relying on the system's gold plugin. LTO optimization is still preserved.

### Error 2: Clock skew detected

**Symptom:**
```
make: warning: Clock skew detected. Your build may be incomplete.
ninja: build stopped: subcommand failed.
```

**Root cause:** File timestamps on Windows host volume mount are ahead of the container's system clock.

**Fix:** Mount the host's timezone file in `docker run`:
```
docker run -it --rm \
  --env GSDK_LOCATION \
  -v `pwd`:`pwd` \
  -w `pwd` \
  -v ${GSDK_LOCATION}:${GSDK_LOCATION} \
  -v /etc/localtime:/etc/localtime:ro \
  uic_arm64
```

### Error 3: React dev_gui TypeScript compilation failure

**Symptom:**
```
TS2786: 'BrowserRouter' cannot be used as a JSX component.
Property 'refs' is missing in type 'Component<BrowserRouterProps, any, any>'
```

**Root cause:** `react-router-dom` BrowserRouter incompatible with the installed `@types/react` version.

**Workaround:** Skip dev_gui and build only the desired target:
```
cd build && ninja zigpc
```

### Error 4: CPack install fails due to missing dev_gui artifacts

**Symptom:**
```
CPack Error: file INSTALL cannot find "dev-gui-api" / "dev-gui-client"
```

**Root cause:** `cpack -D CPackComponentsToInstall=uic-zigpc -G DEB` still runs the install step for ALL components, not just the selected one. dev_gui build failure means its output files don't exist.

**Fix:** Create empty placeholder files before running cpack:
```
touch build/applications/dev_ui/dev_gui/dev-gui-api
touch build/applications/dev_ui/dev_gui/dev-gui-client
cpack -D CPackComponentsToInstall=uic-zigpc -G DEB
```

### Complete build command for zigpc deb package in Docker

```
docker run -it --rm \
  --env GSDK_LOCATION \
  -v `pwd`:`pwd` \
  -w `pwd` \
  -v ${GSDK_LOCATION}:${GSDK_LOCATION} \
  -v /etc/localtime:/etc/localtime:ro \
  uic_arm64 \
  bash -c "export RUSTFLAGS='-C linker-plugin-lto=false' && cd build && ninja zigpc && touch applications/dev_ui/dev_gui/dev-gui-api applications/dev_ui/dev_gui/dev-gui-client && cpack -D CPackComponentsToInstall=uic-zigpc -G DEB"
```
