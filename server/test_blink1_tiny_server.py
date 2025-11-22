#!/usr/bin/env python3
#
# tests for blink1-tiny-server (also somewhat useful for blink1-lib)
# 21 Nov 2025 - @todbot / Tod Kurt
#

import subprocess
import time
import json
import sys
import signal

SERVER_CMD = ["./blink1-tiny-server", "--port", "8000", "--quiet"]
BASE_URL   = "http://localhost:8000"

#
# --- Minimal test harness ----------------------------------------------------
#

tests = []

def test(func):
    """Decorator to register tests."""
    tests.append(func)
    return func

def run_test(name, func):
    print(f"--- {name} ---")
    try:
        func()
        print("PASS\n")
    except Exception as e:
        print("FAIL:", e, "\n")
        raise

#
# --- curl helpers ------------------------------------------------------------
#

def curl_get(path):
    """Return (exitcode, stdout) from curl GET."""
    url = BASE_URL + path
    p = subprocess.run( ["curl", "-s", "-f", url],
                        stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE,
                        text=True)
    return p.returncode, p.stdout.strip()

def curl_json(path):
    """GET JSON and parse it. Raises on curl errors or invalid JSON."""
    code, out = curl_get(path)
    if code != 0:
        raise RuntimeError(f"curl failed ({code}) for {path}")
    try:
        return json.loads(out)
    except json.JSONDecodeError as e:
        raise RuntimeError(f"Invalid JSON from {path}: {e}\nOutput was:\n{out}")

#
# --- Helpers for JSON tests --------------------------------------------------
#

def assert_json_field(data, path, expected):
    """
    Example:
        assert_json_field(json_data, ["patterns", 0, "name"], "red flash")
    """
    cur = data
    for p in path:
        cur = cur[p]
    if cur != expected:
        raise AssertionError(f"JSON field {path} was '{cur}', expected '{expected}'")


def assert_any_matches(items, expected_pairs):
    """
    Pass if ANY dict in `items` has ALL key/value pairs in `expected_pairs`.
    """
    for item in items:
        if all(item.get(k) == v for k, v in expected_pairs.items()):
            return  # success
    raise AssertionError(f"No entry matched expected key/value pairs: {expected_pairs}")


def assert_none_matches(items, forbidden_pairs):
    """
    Pass if NO dict in `items` has ALL key/value pairs in `forbidden_pairs`.
    """
    for item in items:
        if all(item.get(k) == v for k, v in forbidden_pairs.items()):
            raise AssertionError(f"Found an entry that should NOT exist: {forbidden_pairs}")


#
# --- Test definitions --------------------------------------------------------
#

@test
def test_server_alive():
    code, out = curl_get("/blink1/id")
    if code != 0:
        raise AssertionError("Server did not respond to /blink1/id")

@test
def test_blink1_id_json_valid():
    js = curl_json("/blink1/id")
    assert_json_field(js, ["status"], "blink1 id")
    # only if blink1 is plugged in
    #if "blink1_id" not in js:
    #    raise AssertionError("JSON missing 'blink1_id'")

@test
def test_patterns_contains_red_flash():
    js = curl_json("/blink1/patterns")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "red flash",
                                        "pattern": "9,#ff0000,0.5,0,#000000,0.5,0"})

@test
def test_patterns_contains_policecar():
    js = curl_json("/blink1/pattern")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "policecar",
                                        "pattern": "6,#ff0000,0.3,1,#0000ff,0.3,2,#000000,0.1,0,#ff0000,0.3,2,#0000ff,0.3,1,#000000,0.1,0"})

@test
def test_pattern_play():
    js = curl_json("/blink1/pattern/play?pname=red+flash&count=3")
    assert_json_field(js, ["status"], "blink1 pattern play")
    assert_json_field(js, ["pname"], "red flash")
    assert_json_field(js, ["count"], 3)
    assert_json_field(js, ["pattern"], "9,#ff0000,0.50,,#000000,0.50,0")  # note 2 sig figs on time

@test
def test_add_pattern():
    js = curl_json("/blink1/pattern/add?pname=todtest&pattern=3,%23FF00FF,0.5,0,%23000000,0.5,0")
    assert_json_field(js, ["status"], "blink1 pattern add")
    assert_json_field(js, ["pname"], "todtest")
    assert_json_field(js, ["pattern"], "3,#FF00FF,0.5,0,#000000,0.5,0")
    #if js["pname"] != "todtest":
    #    raise AssertionError("Field 'pname' incorrect in JSON")

@test
def test_patterns_contain_test():
    js = curl_json("/blink1/pattern")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "todtest",
                                        "pattern": "3,#FF00FF,0.5,0,#000000,0.5,0"})

@test
def test_patterns_del():
    js = curl_json("/blink1/pattern/del?pname=todtest")
    assert_json_field(js, ["status"], "blink1 pattern del")
    assert_json_field(js, ["pname"], "todtest")
    # fetch pattern list again and look for deleted entry
    js = curl_json("/blink1/patterns")
    assert_none_matches(js["patterns"], {"name": "todtest"})
    

#
# --- Main runner -------------------------------------------------------------
#

def main():
    # Start server
    print("Starting server:", " ".join(SERVER_CMD))
    server = subprocess.Popen(SERVER_CMD)

    # give it time to start
    time.sleep(0.5)

    # run tests
    failed = False
    for t in tests:
        try:
            run_test(t.__name__, t)
        except Exception:
            failed = True

    # shut down server
    server.send_signal(signal.SIGINT)
    try:
        server.wait(timeout=2)
    except subprocess.TimeoutExpired:
        server.kill()

    if failed:
        print("Some tests FAILED.")
        sys.exit(1)
    else:
        print("All tests PASSED.")
        sys.exit(0)

if __name__ == "__main__":
    main()
