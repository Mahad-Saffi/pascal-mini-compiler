#!/usr/bin/env python3
"""Local presentation UI for the Mini Pascal compiler.

Serves a single-page browser front end and shells out to bin/minipascal for
every stage. Standard library only; binds to localhost. Start with:

    python3 tools/web/server.py            # http://127.0.0.1:8000
    python3 tools/web/server.py --port 9000
"""

import argparse
import http.server
import json
import os
import socketserver
import subprocess
import tempfile
from urllib.parse import urlparse, parse_qs

WEB_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_DIR = os.path.dirname(os.path.dirname(WEB_DIR))
BINARY = os.path.join(PROJECT_DIR, "bin", "minipascal")
SAMPLES_DIR = os.path.join(PROJECT_DIR, "test")

FILE_COMMANDS = {"lex", "rdp", "dot", "ll1", "lr", "symtab", "semant", "all"}
GRAMMAR_COMMANDS = {"grammar", "first-follow", "ll1-table", "slr-table"}
ALLOWED_COMMANDS = FILE_COMMANDS | GRAMMAR_COMMANDS

STATIC_FILES = {
    "/": ("index.html", "text/html; charset=utf-8"),
    "/index.html": ("index.html", "text/html; charset=utf-8"),
    "/style.css": ("style.css", "text/css; charset=utf-8"),
    "/app.js": ("app.js", "application/javascript; charset=utf-8"),
}


def list_samples():
    if not os.path.isdir(SAMPLES_DIR):
        return []
    return sorted(name for name in os.listdir(SAMPLES_DIR) if name.endswith(".pas"))


def read_sample(name):
    if name not in list_samples():
        return None
    with open(os.path.join(SAMPLES_DIR, name), encoding="utf-8", errors="replace") as handle:
        return handle.read()


def run_compiler(command, code):
    if command not in ALLOWED_COMMANDS:
        return {"exit": -1, "stdout": "", "stderr": "unknown command: " + str(command)}
    if not os.path.exists(BINARY):
        return {"exit": -1, "stdout": "", "stderr": "binary not found - run 'make' first"}

    arguments = [BINARY, command]
    source_path = None
    try:
        if command in FILE_COMMANDS:
            handle = tempfile.NamedTemporaryFile("w", suffix=".pas", delete=False)
            handle.write(code)
            handle.close()
            source_path = handle.name
            arguments.append(source_path)
        completed = subprocess.run(
            arguments, cwd=PROJECT_DIR, capture_output=True, text=True, timeout=15
        )
        return {"exit": completed.returncode, "stdout": completed.stdout, "stderr": completed.stderr}
    except subprocess.TimeoutExpired:
        return {"exit": -1, "stdout": "", "stderr": "compiler timed out"}
    finally:
        if source_path and os.path.exists(source_path):
            os.remove(source_path)


class Handler(http.server.BaseHTTPRequestHandler):
    def log_message(self, *args):
        pass

    def reply(self, body, content_type, status=200):
        if isinstance(body, str):
            body = body.encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def reply_json(self, payload, status=200):
        self.reply(json.dumps(payload), "application/json; charset=utf-8", status)

    def serve_static(self, route):
        filename, content_type = STATIC_FILES[route]
        path = os.path.join(WEB_DIR, filename)
        if not os.path.exists(path):
            self.reply("not found", "text/plain; charset=utf-8", 404)
            return
        with open(path, "rb") as handle:
            self.reply(handle.read(), content_type)

    def do_GET(self):
        parsed = urlparse(self.path)
        route = parsed.path

        if route in STATIC_FILES:
            self.serve_static(route)
        elif route == "/samples":
            self.reply_json({"samples": list_samples()})
        elif route == "/sample":
            name = os.path.basename(parse_qs(parsed.query).get("name", [""])[0])
            code = read_sample(name)
            if code is None:
                self.reply_json({"error": "unknown sample"}, 404)
            else:
                self.reply_json({"name": name, "code": code})
        else:
            self.reply("not found", "text/plain; charset=utf-8", 404)

    def do_POST(self):
        if urlparse(self.path).path != "/run":
            self.reply_json({"error": "not found"}, 404)
            return
        length = int(self.headers.get("Content-Length", "0"))
        raw = self.rfile.read(length) if length else b""
        try:
            request = json.loads(raw.decode("utf-8"))
        except (ValueError, UnicodeDecodeError):
            self.reply_json({"error": "invalid request"}, 400)
            return
        self.reply_json(run_compiler(request.get("cmd", ""), request.get("code", "")))


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = True


def main():
    parser = argparse.ArgumentParser(description="Mini Pascal compiler web UI")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    options = parser.parse_args()

    if not os.path.exists(BINARY):
        print("warning: " + BINARY + " not found - run 'make' first")

    server = Server((options.host, options.port), Handler)
    url = "http://" + options.host + ":" + str(options.port)
    print("Mini Pascal web UI -> " + url + "   (Ctrl-C to stop)")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nstopped")


if __name__ == "__main__":
    main()
