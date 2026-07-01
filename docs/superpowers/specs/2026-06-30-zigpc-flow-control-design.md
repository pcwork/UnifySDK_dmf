# ZigPC flow control runtime configuration

## Summary

Add a ZigPC runtime configuration option that controls the EZSP host serial flow
control mode used when connecting to the Zigbee NCP. The option must work from
both command line arguments and `uic.cfg`, preserve the current default
behavior, and wire cleanly into the existing ZigPC gateway setup path.

## Goals

- Allow operators to choose ZigPC serial flow control without patching the SDK.
- Support both CLI and config-file configuration through the existing ZigPC
  config system.
- Keep Silicon Labs EZSP host code as the source of truth for actual serial
  setup.
- Preserve existing behavior when the new option is not configured.

## Non-goals

- Changing ZigPC baud-rate behavior.
- Replacing or rewriting the SDK EZSP host serial initialization.
- Adding raw passthrough of arbitrary EZSP host flags.

## User-facing behavior

Introduce a new typed runtime option:

- Key: `zigpc.flow_control`
- Allowed values: `hardware`, `software`
- Default: `hardware`

Examples:

```bash
zigpc --zigpc.serial /dev/ttyACM0 --zigpc.flow_control hardware
zigpc --zigpc.serial /dev/ttyUSB0 --zigpc.flow_control software
```

```yaml
zigpc:
  - serial: /dev/ttyUSB0
  - flow_control: software
```

`hardware` means ZigPC asks the EZSP host stack to use RTS/CTS. `software`
means ZigPC asks the EZSP host stack to use XON/XOFF. Baud rate remains
unchanged in both cases.

## Design

### 1. ZigPC configuration layer

Add a new config key in `zigpc_config` and store the parsed value in
`zigpc_config_t`.

Implementation shape:

- Add `CONFIG_KEY_ZIGPC_FLOW_CONTROL = "zigpc.flow_control"`.
- Add a small ZigPC enum for the parsed value, for example:
  `ZIGPC_FC_HARDWARE` and `ZIGPC_FC_SOFTWARE`.
- Register the option in `zigpc_config_init()` as a string with default
  `hardware`.
- Parse and validate the string during `zigpc_config_fixt_setup()`.

Validation rules:

- `hardware` maps to the hardware-flow-control enum.
- `software` maps to the software-flow-control enum.
- Any other value is rejected through the existing config error path; ZigPC must
  not silently fall back to a different mode.

### 2. Gateway setup wiring

Extend the internal Zigbee host wrapper options so gateway setup can pass the
configured choice into the EZSP host layer.

Implementation shape:

- Extend `struct zigbeeHostOpts` with a flow-control field that uses a small
  wrapper enum.
- In `zigpc_gateway_process_setup()`, translate `zigpc_config->flow_control`
  into the wrapper enum and populate `z3gw_opts`.
- Keep `zigpc.serial`, OTA path, callbacks, and cluster registration behavior
  unchanged.

This keeps ZigPC-specific config parsing in ZigPC, and Zigbee host option
translation in the gateway/wrapper boundary where it already belongs.

### 3. EZSP host argument mapping

Update `zigbeeHostInit()` to translate the new wrapper enum into EZSP host
command options before calling `sl_zigbee_ezsp_process_command_options()`.

Mapping:

- `hardware` -> `-f r`
- `software` -> `-f x`

Only the missing flow-control argument is added. ZigPC will continue to pass the
serial port through `-p` and the OTA path through `-d`.

This deliberately avoids editing SDK serial initialization logic. The SDK
remains responsible for applying termios settings, handling platform quirks, and
deciding whether runtime serial setup succeeds.

## Error handling

- Invalid `zigpc.flow_control` values fail configuration setup instead of being
  ignored.
- Unknown internal enum values in the gateway/wrapper boundary should produce a
  ZigPC failure instead of silently selecting a default.
- Existing EZSP host init failures remain unchanged and continue to surface as
  gateway setup failures.

## Testing

Add targeted coverage in existing ZigPC unit tests:

1. Config parsing tests for:
   - default value -> `hardware`
   - explicit `hardware`
   - explicit `software`
   - invalid string rejection
2. Gateway or wrapper tests for EZSP argument mapping:
   - `hardware` produces `-f r`
   - `software` produces `-f x`
3. Regression coverage that existing serial-port setup still passes `-p`.

## Documentation

Update ZigPC user-facing documentation to mention the new option in:

- command-line examples
- config-file examples
- any setup notes that currently mention only `zigpc.serial`

## Rationale

This design gives operators a stable ZigPC-facing configuration surface without
exposing low-level EZSP host argument details. It follows existing repo
patterns, keeps the SDK integration narrow, and makes flow-control behavior
explicit at the place where ZigPC already gathers connection settings for the
NCP.
