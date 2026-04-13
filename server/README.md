
# blink1-tiny-server

A simple HTTP to blink(1) gateway

- Uses blink1-lib to talk to blink(1)
- Works on Linux, RaspberryPi OS, MacOS, Windows
- Uses [Mongoose](https://github.com/cesanta/mongoose) embedded HTTP server
- No external dependencies beyond blink1-lib

Supports most of the URIs / endpoints of the Blink1Control JSON REST API server,
as described in [app-url-api-examples.md](https://github.com/todbot/blink1/blob/main/docs/app-url-api-examples.md) and [app-url-api.md](https://github.com/todbot/blink1/blob/main/docs/app-url-api.md),
implemented as a tiny portable C webserver.

The JSON API can be used in webpages hosted on the same server.
Several HTML examples showing this functionality are embedded in `blink1-tiny-server`.

<!-- screenshot of the HTML help page goes here -->


## Use

To start the server:

```
./blink1-tiny-server
```

By default it listens on port 8934 and serves the built-in HTML API examples.
Open a browser to http://localhost:8934/ to try them out.

To listen on all interfaces (e.g. for access from other machines on the network):

```
./blink1-tiny-server --host 0.0.0.0
```

To disable the built-in HTML examples:

```
./blink1-tiny-server --no-html
```

To load color patterns from a JSON file (saved automatically on exit):

```
./blink1-tiny-server --patternsjson server/patterns-example.json
```

On Linux, `blink1-tiny-server` can run in a Docker container. For details,
see [Docker and blink(1)](https://github.com/todbot/blink1-tool/#docker-and-blink1) in
the main README.


## curl examples

```bash
# set solid colors
curl "http://localhost:8934/blink1/red"
curl "http://localhost:8934/blink1/off"

# fade to a color over 500ms
curl "http://localhost:8934/blink1/fadeToRGB?rgb=FF00FF&millis=500"

# blink cyan 3 times
curl "http://localhost:8934/blink1/blink?rgb=00FFFF&count=3"

# play a named pattern
curl "http://localhost:8934/blink1/pattern/play?pname=red+flash"

# play a pattern inline
curl "http://localhost:8934/blink1/pattern/play?pattern=3,00ffff,0.2,0,000000,0.2,0"

# add a pattern to the in-memory list
curl "http://localhost:8934/blink1/pattern/add?pname=mypattern&pattern=3,ff0000,0.5,0,000000,0.5,0"

# list all patterns
curl "http://localhost:8934/blink1/patterns"

# stop any playing pattern
curl "http://localhost:8934/blink1/pattern/stop"
```


## API endpoints

| Endpoint | Description |
|---|---|
| `/blink1/` | Status page, returns current color |
| `/blink1/id` | List connected blink(1) serial numbers |
| `/blink1/on` | Full bright white |
| `/blink1/off` | Turn off (black) |
| `/blink1/red` | Solid red |
| `/blink1/green` | Solid green |
| `/blink1/blue` | Solid blue |
| `/blink1/cyan` | Solid cyan |
| `/blink1/yellow` | Solid yellow |
| `/blink1/magenta` | Solid magenta |
| `/blink1/fadeToRGB` | Fade to color specified by `rgb` arg |
| `/blink1/lastColor` | Return last color sent to blink(1) |
| `/blink1/blink` | Blink a color, uses `rgb`, `count`, `millis` args |
| `/blink1/random` | Set a random color |
| `/blink1/patterns` | List all available named patterns |
| `/blink1/pattern/play` | Play a pattern by `pname` or inline `pattern` arg |
| `/blink1/pattern/stop` | Stop any playing pattern |
| `/blink1/pattern/add` | Add a named pattern to the in-memory list |
| `/blink1/pattern/del` | Delete a named pattern from the in-memory list |
| `/blink1/pattern/dump` | Return full pattern list as a JSON object |
| `/blink1/blinkserver` | Like `/blink1/blink` but blocking on the server side |
| `/blink1/servertickle/on` | Enable servertickle watchdog, uses `millis` arg |
| `/blink1/servertickle/off` | Disable servertickle |

### Query arguments

Not all endpoints support all arguments.

| Argument | Description |
|---|---|
| `rgb` | Hex RGB color, e.g. `rgb=FF9900` or `rgb=%23FF9900` |
| `time` | Fade/blink time in seconds, e.g. `time=0.5` |
| `millis` | Fade/blink time in milliseconds, e.g. `millis=500` |
| `bright` | Brightness 1–255, 0=full, e.g. `bright=127` for half |
| `ledn` | Which LED: 0=all, 1=top, 2=bottom, e.g. `ledn=1` |
| `count` | Number of times to blink/repeat, e.g. `count=3` |
| `id` | Which blink(1) to address (index or serial number) |
| `pattern` | Inline color pattern string (see below) |
| `pname` | Named pattern from the pattern list |


## Color patterns

A pattern string has the format:

```
repeats,#rrggbb,time,ledn,#rrggbb,time,ledn,...
```

Where:
- `repeats` — number of times to loop (0 = loop forever)
- `#rrggbb` — hex color
- `time` — fade time in seconds for that step
- `ledn` — which LED (0=both, 1=top, 2=bottom)

Example — blink cyan 3 times:
```
3,#00ffff,0.2,0,#000000,0.2,0
```


## Color pattern JSON file

Named patterns can be loaded from a JSON file with `--patternsjson`.
The file is a simple object mapping pattern names to pattern strings:

```json
{
    "red flash": "9,#ff0000,0.5,0,#000000,0.5,0",
    "green flash": "9,#00ff00,0.5,0,#000000,0.5,0",
    "policecar": "6,#ff0000,0.3,1,#0000ff,0.3,2,#000000,0.1,0,#ff0000,0.3,2,#0000ff,0.3,1,#000000,0.1,0"
}
```

See [`patterns-example.json`](patterns-example.json) for a full example.
The file is automatically saved on server exit, so patterns added via
`/blink1/pattern/add` are persisted across restarts.


## API differences from Blink1Control2

- **No WebSocket support** — Blink1Control2 supports a WebSocket API; blink1-tiny-server is HTTP only
- **Pattern playback is in blink(1) hardware** — patterns are written to the blink(1)'s internal RAM buffer and played there, rather than being software-driven by the server. This means playback continues even if the server is stopped, but the pattern length is limited to the blink(1)'s buffer size (16 lines)
- **`/blink1/blinkserver` blocks** — unlike Blink1Control2 which plays asynchronously, this endpoint sleeps on the server for the duration of the blink sequence
- **Pattern persistence via file** — use `--patternsjson` to persist patterns; there is no `/blink1/pattern/save` endpoint


## Usage

```
% ./blink1-tiny-server --help
Usage:
  blink1-tiny-server [options]
where [options] can be:
  --port port, -p port          port to listen on (default 8934)
  --host host, -H host          host to listen on ('127.0.0.1' or '0.0.0.0')
  --no-html                     do not serve static HTML help
  --logging, -l                 log accesses to stdout
  --patternsjson <fn>, -j <fn>  filepath to JSON color pattern list
  --quiet, -q                   quiet non-logging messages (useful with --logging)
  --version                     version of this program
  --help, -h                    this help page
```


## Building

The Mongoose HTTP server library is included, so `blink1-tiny-server` has the
same dependencies as `blink1-tool`. To build:

```
cd blink1-tool
make blink1-tiny-server
```
