const fs = require("fs");
const vm = require("vm");
const game = "D:/MikeAndDenyse/game/js";
const code =
  fs.readFileSync(game + "/data.js", "utf8") +
  "\n" +
  fs.readFileSync(game + "/levels.js", "utf8") +
  `
const out = [];
for (let i = 0; i < 16; i++) {
  const L = compileLevel(i, false);
  if (!L.spawn) throw new Error("no spawn " + i);
  if (!L.bossAt) throw new Error("no boss " + i);
  if (!L.exit) throw new Error("no exit " + i);
  out.push(
    (i + 1) + ". " + WORLDS[i].name +
    " cols=" + L.cols +
    " inimigos=" + L.ents.length +
    " itens=" + L.items.length +
    " chefe=" + L.world.boss
  );
}
out;
`;
const r = vm.runInNewContext(code, {});
console.log(r.join("\n"));
console.log("OK 16 fases");
