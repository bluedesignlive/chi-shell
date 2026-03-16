#!/usr/bin/env python3
"""Quick diagnostic: what IPC methods does your Wayfire expose?"""
import os, sys, socket, struct, json

sock_path = os.environ.get("WAYFIRE_SOCKET", "")
if not sock_path:
    print("ERROR: WAYFIRE_SOCKET not set"); sys.exit(1)

s = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
s.connect(sock_path)

def call(method, data=None):
    msg = {"method": method}
    if data: msg["data"] = data
    raw = json.dumps(msg).encode()
    s.sendall(struct.pack("<I", len(raw)) + raw)
    hdr = s.recv(4)
    if len(hdr) < 4: return {}
    size = struct.unpack("<I", hdr)[0]
    buf = b""
    while len(buf) < size:
        buf += s.recv(size - len(buf))
    return json.loads(buf)

# List all methods
resp = call("list-methods")
if "error" in resp:
    print("Your Wayfire doesn't support list-methods (too old?).")
    print("Response:", resp)
    # Try methods directly
    for m in ["scale/toggle", "expo/toggle", "wm-actions/toggle_showdesktop",
              "window-rules/list-views"]:
        r = call(m)
        status = "✓ OK" if "error" not in r else f"✗ {r.get('error','?')}"
        print(f"  {m}: {status}")
else:
    methods = resp.get("methods", resp.get("result", []))
    if isinstance(methods, list):
        print(f"Found {len(methods)} methods:")
        for m in sorted(methods):
            print(f"  {m}")
    else:
        print("Response:", json.dumps(resp, indent=2))

s.close()
