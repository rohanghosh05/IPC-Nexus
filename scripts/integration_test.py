#!/usr/bin/env python3
"""Automated localhost integration check for TCP broadcast and binary upload."""
import hashlib, os, socket, subprocess, tempfile, time
PORT = 55173
server = subprocess.Popen(["./build/tcp_server", str(PORT)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
clients = []
def connect():
    s = socket.create_connection(("127.0.0.1", PORT), timeout=3)
    s.settimeout(3)
    f = s.makefile("rb")
    welcome = f.readline().decode().strip()
    if not welcome.startswith("INFO Connected"):
        raise RuntimeError(f"Unexpected welcome: {welcome}")
    clients.append((s, f))
    return s, f
try:
    for _ in range(30):
        try:
            a, fa = connect(); b, fb = connect(); c, fc = connect(); break
        except OSError:
            time.sleep(.1)
    else:
        raise RuntimeError("Server did not start")
    a.sendall(b"CHAT integration-test-message\n")
    assert fb.readline().decode().strip() == "integration-test-message"
    assert fc.readline().decode().strip() == "integration-test-message"
    payload = bytes(range(256)) * 128
    a.sendall(f"FILE {len(payload)} integration_test.bin\n".encode())
    assert fa.readline().decode().strip() == "READY"
    a.sendall(payload)
    ack = fa.readline().decode().strip()
    assert ack.startswith("OK File saved as integration_test.bin"), ack
    with open("uploads/integration_test.bin", "rb") as f:
        saved = f.read()
    assert hashlib.sha256(payload).digest() == hashlib.sha256(saved).digest()
    print("PASS: three-client TCP broadcast")
    print("PASS: binary file upload and SHA-256 verification")
finally:
    for s, _ in clients:
        try: s.close()
        except OSError: pass
    server.terminate()
    try: server.wait(timeout=3)
    except subprocess.TimeoutExpired: server.kill()
    try: os.remove("uploads/integration_test.bin")
    except FileNotFoundError: pass
