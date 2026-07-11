const helpers = require('./addon-helper')
const type_alias_helpers = require('./type-alias-helper')
let zclTypeMap = {}

function zcl_type_map_set_item(label, type) {
  zclTypeMap[label] = type_alias_helpers.resolveBaseType(type)
  return ''
}

function zcl_type_map_get_item(label) {
  return zclTypeMap[label] || type_alias_helpers.resolveBaseType(label) || label
}

function ident() {
  return ''
}

function after(options) {
  return options.fn(this)
}

function isEqual(left, right) {
  return left === right
}

function asHex(value, width) {
  const numericValue = Number(value)
  if (!Number.isFinite(numericValue)) {
    return value
  }

  return '0x' + numericValue.toString(16).toUpperCase().padStart(width, '0')
}

function asDelimitedMacro(value) {
  return helpers.asSnakeCaseUpper(value)
}

function asUnderlyingType(type) {
  const resolvedType = type_alias_helpers.resolveBaseType(type)

  // Preserve named enum aliases (for example BatterySize, DrlkOperMode,
  // DiscoveryStatus) so generated APIs keep the typedef'ed enum type instead of
  // collapsing to the raw integer storage type.
  if (type !== resolvedType && helpers.isEnum && helpers.isEnum(type)) {
    return type
  }

  switch (resolvedType) {
    case 'bool':
      return 'bool'
    case 'single':
      return 'float'
    case 'double':
      return 'double'
    case 'semi':
      return 'uint16_t'
    case 'data8':
    case 'map8':
    case 'uint8':
    case 'enum8':
      return 'uint8_t'
    case 'data16':
    case 'map16':
    case 'uint16':
    case 'enum16':
    case 'clusterId':
    case 'attribId':
      return 'uint16_t'
    case 'data24':
    case 'map24':
    case 'uint24':
      return 'uint32_t'
    case 'data32':
    case 'map32':
    case 'uint32':
    case 'UTC':
      return 'uint32_t'
    case 'data40':
    case 'map40':
    case 'uint40':
    case 'data48':
    case 'map48':
    case 'uint48':
    case 'data56':
    case 'map56':
    case 'uint56':
    case 'data64':
    case 'map64':
    case 'uint64':
    case 'bacOID':
      return 'uint64_t'
    case 'int8':
      return 'int8_t'
    case 'int16':
      return 'int16_t'
    case 'int24':
    case 'int32':
      return 'int32_t'
    case 'int40':
    case 'int48':
    case 'int56':
    case 'int64':
      return 'int64_t'
    case 'octstr':
    case 'string':
    case 'octstr16':
    case 'string16':
      return 'char'
    case 'EUI64':
      return 'zigbee_eui64_t'
    default:
      if (helpers.isStruct && helpers.isStruct(type)) {
        return `zigpc_zcl_${helpers.asSnakeCaseLower(type)}_t`
      }
      if (helpers.isEnum && helpers.isEnum(resolvedType)) {
        return helpers.enumType(resolvedType)
      }
      return resolvedType
  }
}

module.exports = {
  ...helpers,
  after,
  ident,
  isEqual,
  asHex,
  asDelimitedMacro,
  asUnderlyingType,
  zcl_type_map_set_item,
  zcl_type_map_get_item,
}
