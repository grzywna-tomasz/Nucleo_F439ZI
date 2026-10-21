const assert = require("node:assert/strict");
const os = require("node:os");
const path = require("node:path");
const test = require("node:test");
const { parseRootProperties, updateJsoncSettings } = require("../settings");
const {
  collectBrowsePaths,
  findCompileDatabases,
  resolveConfiguredPath,
  tokenizeCommand
} = require("../sync");

test("tokenizes compiler arguments while preserving quoted paths", () => {
  assert.deepEqual(
    tokenizeCommand('arm-none-eabi-gcc -I"include folder" -isystem /sdk/include -DVALUE=1'),
    ["arm-none-eabi-gcc", "-Iinclude folder", "-isystem", "/sdk/include", "-DVALUE=1"]
  );
});

test("derives browse paths from source and include arguments", () => {
  const workspace = path.resolve("/workspace/project");
  const paths = collectBrowsePaths([{
    directory: path.join(workspace, "build"),
    file: "../src/main.c",
    command: 'arm-none-eabi-gcc -I../inc -isystem "/sdk headers" -c ../src/main.c'
  }], workspace);

  assert.deepEqual(paths, [
    "/sdk headers",
    "${workspaceFolder}/inc",
    "${workspaceFolder}/src"
  ]);
});

test("finds compile databases and sorts newest first", async () => {
  const root = await require("node:fs").promises.mkdtemp(path.join(os.tmpdir(), "cmake-db-"));
  try {
    const debug = path.join(root, "Debug");
    const release = path.join(root, "Release");
    await require("node:fs").promises.mkdir(debug, { recursive: true });
    await require("node:fs").promises.mkdir(release, { recursive: true });
    await require("node:fs").promises.writeFile(path.join(debug, "compile_commands.json"), "[]");
    await new Promise((resolve) => setTimeout(resolve, 15));
    await require("node:fs").promises.writeFile(path.join(release, "compile_commands.json"), "[]");

    const databases = await findCompileDatabases(root);
    assert.equal(databases.length, 2);
    assert.equal(path.basename(path.dirname(databases[0].path)), "Release");
  } finally {
    await require("node:fs").promises.rm(root, { recursive: true, force: true });
  }
});

test("expands workspace-relative database settings", () => {
  const workspace = path.resolve("/workspace/project");
  assert.equal(
    resolveConfiguredPath("${workspaceFolder}/build/Debug/compile_commands.json", workspace),
    path.join(workspace, "build", "Debug", "compile_commands.json")
  );
});

test("updates existing JSONC settings and preserves comments", () => {
  const original = `{
    // Keep this workspace's include list.
    "C_Cpp.default.includePath": ["src"],
    "C_Cpp.default.browse.path": ["old"]
}`;
  const updated = updateJsoncSettings(original, {
    "C_Cpp.default.browse.path": ["src", "include"],
    "C_Cpp.default.compileCommands": "\${workspaceFolder}/build/Debug/compile_commands.json"
  });
  const keys = parseRootProperties(updated).properties.map((property) => property.key);

  assert.match(updated, /\/\/ Keep this workspace's include list\./);
  assert.deepEqual(keys, [
    "C_Cpp.default.includePath",
    "C_Cpp.default.browse.path",
    "C_Cpp.default.compileCommands"
  ]);
  assert.match(updated, /"C_Cpp\.default\.browse\.path": \[\n        "src",\n        "include"\n    \]/);
});

test("adds settings to an object with a trailing comma", () => {
  const updated = updateJsoncSettings('{\n    "existing": true,\n}', {
    "new.setting": false
  });
  assert.deepEqual(parseRootProperties(updated).properties.map((property) => property.key), [
    "existing",
    "new.setting"
  ]);
});

test("updates multiple existing settings without stale source offsets", () => {
  const updated = updateJsoncSettings(`{
    "first": "short",
    "second": ["old"]
}`, {
    first: "a substantially longer replacement",
    second: ["new", "values"]
  });
  const properties = parseRootProperties(updated).properties;
  assert.deepEqual(properties.map((property) => property.key), ["first", "second"]);
  assert.match(updated, /"first": "a substantially longer replacement"/);
  assert.match(updated, /"second": \[\n        "new",\n        "values"\n    \]/);
});
