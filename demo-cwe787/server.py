#!/usr/bin/env python3
"""
Servidor local para la demo de CWE-787 (Out-of-bounds Write).
Sirve la pagina y expone /api/check que ejecuta el binario en C con el input.

IMPORTANTE: el input del usuario se pasa como argv (lista), NUNCA por shell,
asi que aca no hay inyeccion de comandos: lo unico vulnerable es el binario en C,
que es justo lo que queremos demostrar.
"""
import http.server, socketserver, json, subprocess, os, urllib.parse

PORT = 8000
HERE = os.path.dirname(os.path.abspath(__file__))
BINS = {"vuln": os.path.join(HERE, "bin", "vuln"),
        "secure": os.path.join(HERE, "bin", "secure")}

def run_binary(mode, coupon):
    binpath = BINS.get(mode, BINS["vuln"])
    if not os.path.exists(binpath):
        return {"error": f"No existe el binario '{mode}'. Corre ./build.sh primero."}
    try:
        # argv directo -> sin shell -> sin command injection. Timeout por las dudas.
        proc = subprocess.run([binpath, coupon], capture_output=True,
                              text=True, timeout=3)
    except subprocess.TimeoutExpired:
        return {"error": "timeout", "crashed": True}
    out = proc.stdout
    parsed = {"raw": out, "returncode": proc.returncode,
              "crashed": proc.returncode < 0}
    for line in out.splitlines():
        if line.startswith("BYTES:"):
            parsed["bytes"] = line.split(":", 1)[1].split()
        elif line.startswith("DISCOUNT:"):
            parsed["discount"] = int(line.split(":", 1)[1].strip())
        elif line.startswith("COUPON_VALID:"):
            parsed["coupon_valid"] = line.split(":", 1)[1].strip() == "1"
        elif line.startswith("BASE:"):
            parsed["base"] = int(line.split(":", 1)[1].strip())
        elif line.startswith("FINAL:"):
            parsed["final"] = int(line.split(":", 1)[1].strip())
        elif line.startswith("RESULT:"):
            parsed["result"] = line.split(":", 1)[1].strip()
    return parsed

class Handler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *a, **k):
        super().__init__(*a, directory=HERE, **k)
    def log_message(self, *a):  # silencioso
        pass
    def do_POST(self):
        if self.path != "/api/check":
            self.send_error(404); return
        length = int(self.headers.get("Content-Length", 0))
        data = json.loads(self.rfile.read(length) or b"{}")
        result = run_binary(data.get("mode", "vuln"), data.get("coupon", ""))
        body = json.dumps(result).encode()
        self.send_response(200)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

if __name__ == "__main__":
    os.chdir(HERE)
    socketserver.TCPServer.allow_reuse_address = True
    with socketserver.TCPServer(("127.0.0.1", PORT), Handler) as httpd:
        print(f"\n  Demo CWE-787 corriendo en:  http://localhost:{PORT}\n")
        print("  Ctrl+C para detener.\n")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n  Servidor detenido.")
