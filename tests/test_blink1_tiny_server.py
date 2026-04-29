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
import urllib.request
import urllib.error

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
# --- http get helpers ------------------------------------------------------------
#

def http_get(path):
    """Return (status_code, body) from a GET request."""
    url = BASE_URL + path
    try:
        with urllib.request.urlopen(url) as resp:
            return resp.status, resp.read().decode().strip()
    except urllib.error.HTTPError as e:
        return e.code, ""
    except urllib.error.URLError:
        return 1, ""

def http_get_json(path):
    """GET JSON and parse it. Raises on HTTP errors or invalid JSON."""
    code, out = http_get(path)
    if code != 200:
        raise RuntimeError(f"request failed ({code}) for {path}")
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
    code, out = http_get("/blink1/id")
    if code != 200:
        raise AssertionError("Server did not respond to /blink1/id")

@test
def test_blink1_id_json_valid():
    js = http_get_json("/blink1/id")
    assert_json_field(js, ["status"], "blink1 id")
    # only if blink1 is plugged in
    #if "blink1_id" not in js:
    #    raise AssertionError("JSON missing 'blink1_id'")

@test
def test_patterns_contains_red_flash():
    js = http_get_json("/blink1/patterns")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "red flash",
                                        "pattern": "9,#ff0000,0.5,0,#000000,0.5,0"})

@test
def test_patterns_contains_policecar():
    js = http_get_json("/blink1/pattern")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "policecar",
                                        "pattern": "6,#ff0000,0.3,1,#0000ff,0.3,2,#000000,0.1,0,#ff0000,0.3,2,#0000ff,0.3,1,#000000,0.1,0"})

@test
def test_pattern_play():
    js = http_get_json("/blink1/pattern/play?pname=red+flash&count=3")
    assert_json_field(js, ["status"], "blink1 pattern play")
    assert_json_field(js, ["pname"], "red flash")
    assert_json_field(js, ["count"], 3)
    assert_json_field(js, ["pattern"], "9,#ff0000,0.50,0,#000000,0.50,0")

@test
def test_add_pattern():
    js = http_get_json("/blink1/pattern/add?pname=todtest&pattern=3,%23FF00FF,0.5,0,%23000000,0.5,0")
    assert_json_field(js, ["status"], "blink1 pattern add")
    assert_json_field(js, ["pname"], "todtest")
    assert_json_field(js, ["pattern"], "3,#FF00FF,0.5,0,#000000,0.5,0")
    #if js["pname"] != "todtest":
    #    raise AssertionError("Field 'pname' incorrect in JSON")

@test
def test_patterns_contain_test():
    js = http_get_json("/blink1/pattern")
    if "patterns" not in js:
        raise AssertionError("Missing 'patterns' in JSON")
    assert_any_matches(js["patterns"], {"name": "todtest",
                                        "pattern": "3,#FF00FF,0.5,0,#000000,0.5,0"})

@test
def test_patterns_del():
    js = http_get_json("/blink1/pattern/del?pname=todtest")
    assert_json_field(js, ["status"], "blink1 pattern del")
    assert_json_field(js, ["pname"], "todtest")
    # fetch pattern list again and look for deleted entry
    js = http_get_json("/blink1/patterns")
    assert_none_matches(js["patterns"], {"name": "todtest"})

@test
def test_common_response_shape():
    for path in ["/blink1/", "/blink1/red", "/blink1/off"]:
        js = http_get_json(path)
        for key in ("status", "version", "rgb", "millis"):
            if key not in js:
                raise AssertionError(f"Response from {path} missing field '{key}'")

@test
def test_fadeToRGB_rgb_echoed():
    js = http_get_json("/blink1/fadeToRGB?rgb=FF00FF")
    assert_json_field(js, ["rgb"], "#ff00ff")

@test
def test_lastColor_tracks_red():
    http_get_json("/blink1/red")
    js = http_get_json("/blink1/lastColor")
    assert_json_field(js, ["status"], "blink1 lastColor")
    assert_json_field(js, ["lastColor"], "#ff0000")

@test
def test_pattern_play_unknown_pname():
    js = http_get_json("/blink1/pattern/play?pname=doesnotexist")
    status = js.get("status", "")
    if "error" not in status:
        raise AssertionError(f"Expected error in status for unknown pname, got '{status}'")

@test
def test_pattern_add_missing_pattern_arg():
    js = http_get_json("/blink1/pattern/add?pname=onlyname")
    status = js.get("status", "")
    if "error" not in status:
        raise AssertionError(f"Expected error in status when pattern arg missing, got '{status}'")

@test
def test_pattern_del_missing_pname():
    js = http_get_json("/blink1/pattern/del")
    status = js.get("status", "")
    if "error" not in status:
        raise AssertionError(f"Expected error in status when pname arg missing, got '{status}'")

@test
def test_unknown_endpoint_404():
    code, _ = http_get("/blink1/doesnotexist")
    if code != 404:
        raise AssertionError(f"Expected 404, got {code}")

@test
def test_malformed_id_no_crash():
    http_get("/blink1/on?id=,")
    # server should still respond after malformed id
    code, _ = http_get("/blink1/id")
    if code != 200:
        raise AssertionError("Server stopped responding after malformed id= arg")
    
@test
def test_fadeToRGB_rgb_off():
    js = http_get_json("/blink1/off")
    assert_json_field(js, ["rgb"], "#000000")

@test
def test_pattern_play_inline_pattern():
    js = http_get_json("/blink1/pattern/play?pattern=3,%23ff00ff,0.5,0,%23000000,0.5,0")
    assert_json_field(js, ["status"], "blink1 pattern play")
    assert_json_field(js, ["pattern"], "3,#ff00ff,0.50,0,#000000,0.50,0")

@test
def test_patterns_count_stable_after_add_del():
    js = http_get_json("/blink1/patterns")
    count_before = len(js["patterns"])
    http_get_json("/blink1/pattern/add?pname=counttest&pattern=1,%23ff0000,0.5,0")
    http_get_json("/blink1/pattern/del?pname=counttest")
    js = http_get_json("/blink1/patterns")
    count_after = len(js["patterns"])
    if count_before != count_after:
        raise AssertionError(f"Pattern count changed: {count_before} → {count_after}")

@test
def test_lastColor_tracks_off():
    http_get_json("/blink1/off")
    js = http_get_json("/blink1/lastColor")
    assert_json_field(js, ["lastColor"], "#000000")

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
