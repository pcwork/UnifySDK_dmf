const fs = require('fs')
const path = require('path')

const DOTDOT_XML_DIR = path.join(__dirname, '..', 'dotdot-xml')
let typeAliasMap = null

function loadTypeAliasMap() {
  if (typeAliasMap !== null) {
    return typeAliasMap
  }

  typeAliasMap = {}
  const entries = fs.readdirSync(DOTDOT_XML_DIR, { withFileTypes: true })

  for (const entry of entries) {
    if (!entry.isFile() || !entry.name.endsWith('.xml')) {
      continue
    }

    const xml = fs.readFileSync(path.join(DOTDOT_XML_DIR, entry.name), 'utf8')
    const matches = xml.match(/<type:type\b[^>]*>/g) || []

    for (const tag of matches) {
      const shortMatch = tag.match(/\bshort="([^"]+)"/)
      const nameMatch = tag.match(/\bname="([^"]+)"/)
      const inheritsFromMatch = tag.match(/\binheritsFrom="([^"]+)"/)

      if (inheritsFromMatch) {
        if (shortMatch) {
          typeAliasMap[shortMatch[1]] = inheritsFromMatch[1]
        }
        if (nameMatch) {
          typeAliasMap[nameMatch[1]] = inheritsFromMatch[1]
        }
      }
    }
  }

  return typeAliasMap
}

function resolveBaseType(type, seen = new Set()) {
  const aliases = loadTypeAliasMap()
  let resolvedType = type

  while (aliases[resolvedType] && !seen.has(resolvedType)) {
    seen.add(resolvedType)
    resolvedType = aliases[resolvedType]
  }

  return resolvedType
}

module.exports = {
  resolveBaseType,
}
