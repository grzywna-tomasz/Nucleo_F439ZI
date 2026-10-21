const fs = require("node:fs");

function skipTrivia(text, start) {
  let index = start;
  while (index < text.length) {
    if (/\s/.test(text[index])) {
      index += 1;
    } else if (text.startsWith("//", index)) {
      const newline = text.indexOf("\n", index + 2);
      index = newline === -1 ? text.length : newline + 1;
    } else if (text.startsWith("/*", index)) {
      const end = text.indexOf("*/", index + 2);
      if (end === -1) {
        throw new Error("Unterminated block comment in workspace settings.");
      }
      index = end + 2;
    } else {
      break;
    }
  }
  return index;
}

function readString(text, start) {
  if (text[start] !== '"') {
    throw new Error(`Expected a JSON string at character ${start}.`);
  }
  let index = start + 1;
  while (index < text.length) {
    if (text[index] === "\\") {
      index += 2;
    } else if (text[index] === '"') {
      return { value: JSON.parse(text.slice(start, index + 1)), end: index + 1 };
    } else {
      index += 1;
    }
  }
  throw new Error("Unterminated string in workspace settings.");
}

function readValueEnd(text, start) {
  const first = text[start];
  if (first === '"' || first !== "{" && first !== "[") {
    if (first === '"') {
      return readString(text, start).end;
    }
    let index = start;
    while (index < text.length && !/[\s,}\]]/.test(text[index])) {
      index += 1;
    }
    if (index === start) {
      throw new Error(`Missing JSON value at character ${start}.`);
    }
    return index;
  }

  const closers = [];
  let index = start;
  while (index < text.length) {
    const char = text[index];
    if (char === '"') {
      index = readString(text, index).end;
      continue;
    }
    if (text.startsWith("//", index)) {
      const newline = text.indexOf("\n", index + 2);
      index = newline === -1 ? text.length : newline + 1;
      continue;
    }
    if (text.startsWith("/*", index)) {
      const end = text.indexOf("*/", index + 2);
      if (end === -1) {
        throw new Error("Unterminated block comment in workspace settings.");
      }
      index = end + 2;
      continue;
    }
    if (char === "{" || char === "[") {
      closers.push(char === "{" ? "}" : "]");
    } else if (char === "}" || char === "]") {
      if (closers.pop() !== char) {
        throw new Error(`Mismatched JSON delimiter at character ${index}.`);
      }
      if (closers.length === 0) {
        return index + 1;
      }
    }
    index += 1;
  }
  throw new Error("Unterminated JSON object or array in workspace settings.");
}

function parseRootProperties(text) {
  const start = skipTrivia(text, 0);
  if (text[start] !== "{") {
    throw new Error("Workspace settings must have a JSON object at the root.");
  }

  const properties = [];
  let index = skipTrivia(text, start + 1);
  if (text[index] === "}") {
    return { properties, closeBrace: index };
  }

  while (index < text.length) {
    const key = readString(text, index);
    index = skipTrivia(text, key.end);
    if (text[index] !== ":") {
      throw new Error(`Expected ':' after workspace setting "${key.value}".`);
    }
    const valueStart = skipTrivia(text, index + 1);
    const valueEnd = readValueEnd(text, valueStart);
    index = skipTrivia(text, valueEnd);
    const comma = text[index] === ",";
    properties.push({ key: key.value, valueStart, valueEnd, comma });
    index = skipTrivia(text, comma ? index + 1 : index);
    if (text[index] === "}") {
      return { properties, closeBrace: index };
    }
    if (!comma) {
      throw new Error(`Expected ',' or '}' after workspace setting "${key.value}".`);
    }
  }
  throw new Error("Unterminated root object in workspace settings.");
}

function updateJsoncSettings(text, updates) {
  let result = text;

  for (const [key, value] of Object.entries(updates)) {
    const property = parseRootProperties(result).properties.find((item) => item.key === key);
    const serialized = JSON.stringify(value, null, 4).replace(/\n/g, "\n    ");
    if (property) {
      result = `${result.slice(0, property.valueStart)}${serialized}${result.slice(property.valueEnd)}`;
    } else {
      const currentRoot = parseRootProperties(result);
      const closeBrace = currentRoot.closeBrace;
      let insertAt = closeBrace;
      while (insertAt > 0 && /\s/.test(result[insertAt - 1])) {
        insertAt -= 1;
      }
      const hasProperties = currentRoot.properties.length > 0;
      const lastProperty = currentRoot.properties.at(-1);
      const comma = hasProperties && !lastProperty.comma ? "," : "";
      const indent = text.match(/(?:^|\n)([ \t]+)"[^"]+"\s*:/)?.[1] ?? "    ";
      const propertyIndent = indent;
      const valueIndent = `${indent}    `;
      const propertyValue = JSON.stringify(value, null, 4).replace(/\n/g, `\n${valueIndent}`);
      const insertion = `${comma}\n${propertyIndent}${JSON.stringify(key)}: ${propertyValue}`;
      result = `${result.slice(0, insertAt)}${insertion}${result.slice(insertAt)}`;
    }
  }
  return result;
}

function updateSettingsFile(settingsPath, updates) {
  const original = fs.readFileSync(settingsPath, "utf8");
  const updated = updateJsoncSettings(original, updates);
  if (updated !== original) {
    fs.writeFileSync(settingsPath, updated);
  }
  return updated !== original;
}

module.exports = { parseRootProperties, updateJsoncSettings, updateSettingsFile };
