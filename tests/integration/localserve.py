#!/usr/bin/env python3
"""Serves a local repository over git's smart HTTP protocol by shelling out to
git-http-backend, so the clone path can be exercised end to end with no network.

The point of doing it with the real backend rather than a hand-rolled responder
is that pkt-line framing, the ref advertisement's flush packets, the
Content-Type of the upload-pack response and the status codes all come from git
itself. A mock would test the mock.

Usage: localserve.py <repo.git> <port>
"""
import os
import subprocess
import sys
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

REPO = sys.argv[1]
PORT = int(sys.argv[2])
# git-core layout on Debian/Ubuntu and Arch; fall back to whatever is on PATH.
CANDIDATES = [
    "/usr/lib/git-core/git-http-backend",
    "/usr/libexec/git-core/git-http-backend",
]


def findBackend():
    for path in CANDIDATES:
        if os.path.isfile(path) and os.access(path, os.X_OK):
            return path
    found = shutil_which()
    if found:
        return found
    sys.exit(
        "FATAL: git-http-backend not found. Install git's http-backend CGI "
        "(Debian/Ubuntu: apt install git, Arch: pacman -S git) or put "
        "git-http-backend on PATH."
    )


def shutil_which():
    from shutil import which
    return which("git-http-backend")


BACKEND = findBackend()


class Handler(BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def log_message(self, *args):
        pass

    def _run_backend(self, body=b""):
        env = dict(os.environ)
        env.update({
            "GIT_PROJECT_ROOT": os.path.dirname(REPO.rstrip("/")),
            "GIT_HTTP_EXPORT_ALL": "1",
            "PATH_INFO": self.path.partition("?")[0],
            "REQUEST_METHOD": self.command,
            "QUERY_STRING": self.path.partition("?")[2],
            "CONTENT_TYPE": self.headers.get("Content-Type", ""),
            "CONTENT_LENGTH": str(len(body)),
            "REMOTE_USER": "tester",
            "REMOTE_ADDR": "127.0.0.1",
            "SERVER_PROTOCOL": "HTTP/1.1",
            "GATEWAY_INTERFACE": "CGI/1.1",
        })
        proc = subprocess.run([BACKEND], input=body, env=env, capture_output=True)
        if proc.returncode != 0:
            self.send_response(500)
            self.end_headers()
            self.wfile.write(proc.stderr)
            return
        # git-http-backend writes CGI headers, then a blank line, then the body
        head, sep, payload = proc.stdout.partition(b"\r\n\r\n")
        if not sep:
            head, sep, payload = proc.stdout.partition(b"\n\n")
        status = 200
        headers = []
        for line in head.replace(b"\r\n", b"\n").split(b"\n"):
            if not line:
                continue
            key, _, value = line.decode().partition(":")
            key = key.strip()
            if key.lower() == "status":
                code, _, reason = value.strip().partition(" ")
                status = int(code)
                continue
            headers.append((key, value.strip()))
        self.send_response(status)
        for key, value in headers:
            self.send_header(key, value)
        # git-http-backend does not always send Content-Length, so close the
        # connection to delimit the body instead of hanging the client
        self.send_header("Connection", "close")
        self.close_connection = True
        self.end_headers()
        self.wfile.write(payload)

    def do_GET(self):
        self._run_backend()

    def do_POST(self):
        length = int(self.headers.get("Content-Length", "0"))
        self._run_backend(self.rfile.read(length))


if __name__ == "__main__":
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
