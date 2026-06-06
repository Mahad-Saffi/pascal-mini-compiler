"""
Mini Pascal Compiler — FastAPI backend
Wraps ./bin/minipascal and returns structured JSON.
Run with:  uvicorn api:app --reload  (from web/ dir)
"""

import os
import re
import subprocess
import tempfile
from pathlib import Path
from typing import Any, Optional

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

app = FastAPI(title="Mini Pascal Compiler API")

app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

SCRIPT_DIR   = Path(__file__).parent
COMPILER_DIR = SCRIPT_DIR.parent            # Final Project/
COMPILER     = str(COMPILER_DIR / "bin" / "minipascal")
TEST_DIR     = COMPILER_DIR / "test"


# ── helpers ─────────────────────────────────────────────────────────────────

def run(args: list[str]) -> tuple[str, str, int]:
    r = subprocess.run(
        [COMPILER] + args,
        capture_output=True,
        text=True,
        cwd=str(COMPILER_DIR),
        timeout=20,
    )
    return r.stdout, r.stderr, r.returncode


def run_with_tmp(source: str, cmd: str) -> tuple[str, str, int]:
    with tempfile.NamedTemporaryFile(mode="w", suffix=".pas", delete=False) as f:
        f.write(source)
        tmp = f.name
    try:
        return run([cmd, tmp])
    finally:
        os.unlink(tmp)


# ── parsers ──────────────────────────────────────────────────────────────────

def parse_tokens(text: str) -> list[dict]:
    tokens: list[dict] = []
    for line in text.splitlines():
        m = re.match(r"^\s+(\d+)\s+(\d+)\s+(\S+)\s+(\S+)\s+(.*?)\s*$", line)
        if m:
            tokens.append({
                "line":     int(m.group(1)),
                "col":      int(m.group(2)),
                "type":     m.group(3),
                "lexeme":   m.group(4),
                "terminal": m.group(5).strip(),
            })
    return tokens


def parse_ast_tree(text: str) -> Optional[dict]:
    rows: list[dict] = []
    for line in text.splitlines():
        stripped = line.lstrip(" ")
        indent   = len(line) - len(stripped)
        stripped = stripped.rstrip()
        if stripped:
            rows.append({"indent": indent, "text": stripped})
    if not rows:
        return None

    ctr = [0]

    def mk(text: str) -> dict:
        ctr[0] += 1
        is_sec   = text.endswith(":")
        clean    = text.rstrip(":")
        parts    = clean.split(" ", 1)
        return {
            "id":        ctr[0],
            "type":      parts[0],
            "label":     parts[1] if len(parts) > 1 else "",
            "isSection": is_sec,
            "children":  [],
        }

    root  = mk(rows[0]["text"])
    stack = [(rows[0]["indent"], root)]

    for row in rows[1:]:
        node = mk(row["text"])
        while len(stack) > 1 and stack[-1][0] >= row["indent"]:
            stack.pop()
        stack[-1][1]["children"].append(node)
        stack.append((row["indent"], node))

    return root


def parse_symtab(text: str) -> list[dict]:
    scopes: list[dict] = []
    cur: Optional[dict] = None

    for line in text.splitlines():
        m = re.match(r"\s*scope\s+(\d+)(.*?):", line)
        if m:
            cur = {"level": int(m.group(1)), "label": m.group(2).strip().strip("()"), "entries": []}
            scopes.append(cur)
            continue
        if cur is None:
            continue
        m = re.match(r"\s+(\w+)\s+\[(\w+)\]\s+(.+?)(?:\s+\(line\s+(\d+)\))?\s*$", line)
        if m:
            cur["entries"].append({
                "name":      m.group(1),
                "kind":      m.group(2),
                "type_info": m.group(3).strip(),
                "line":      int(m.group(4)) if m.group(4) else None,
            })
    return scopes


def parse_diagnostics(text: str) -> list[dict]:
    diags: list[dict] = []
    for line in text.splitlines():
        m = re.match(r"\s+\[(\w+)(?:\s+(\w+))?\]\s+line\s+(\d+),\s+col\s+(\d+):\s+(.+)", line)
        if m:
            diags.append({
                "phase":    m.group(1),
                "severity": m.group(2) or "error",
                "line":     int(m.group(3)),
                "col":      int(m.group(4)),
                "message":  m.group(5).strip(),
            })
    return diags


def parse_error_summary(text: str) -> dict:
    if "No errors." in text:
        return {"errors": 0, "warnings": 0}
    m = re.search(r"Errors:\s+(\d+)", text)
    errors = int(m.group(1)) if m else 0
    m = re.search(r"Warnings:\s+(\d+)", text)
    warnings = int(m.group(1)) if m else 0
    return {"errors": errors, "warnings": warnings}


def parse_ll1_trace(text: str) -> list[dict]:
    # Col offsets: 2 + 46 + 34 + rest  (from predictive.cpp setw calls)
    rows: list[dict] = []
    in_trace = False
    for line in text.splitlines():
        if re.match(r"\s*-{10,}", line):
            in_trace = True
            continue
        if not in_trace or not line.strip():
            continue
        if line.strip().startswith("->"):          # recovery annotation
            if rows:
                rows[-1]["action"] += "  " + line.strip()
            continue
        body = line[2:]                            # strip leading 2 spaces
        if len(body) < 20:
            continue
        stack  = body[:46].strip()
        inp    = body[46:80].strip()
        action = body[80:].strip()
        rows.append({"stack": stack, "input": inp, "action": action})
    return rows


def parse_lr_trace(text: str) -> list[dict]:
    # Col offsets: 2 + 26 + 28 + 26 + rest  (from lr.cpp setw calls)
    rows: list[dict] = []
    in_trace = False
    for line in text.splitlines():
        if re.match(r"\s*-{10,}", line):
            in_trace = True
            continue
        if not in_trace or not line.strip():
            continue
        if line.strip().startswith("->"):
            if rows:
                rows[-1]["action"] += "  " + line.strip()
            continue
        body = line[2:]
        if len(body) < 10:
            continue
        states  = body[:26].strip()
        symbols = body[26:54].strip()
        inp     = body[54:80].strip()
        action  = body[80:].strip()
        rows.append({"states": states, "symbols": symbols, "input": inp, "action": action})
    return rows


def parse_agreement(text: str) -> dict:
    m = re.search(
        r"^\s+(accept|reject)\s+(accept|reject)\s+(accept|reject)\s+(.+)$",
        text, re.MULTILINE,
    )
    if m:
        return {
            "rdp":     m.group(1) == "accept",
            "ll1":     m.group(2) == "accept",
            "lr":      m.group(3) == "accept",
            "verdict": m.group(4).strip(),
            "agree":   "agree" in m.group(4).lower(),
        }
    return {"rdp": False, "ll1": False, "lr": False, "verdict": "unknown", "agree": False}


def parse_first_follow(text: str) -> list[dict]:
    result: list[dict] = []
    cur: Optional[dict] = None
    for line in text.splitlines():
        m = re.match(r"^(<[^>]+>)\s+FIRST\s*=\s*\{([^}]*)\}", line)
        if m:
            cur = {
                "nonterminal": m.group(1),
                "first":  [x.strip() for x in m.group(2).split(",") if x.strip()],
                "follow": [],
            }
            result.append(cur)
            continue
        if cur:
            m = re.match(r"^\s+FOLLOW\s*=\s*\{([^}]*)\}", line)
            if m:
                cur["follow"] = [x.strip() for x in m.group(1).split(",") if x.strip()]
    return result


def extract_ast_text(rdp_out: str) -> str:
    lines, in_ast, started = [], False, False
    for line in rdp_out.splitlines():
        if "Abstract syntax tree:" in line:
            in_ast = True
            continue
        if not in_ast:
            continue
        # Skip leading blank lines before content starts
        if not started and not line.strip():
            continue
        # Stop at trailing blank line or summary line
        if started and (not line.strip() or re.match(r"^(No errors|Errors|Warnings)", line.strip())):
            break
        started = True
        lines.append(line)
    return "\n".join(lines)


# ── endpoints ────────────────────────────────────────────────────────────────

class CompileRequest(BaseModel):
    source: str


@app.post("/api/compile")
async def compile_source(req: CompileRequest) -> dict[str, Any]:
    src = req.source

    with tempfile.NamedTemporaryFile(mode="w", suffix=".pas", delete=False) as f:
        f.write(src)
        tmp = f.name

    try:
        lex_out,    _, _  = run(["lex",    tmp])
        rdp_out,    _, _  = run(["rdp",    tmp])
        ll1_out,    _, _  = run(["ll1",    tmp])
        lr_out,     _, _  = run(["lr",     tmp])
        sym_out,    _, _  = run(["symtab", tmp])
        sem_out, sem_err, _ = run(["semant", tmp])
        all_out,    _, _  = run(["all",    tmp])

        ast_text = extract_ast_text(rdp_out)
        sem_full = sem_out + sem_err

        return {
            "success":       True,
            "tokens":        parse_tokens(lex_out),
            "ast_text":      ast_text,
            "ast_tree":      parse_ast_tree(ast_text),
            "ll1_trace":     parse_ll1_trace(ll1_out),
            "ll1_raw":       ll1_out,
            "lr_trace":      parse_lr_trace(lr_out),
            "lr_raw":        lr_out,
            "symtab":        parse_symtab(sym_out),
            "diagnostics":   parse_diagnostics(sem_full),
            "error_summary": parse_error_summary(sem_full),
            "agreement":     parse_agreement(all_out),
            "rdp_raw":       rdp_out,
        }

    except subprocess.TimeoutExpired:
        return {"success": False, "error": "Compiler timed out"}
    except Exception as e:
        return {"success": False, "error": str(e)}
    finally:
        os.unlink(tmp)


@app.get("/api/grammar")
async def get_grammar() -> dict[str, Any]:
    try:
        g_out,  _, _ = run(["grammar"])
        ff_out, _, _ = run(["first-follow"])
        l1_out, _, _ = run(["ll1-table"])
        sl_out, _, _ = run(["slr-table"])
        return {
            "grammar_raw":       g_out,
            "first_follow":      parse_first_follow(ff_out),
            "first_follow_raw":  ff_out,
            "ll1_table_raw":     l1_out,
            "slr_table_raw":     sl_out,
        }
    except Exception as e:
        return {"error": str(e)}


@app.get("/api/samples")
async def get_samples() -> dict[str, Any]:
    samples = []
    for f in sorted(TEST_DIR.glob("*.pas")):
        samples.append({"name": f.name, "content": f.read_text()})
    return {"samples": samples}


# Serve compiled frontend in production
_dist = SCRIPT_DIR / "frontend" / "dist"
if _dist.exists():
    app.mount("/assets", StaticFiles(directory=str(_dist / "assets")), name="assets")

    @app.get("/{full_path:path}")
    async def spa(_: str):
        return FileResponse(str(_dist / "index.html"))
