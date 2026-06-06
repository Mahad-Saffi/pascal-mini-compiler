"use strict";

const SVG_NS = "http://www.w3.org/2000/svg";

const STAGES = [
  { id: "tokens",  label: "Tokens",           cmd: "lex",          kind: "text" },
  { id: "asttree", label: "AST (tree)",       cmd: "dot",          kind: "tree" },
  { id: "asttext", label: "AST (text)",       cmd: "rdp",          kind: "text" },
  { id: "ll1",     label: "Predictive LL(1)", cmd: "ll1",          kind: "text" },
  { id: "lr",      label: "SLR(1) LR",        cmd: "lr",           kind: "text" },
  { id: "symtab",  label: "Symbol table",     cmd: "symtab",       kind: "text" },
  { id: "semant",  label: "Semantic",         cmd: "semant",       kind: "text" },
  { id: "grammar", label: "Grammar",          cmd: "grammar",      kind: "text" },
  { id: "ff",      label: "FIRST / FOLLOW",   cmd: "first-follow", kind: "text" },
  { id: "ll1t",    label: "LL(1) table",      cmd: "ll1-table",    kind: "text" },
  { id: "slrt",    label: "SLR table",        cmd: "slr-table",    kind: "text" },
];

const codeEl = document.getElementById("code");
const statusEl = document.getElementById("status");
const textEl = document.getElementById("text");
const treeEl = document.getElementById("tree");
const svg = document.getElementById("svg");
const stagesEl = document.getElementById("stages");
const sampleSel = document.getElementById("sample");
const loadBtn = document.getElementById("load");
const resultEl = document.querySelector(".result");
const structuredEl = document.getElementById("structured");
const viewbarEl = document.getElementById("viewbar");

const stageButtons = new Map();
let running = false;
let activeStage = null;
let loadedCode = "";

function clamp(value, low, high) {
  return Math.max(low, Math.min(high, value));
}

function buildStageButtons() {
  for (const stage of STAGES) {
    const button = document.createElement("button");
    button.textContent = stage.label;
    button.addEventListener("click", () => runStage(stage));
    stagesEl.appendChild(button);
    stageButtons.set(stage.id, button);
  }
}

function setActiveStage(id) {
  for (const [key, button] of stageButtons) {
    button.classList.toggle("active", key === id);
  }
}

function setBusy(on) {
  running = on;
  statusEl.classList.toggle("busy", on);
  resultEl.setAttribute("aria-busy", on ? "true" : "false");
}

async function runStage(stage) {
  if (running) return;
  activeStage = stage;
  setActiveStage(stage.id);
  setBusy(true);
  statusEl.textContent = "running " + stage.label + "…";
  let data;
  try {
    const response = await fetch("/run", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ cmd: stage.cmd, code: codeEl.value }),
    });
    data = await response.json();
  } catch (error) {
    statusEl.textContent = "server error";
    showRawText("Could not reach the server.\n" + error);
    return;
  } finally {
    setBusy(false);
  }
  statusEl.textContent = "exit " + data.exit + " · " + stage.label;
  showStage(stage, data);
}

function viewText() { showOnly("text"); }
function viewTree() { showOnly("tree"); }

function escapeHtml(text) {
  return text.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
}

function colorize(text) {
  return text.split("\n").map((line) => {
    const cls = severityClass(line);
    const escaped = escapeHtml(line);
    return cls ? '<span class="' + cls + '">' + escaped + "</span>" : escaped;
  }).join("\n");
}

// Map one line of compiler output to a severity colour. Grounded in the real
// strings bin/minipascal emits: the "Errors: N (lexical .. semantic ..)"
// summary, the "  [semantic] line ..." diagnostics, "No errors.", and the
// parsers' "Result: ACCEPTED/REJECTED ..." verdicts. Good news is matched
// first so "No errors." and "Errors: 0" never read as failures.
function severityClass(line) {
  if (/\bno (errors|warnings)\b/i.test(line)
      || /\berrors?:\s*0\b/i.test(line)
      || /\bACCEPTED\b/.test(line)
      || /\bPASSED\b|all three agree/.test(line))
    return "good";
  if (/\berrors?\b/i.test(line)
      || /^\s*\[(lexical|syntactic|semantic)\]/i.test(line)
      || /\bREJECTED\b|DISAGREEMENT|FAILED/.test(line))
    return "err";
  if (/\bwarnings?\b/i.test(line) || /^\s*\[warning\]/i.test(line))
    return "warn";
  return "";
}

function showText(data) {
  viewText();
  let body = data.stdout || "";
  if (data.stderr) body += (body && !body.endsWith("\n") ? "\n" : "") + data.stderr;
  textEl.innerHTML = colorize(body || "(no output)");
}

function showRawText(text) {
  viewbarEl.hidden = true;
  viewText();
  textEl.textContent = text;
}

// ---- Structured stage views -------------------------------------------------
// Several stages print fixed-width columnar tables (token stream, symbol table,
// FIRST/FOLLOW, the LL(1) and SLR parsing tables). On a projector those read as
// a wall of monospace. We re-parse that exact stdout into real HTML grids and
// offer a Table / Raw toggle. Parsing is best-effort: if a renderer extracts
// nothing, or the run exited non-zero (so its stdout carries diagnostics we must
// not hide), showStage falls back to the raw colourised text. The Table view is
// a reformat of the compiler's own output, never a reinterpretation of it.

let viewMode = "table";
let lastData = null;

function h(tag, attrs, kids) {
  const el = document.createElement(tag);
  if (attrs) {
    for (const key in attrs) {
      if (key === "class") el.className = attrs[key];
      else if (key === "text") el.textContent = attrs[key];
      else el.setAttribute(key, attrs[key]);
    }
  }
  if (kids != null) {
    for (const kid of Array.isArray(kids) ? kids : [kids]) {
      if (kid == null) continue;
      el.appendChild(kid instanceof Node ? kid : document.createTextNode(String(kid)));
    }
  }
  return el;
}

function dtable(headers, rows, aligns) {
  const head = h("tr", null, headers.map((label, i) =>
    h("th", aligns && aligns[i] === "r" ? { class: "num" } : null, label)));
  const body = rows.map((row) =>
    h("tr", null, row.map((cell, i) =>
      h("td", aligns && aligns[i] === "r" ? { class: "num" } : null,
        cell instanceof Node ? cell : String(cell)))));
  return h("table", { class: "dtable" }, [h("thead", null, head), h("tbody", null, body)]);
}

function dwrap(captionText, ...nodes) {
  return h("div", { class: "dwrap" }, [h("div", { class: "dcaption", text: captionText }), ...nodes]);
}

function renderTokens(text) {
  const rowRe = /^\s*(\d+)\s+(\d+)\s+(\S+)\s+(.*\S)\s+(\S+)\s*$/;
  const rows = [];
  for (const line of text.split("\n")) {
    const m = rowRe.exec(line);
    if (m) {
      rows.push([m[1], m[2], h("span", { class: "tok-type" }, m[3]),
        h("span", { class: "sym-name" }, m[4]), h("code", { class: "term" }, m[5])]);
    }
  }
  if (!rows.length) return null;
  return dwrap(rows.length + " tokens",
    dtable(["Line", "Col", "Type", "Lexeme", "Terminal"], rows, ["r", "r"]));
}

function renderSymtab(text) {
  const scopeRe = /^(\s*)scope (\d+)(?: \((\w+)\))?:\s*$/;
  const entryRe = /^(\s*)(\S+)\s+\[(\w+)\]\s+(.*?)(?:\s+\(line (\d+)\))?\s*$/;
  const root = h("div", { class: "symroot" });
  const stack = [{ indent: -1, entries: [], el: root }];
  let total = 0;

  function flush(scope) {
    if (!scope.entries.length) return;
    const rows = scope.entries.map((e) => [
      h("span", { class: "sym-name" }, e.name),
      h("code", { class: "sym-kind" }, e.kind),
      e.type || "", e.line || "",
    ]);
    scope.el.appendChild(dtable(["Name", "Kind", "Type", "Line"], rows, [null, null, null, "r"]));
    scope.entries = [];
  }

  for (const line of text.split("\n")) {
    const sm = scopeRe.exec(line);
    if (sm) {
      const indent = sm[1].length;
      while (stack.length > 1 && indent <= stack[stack.length - 1].indent) {
        flush(stack[stack.length - 1]); stack.pop();
      }
      const parent = stack[stack.length - 1];
      flush(parent);
      const box = h("div", { class: "scope" });
      box.appendChild(h("div", { class: "scope-head" }, [
        h("span", { class: "scope-name" }, "scope " + sm[2]),
        sm[3] ? h("span", { class: "scope-tag" }, sm[3]) : null,
      ]));
      parent.el.appendChild(box);
      stack.push({ indent, entries: [], el: box });
      continue;
    }
    const em = entryRe.exec(line);
    if (em) {
      stack[stack.length - 1].entries.push({
        name: em[2], kind: em[3], type: em[4], line: em[5] || "",
      });
      total++;
    }
  }
  while (stack.length > 1) { flush(stack[stack.length - 1]); stack.pop(); }
  if (!total) return null;
  return dwrap(total + " symbols", root);
}

function renderFirstFollow(text) {
  const firstRe = /^(<[^>]+>)\s+FIRST\s*=\s*\{(.*)\}\s*$/;
  const followRe = /^\s+FOLLOW\s*=\s*\{(.*)\}\s*$/;
  const rows = [];
  let current = null;
  for (const line of text.split("\n")) {
    const fm = firstRe.exec(line);
    if (fm) { current = { nt: fm[1], first: fm[2].trim(), follow: "" }; rows.push(current); continue; }
    const lm = followRe.exec(line);
    if (lm && current) current.follow = lm[1].trim();
  }
  if (!rows.length) return null;
  const tableRows = rows.map((r) => [
    h("code", { class: "nt" }, r.nt),
    h("code", { class: "set" }, "{ " + r.first + " }"),
    h("code", { class: "set" }, "{ " + r.follow + " }"),
  ]);
  return dwrap(rows.length + " nonterminals",
    dtable(["Nonterminal", "FIRST", "FOLLOW"], tableRows));
}

function renderLL1Table(text) {
  const ntRe = /^(<[^>]+>)\s*$/;
  const entryRe = /^\s+on\s+(.+?)\s+=>\s+(.*?)\s*$/;
  const groups = [];
  let current = null;
  for (const line of text.split("\n")) {
    const nm = ntRe.exec(line);
    if (nm) { current = { nt: nm[1], rows: [] }; groups.push(current); continue; }
    const em = entryRe.exec(line);
    if (em && current) {
      current.rows.push([h("code", { class: "term" }, em[1]), h("code", { class: "prod" }, em[2])]);
    }
  }
  const filled = groups.filter((g) => g.rows.length);
  if (!filled.length) return null;
  const wrap = h("div", { class: "ll1-groups" });
  for (const g of filled) {
    wrap.appendChild(h("div", { class: "ll1-group" }, [
      h("code", { class: "nt-head" }, g.nt),
      dtable(["On", "Produces"], g.rows),
    ]));
  }
  return dwrap(filled.length + " nonterminals", wrap);
}

// The slr-table stage prints two sections: the canonical LR(0) item sets (each
// state lists its items and "on SYM -> State N" gotos) followed by the ACTION /
// GOTO parsing tables (each state lists "ACTION[t] = shift/reduce/accept" and
// "GOTO[N] = State N"). State numbers repeat across the two sections, so cards
// are keyed by section index to keep ids unique and navigation in-section.
function renderSLRTable(text) {
  const sectionRe = /^SLR\(1\)\s+(.*\S)\s*$/;
  const stateRe = /^State (\d+):\s*$/;
  const transRe = /^\s+on\s+(.+?)\s+->\s+State (\d+)\s*$/;
  const gotoRe = /^\s+GOTO\[(.+?)\]\s*=\s*State (\d+)\s*$/;
  const actionRe = /^\s+ACTION\[(.+?)\]\s*=\s*(.+?)\s*$/;

  const sections = [];
  let section = null, state = null;
  for (const line of text.split("\n")) {
    const sec = sectionRe.exec(line);
    if (sec) { section = { title: sec[1], states: [] }; sections.push(section); state = null; continue; }
    const st = stateRe.exec(line);
    if (st) {
      if (!section) { section = { title: "", states: [] }; sections.push(section); }
      state = { n: st[1], items: [], rows: [] };
      section.states.push(state);
      continue;
    }
    if (!state) continue;
    const tr = transRe.exec(line);
    if (tr) { state.rows.push({ sym: tr[1], kind: "state", to: tr[2] }); continue; }
    const go = gotoRe.exec(line);
    if (go) { state.rows.push({ sym: go[1], kind: "state", to: go[2] }); continue; }
    const ac = actionRe.exec(line);
    if (ac) {
      const shift = /^shift (\d+)$/.exec(ac[2]);
      state.rows.push(shift ? { sym: ac[1], kind: "shift", to: shift[1] }
                            : { sym: ac[1], kind: "text", value: ac[2] });
      continue;
    }
    if (/\S/.test(line) && line.indexOf("->") !== -1) state.items.push(line.trim());
  }

  const real = sections.filter((s) => s.states.length);
  if (!real.length) return null;

  function gotoLink(si, to) {
    const link = h("a", { class: "lr-goto", href: "#lr-s" + si + "-" + to }, "State " + to);
    link.addEventListener("click", (event) => {
      event.preventDefault();
      const target = document.getElementById("lr-s" + si + "-" + to);
      if (target) target.scrollIntoView({ behavior: "smooth", block: "start" });
    });
    return link;
  }

  const out = h("div", { class: "lr-sections" });
  real.forEach((sec, si) => {
    const grid = h("div", { class: "lr-states" });
    for (const st of sec.states) {
      const children = [h("div", { class: "lr-state-head", text: "State " + st.n })];
      if (st.items.length) {
        children.push(h("ul", { class: "lr-items" },
          st.items.map((it) => h("li", null, h("code", null, it)))));
      }
      if (st.rows.length) {
        const rows = st.rows.map((r) => {
          let cell;
          if (r.kind === "state") cell = gotoLink(si, r.to);
          else if (r.kind === "shift") cell = h("span", null, ["shift ", gotoLink(si, r.to)]);
          else cell = h("code", { class: "prod" }, r.value);
          return [h("code", { class: "sym" }, r.sym), cell];
        });
        children.push(dtable(["On", st.items.length ? "Goto" : "Action"], rows));
      }
      grid.appendChild(h("div", { class: "lr-state", id: "lr-s" + si + "-" + st.n }, children));
    }
    out.appendChild(h("div", { class: "lr-section" },
      sec.title ? [h("div", { class: "lr-section-head", text: sec.title }), grid] : [grid]));
  });

  const total = real.reduce((n, s) => n + s.states.length, 0);
  return dwrap(real.length > 1 ? real.length + " sections · " + total + " states" : total + " states", out);
}

// The ll1 and lr stages print a fixed-width parse trace whose columns are placed
// by character offset, not whitespace: a truncated INPUT cell can run straight
// into ACTION ("... ) ...shift 2"), so splitting on spaces is impossible. We read
// each column's start offset from the header row's labels and slice every row at
// those same offsets -- a faithful reformat of the compiler's own columns.
function traceTable(text, anchors, headers, actionCol, widths) {
  const lines = text.split("\n");
  const hi = lines.findIndex((l) => anchors.every((a) => l.includes(a)));
  if (hi < 0) return null;
  const bounds = anchors.map((a) => lines[hi].indexOf(a));
  let start = hi + 1;
  if (/^\s*-{3,}\s*$/.test(lines[start] || "")) start++;
  const result = (lines.find((l) => /^Result:/.test(l)) || "")
    .replace(/^Result:\s*/, "").replace(/\.\s*$/, "").trim();
  const rows = [];
  for (let i = start; i < lines.length; i++) {
    const line = lines[i];
    if (/^Result:/.test(line)) break;
    if (!line.trim()) { if (rows.length) break; continue; }
    rows.push(bounds.map((b, k) =>
      line.slice(b, k + 1 < bounds.length ? bounds[k + 1] : undefined).trim()));
  }
  if (!rows.length) return null;
  const trows = rows.map((cells) => cells.map((c, k) =>
    k === actionCol ? traceAction(c) : h("code", { class: "trace-cell" }, c)));
  // Fixed layout so the long "reduce <..>" actions wrap inside the panel instead
  // of forcing a horizontal scroll that clips the Action column.
  const table = dtable(headers, trows);
  table.classList.add("trace-table");
  const colgroup = document.createElement("colgroup");
  for (const w of widths) {
    const col = document.createElement("col");
    col.style.width = w;
    colgroup.appendChild(col);
  }
  table.insertBefore(colgroup, table.firstChild);
  return dwrap(rows.length + " steps" + (result ? " · " + result : ""), table);
}

function traceAction(text) {
  let cls = "action";
  if (/^accept\b/.test(text)) cls += " good";
  else if (/^(shift|match)\b/.test(text)) cls += " action-shift";
  else if (/^(reduce|output)\b/.test(text)) cls += " action-reduce";
  return h("code", { class: cls }, text);
}

function renderLRTrace(text) {
  return traceTable(text,
    ["STATE STACK", "SYMBOL STACK", "INPUT", "ACTION"],
    ["State stack", "Symbol stack", "Input", "Action"], 3,
    ["18%", "26%", "24%", "32%"]);
}

function renderLL1Trace(text) {
  return traceTable(text,
    ["STACK", "INPUT", "ACTION"],
    ["Stack (bottom..top)", "Input", "Action"], 2,
    ["38%", "28%", "34%"]);
}

const STRUCTURED = {
  tokens: renderTokens,
  symtab: renderSymtab,
  ll1: renderLL1Trace,
  lr: renderLRTrace,
  ff: renderFirstFollow,
  ll1t: renderLL1Table,
  slrt: renderSLRTable,
};

function showOnly(which) {
  textEl.hidden = which !== "text";
  treeEl.hidden = which !== "tree";
  structuredEl.hidden = which !== "structured";
}

function updateViewbar() {
  for (const button of viewbarEl.querySelectorAll("button")) {
    button.classList.toggle("active", button.getAttribute("data-mode") === viewMode);
  }
}

function showStage(stage, data) {
  lastData = data;
  const renderer = STRUCTURED[stage.id];
  // No Table/Raw toggle when there is nothing to reformat: the tree stages, the
  // free-text stages, or an error run (its raw stdout carries the diagnostics).
  viewbarEl.hidden = !renderer || stage.kind === "tree" || data.exit !== 0;
  updateViewbar();
  if (stage.kind === "tree") { showTree(data); return; }
  // Only reformat clean runs. A non-zero exit means stdout carries diagnostics
  // (the "Errors: N" block) that the columnar parsers would silently drop.
  if (renderer && viewMode === "table" && data.exit === 0) {
    let el = null;
    try { el = renderer(data.stdout || ""); } catch (error) { el = null; }
    if (el && el.childElementCount) {
      structuredEl.replaceChildren(el);
      structuredEl.scrollTop = 0;
      showOnly("structured");
      return;
    }
  }
  showText(data);
}

function unescapeDot(text) {
  let out = "";
  for (let i = 0; i < text.length; i++) {
    if (text[i] === "\\" && i + 1 < text.length) {
      const next = text[++i];
      out += next === "n" ? "\n" : next;
    } else {
      out += text[i];
    }
  }
  return out;
}

function parseDot(dot) {
  const nodes = new Map();
  const children = new Map();
  const hasParent = new Set();
  const nodeRe = /^\s*n(\d+)\s*\[label="((?:[^"\\]|\\.)*)",\s*shape=([A-Za-z0-9]+)\];/;
  const edgeRe = /^\s*n(\d+)\s*->\s*n(\d+)(?:\s*\[label="((?:[^"\\]|\\.)*)"\])?;/;

  for (const line of dot.split("\n")) {
    const nodeMatch = nodeRe.exec(line);
    if (nodeMatch) {
      nodes.set(+nodeMatch[1], {
        id: +nodeMatch[1],
        label: unescapeDot(nodeMatch[2]),
        shape: nodeMatch[3],
      });
      continue;
    }
    const edgeMatch = edgeRe.exec(line);
    if (edgeMatch) {
      const from = +edgeMatch[1];
      const to = +edgeMatch[2];
      const label = edgeMatch[3] ? unescapeDot(edgeMatch[3]) : "";
      if (!children.has(from)) children.set(from, []);
      children.get(from).push({ to, label });
      hasParent.add(to);
    }
  }

  let root = null;
  for (const id of nodes.keys()) {
    if (!hasParent.has(id)) { root = id; break; }
  }
  return { nodes, children, root };
}

const CHAR_W = 7.3, LINE_H = 16, PAD_X = 12, PAD_Y = 9;

function measure(node) {
  const lines = node.label.split("\n");
  const widest = lines.reduce((max, line) => Math.max(max, line.length), 0);
  return { lines, w: widest * CHAR_W + PAD_X * 2, h: lines.length * LINE_H + PAD_Y * 2 };
}

function layout(tree) {
  const sizes = new Map();
  let maxW = 0, maxH = 0;
  for (const [id, node] of tree.nodes) {
    const size = measure(node);
    sizes.set(id, size);
    maxW = Math.max(maxW, size.w);
    maxH = Math.max(maxH, size.h);
  }
  const xStep = maxW + 26;
  const yStep = maxH + 56;

  const place = new Map();
  const order = [];
  let leaf = 0;

  function walk(id, depth) {
    const kids = (tree.children.get(id) || []).map((edge) => edge.to);
    let centerX;
    if (kids.length === 0) {
      centerX = leaf * xStep;
      leaf++;
    } else {
      const childX = kids.map((kid) => walk(kid, depth + 1));
      centerX = (childX[0] + childX[childX.length - 1]) / 2;
    }
    place.set(id, { cx: centerX, cy: depth * yStep });
    order.push(id);
    return centerX;
  }

  walk(tree.root, 0);
  return { place, sizes, order };
}

let treeBounds = null;
let viewport = { scale: 1, tx: 0, ty: 0 };

function applyViewport() {
  const root = document.getElementById("zoomroot");
  if (root) root.setAttribute("transform", `translate(${viewport.tx},${viewport.ty}) scale(${viewport.scale})`);
}

// A 27-node tree fitted edge-to-edge lands near 0.12 scale (~1px text),
// illegible on a projector. On first render we floor the zoom to READABLE_SCALE
// so the root and first levels stay readable; the toolbar "Fit" button (and the
// "0" key) still pass no floor for a true fit-to-window overview.
const READABLE_SCALE = 0.6;

function fitTree(minScale) {
  if (!treeBounds) return;
  const floor = minScale == null ? 0.1 : minScale;
  const rect = svg.getBoundingClientRect();
  const width = treeBounds.maxX - treeBounds.minX || 1;
  const height = treeBounds.maxY - treeBounds.minY || 1;
  const pad = 48;
  const raw = Math.min((rect.width - pad) / width, (rect.height - pad) / height);
  viewport.scale = clamp(raw, floor, 1.4);
  viewport.tx = (rect.width - width * viewport.scale) / 2 - treeBounds.minX * viewport.scale;
  // When the floor forced the scale above the true fit (tree taller than the
  // viewport), anchor to the top so the root stays on screen instead of being
  // centred out of view.
  const clampedUp = viewport.scale > raw + 1e-6;
  viewport.ty = clampedUp
    ? pad / 2 - treeBounds.minY * viewport.scale
    : (rect.height - height * viewport.scale) / 2 - treeBounds.minY * viewport.scale;
  applyViewport();
}

function makeEl(name, attrs) {
  const element = document.createElementNS(SVG_NS, name);
  for (const key in attrs) element.setAttribute(key, attrs[key]);
  return element;
}

function showTree(data) {
  const tree = parseDot(data.stdout || "");
  if (tree.root === null) {
    showText({ stdout: data.stdout, stderr: data.stderr || "No AST produced - nothing to draw." });
    return;
  }
  viewTree();
  renderTree(tree);
}

function renderTree(tree) {
  const { place, sizes, order } = layout(tree);

  let minX = Infinity, minY = Infinity, maxX = -Infinity, maxY = -Infinity;
  for (const id of order) {
    const point = place.get(id), size = sizes.get(id);
    minX = Math.min(minX, point.cx - size.w / 2);
    maxX = Math.max(maxX, point.cx + size.w / 2);
    minY = Math.min(minY, point.cy);
    maxY = Math.max(maxY, point.cy + size.h);
  }
  treeBounds = { minX, minY, maxX, maxY };

  svg.innerHTML = "";
  const root = makeEl("g", { id: "zoomroot" });
  svg.appendChild(root);

  const edgeLayer = makeEl("g", {});
  root.appendChild(edgeLayer);
  for (const [from, edges] of tree.children) {
    const pf = place.get(from), sf = sizes.get(from);
    if (!pf) continue;
    for (const edge of edges) {
      const pc = place.get(edge.to), sc = sizes.get(edge.to);
      if (!pc || !sc) continue;
      const x1 = pf.cx, y1 = pf.cy + sf.h;
      const x2 = pc.cx, y2 = pc.cy;
      const midY = (y1 + y2) / 2;
      const group = makeEl("g", { class: "edge" });
      group.appendChild(makeEl("path", { d: `M ${x1} ${y1} C ${x1} ${midY}, ${x2} ${midY}, ${x2} ${y2}` }));
      if (edge.label) {
        const labelWidth = edge.label.length * 6.2 + 6;
        group.appendChild(makeEl("rect", {
          class: "edge-bg", x: x2 - labelWidth / 2, y: midY - 8,
          width: labelWidth, height: 15, rx: 3,
        }));
        const text = makeEl("text", { x: x2, y: midY + 3.5, "text-anchor": "middle" });
        text.textContent = edge.label;
        group.appendChild(text);
      }
      edgeLayer.appendChild(group);
    }
  }

  for (const id of order) {
    const point = place.get(id), size = sizes.get(id), node = tree.nodes.get(id);
    const klass = node.shape === "box3d" ? "node node-prog"
      : node.shape === "ellipse" ? "node node-expr" : "node node-stmt";
    const group = makeEl("g", { class: klass, transform: `translate(${point.cx - size.w / 2}, ${point.cy})` });
    group.appendChild(makeEl("rect", { width: size.w, height: size.h, rx: 6 }));
    size.lines.forEach((line, index) => {
      const text = makeEl("text", { x: size.w / 2, y: PAD_Y + LINE_H * (index + 0.78), "text-anchor": "middle" });
      text.textContent = line;
      group.appendChild(text);
    });
    root.appendChild(group);
  }

  svg.setAttribute("aria-label",
    "Abstract syntax tree, " + tree.nodes.size + " nodes. Scroll to zoom, drag to pan.");
  fitTree(READABLE_SCALE);
}

function zoomAround(screenX, screenY, factor) {
  const next = clamp(viewport.scale * factor, 0.1, 4);
  const worldX = (screenX - viewport.tx) / viewport.scale;
  const worldY = (screenY - viewport.ty) / viewport.scale;
  viewport.scale = next;
  viewport.tx = screenX - worldX * next;
  viewport.ty = screenY - worldY * next;
  applyViewport();
}

function wireTreeControls() {
  svg.addEventListener("wheel", (event) => {
    event.preventDefault();
    const rect = svg.getBoundingClientRect();
    zoomAround(event.clientX - rect.left, event.clientY - rect.top, event.deltaY < 0 ? 1.12 : 1 / 1.12);
  }, { passive: false });

  let drag = null;
  svg.addEventListener("mousedown", (event) => {
    drag = { x: event.clientX, y: event.clientY, tx: viewport.tx, ty: viewport.ty };
    svg.classList.add("panning");
  });
  window.addEventListener("mousemove", (event) => {
    if (!drag) return;
    viewport.tx = drag.tx + (event.clientX - drag.x);
    viewport.ty = drag.ty + (event.clientY - drag.y);
    applyViewport();
  });
  window.addEventListener("mouseup", () => { drag = null; svg.classList.remove("panning"); });

  document.querySelector(".tree-toolbar").addEventListener("click", (event) => {
    const zoom = event.target.getAttribute("data-zoom");
    if (!zoom) return;
    if (zoom === "fit") { fitTree(); return; }
    const rect = svg.getBoundingClientRect();
    zoomAround(rect.width / 2, rect.height / 2, zoom === "in" ? 1.2 : 1 / 1.2);
  });

  window.addEventListener("resize", () => { if (!treeEl.hidden) fitTree(READABLE_SCALE); });

  svg.addEventListener("keydown", (event) => {
    const rect = svg.getBoundingClientRect();
    if (event.key === "+" || event.key === "=") {
      zoomAround(rect.width / 2, rect.height / 2, 1.2); event.preventDefault();
    } else if (event.key === "-" || event.key === "_") {
      zoomAround(rect.width / 2, rect.height / 2, 1 / 1.2); event.preventDefault();
    } else if (event.key === "0") {
      fitTree(); event.preventDefault();
    }
  });
}

async function loadSampleList() {
  try {
    const response = await fetch("/samples");
    const data = await response.json();
    for (const name of data.samples) {
      const option = document.createElement("option");
      option.value = name;
      option.textContent = name;
      sampleSel.appendChild(option);
    }
    if (data.samples.includes("gcd.pas")) sampleSel.value = "gcd.pas";
    await loadSample(true);
  } catch (error) {
    statusEl.textContent = "could not list samples";
  }
}

async function loadSample(force) {
  const name = sampleSel.value;
  if (!name) return;
  const dirty = codeEl.value.trim() && codeEl.value !== loadedCode;
  if (force !== true && dirty &&
      !window.confirm("Replace the editor with “" + name + "”? Your current edits will be lost.")) {
    return;
  }
  const response = await fetch("/sample?name=" + encodeURIComponent(name));
  const data = await response.json();
  if (data.code != null) { codeEl.value = data.code; loadedCode = data.code; }
}

async function init() {
  buildStageButtons();
  wireTreeControls();
  loadBtn.addEventListener("click", () => loadSample(false));
  viewbarEl.addEventListener("click", (event) => {
    const mode = event.target.getAttribute("data-mode");
    if (!mode || mode === viewMode) return;
    viewMode = mode;
    if (activeStage && lastData) showStage(activeStage, lastData);
  });
  document.addEventListener("keydown", (event) => {
    if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
      event.preventDefault();
      if (activeStage) runStage(activeStage);
    }
  });
  await loadSampleList();
  runStage(STAGES[1]);
}

init();
