const fs = require("node:fs");
const path = require("node:path");

function tokenizeCommand(command) {
  const args = [];
  let token = "";
  let quote = null;
  let active = false;

  for (let i = 0; i < command.length; i += 1) {
    const char = command[i];
    if (quote === "'") {
      if (char === "'") {
        quote = null;
      } else {
        token += char;
      }
      active = true;
      continue;
    }

    if (quote === '"') {
      if (char === '"') {
        quote = null;
      } else if (char === "\\" && i + 1 < command.length && ['"', "\\"].includes(command[i + 1])) {
        token += command[++i];
      } else {
        token += char;
      }
      active = true;
      continue;
    }

    if (char === "'" || char === '"') {
      quote = char;
      active = true;
    } else if (char === "\\" && i + 1 < command.length && /\s|["'\\]/.test(command[i + 1])) {
      token += command[++i];
      active = true;
    } else if (/\s/.test(char)) {
      if (active) {
        args.push(token);
        token = "";
        active = false;
      }
    } else {
      token += char;
      active = true;
    }
  }

  if (quote !== null) {
    throw new Error("Unterminated quote in a compile command.");
  }
  if (active) {
    args.push(token);
  }
  return args;
}

function getEntryArguments(entry) {
  if (Array.isArray(entry.arguments)) {
    return entry.arguments;
  }
  if (typeof entry.command === "string") {
    return tokenizeCommand(entry.command);
  }
  throw new Error(`Compile database entry for "${entry.file}" has no command or arguments.`);
}

function readCompileDatabase(databasePath) {
  const entries = JSON.parse(fs.readFileSync(databasePath, "utf8"));
  if (!Array.isArray(entries) || entries.length === 0) {
    throw new Error(`Compile database is empty or invalid: ${databasePath}`);
  }
  return entries;
}

function collectBrowsePaths(entries, workspaceFolder) {
  const paths = new Set();
  const includeOptions = new Set(["-I", "-isystem", "-iquote", "-idirafter"]);

  for (const entry of entries) {
    if (typeof entry.directory !== "string" || typeof entry.file !== "string") {
      throw new Error("Compile database entries must contain directory and file paths.");
    }

    const directory = path.resolve(entry.directory);
    paths.add(path.dirname(path.resolve(directory, entry.file)));

    const args = getEntryArguments(entry);
    for (let i = 0; i < args.length; i += 1) {
      let includePath;
      if (includeOptions.has(args[i])) {
        includePath = args[++i];
      } else {
        const attachedOption = [...includeOptions].find((option) =>
          option !== "-I" && args[i].startsWith(option) && args[i].length > option.length
        );
        if (attachedOption) {
          includePath = args[i].slice(attachedOption.length);
        } else if (args[i].startsWith("-I") && args[i].length > 2) {
          includePath = args[i].slice(2);
        }
      }

      if (includePath) {
        paths.add(path.resolve(directory, includePath));
      }
    }
  }

  return [...paths]
    .map((item) => toWorkspaceSettingPath(item, workspaceFolder))
    .sort((left, right) => left.localeCompare(right));
}

function toWorkspaceSettingPath(absolutePath, workspaceFolder) {
  const relativePath = path.relative(workspaceFolder, absolutePath);
  if (relativePath === "" || (!relativePath.startsWith(`..${path.sep}`) && relativePath !== ".." && !path.isAbsolute(relativePath))) {
    return relativePath === "" ? "${workspaceFolder}" : `\${workspaceFolder}/${relativePath.split(path.sep).join("/")}`;
  }
  return absolutePath;
}

function resolveConfiguredPath(configuredPath, workspaceFolder) {
  const expanded = configuredPath.replace(/\$\{workspaceFolder\}/g, workspaceFolder);
  return path.resolve(workspaceFolder, expanded);
}

async function findCompileDatabases(buildDirectory) {
  const results = [];

  async function visit(directory) {
    let entries;
    try {
      entries = await fs.promises.readdir(directory, { withFileTypes: true });
    } catch (error) {
      if (error.code === "ENOENT" || error.code === "ENOTDIR") {
        return;
      }
      throw error;
    }

    for (const entry of entries) {
      const entryPath = path.join(directory, entry.name);
      if (entry.isFile() && entry.name === "compile_commands.json") {
        const stat = await fs.promises.stat(entryPath);
        results.push({ path: entryPath, mtimeMs: stat.mtimeMs });
      } else if (entry.isDirectory()) {
        await visit(entryPath);
      }
    }
  }

  await visit(buildDirectory);
  return results.sort((left, right) => right.mtimeMs - left.mtimeMs || left.path.localeCompare(right.path));
}

module.exports = {
  collectBrowsePaths,
  findCompileDatabases,
  readCompileDatabase,
  resolveConfiguredPath,
  toWorkspaceSettingPath,
  tokenizeCommand
};
