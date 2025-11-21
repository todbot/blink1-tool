
## blink1-tiny-server

A simple HTTP to blink(1) gateway

- Uses blink1-lib to talk to blink(1)
- Uses [Mongoose](https://github.com/cesanta/mongoose), included

Supports most of the URIs / endpoints of the Blink1Control API server,
as described in [app-url-api-examples.md](https://github.com/todbot/blink1/blob/main/docs/app-url-api-examples.md) and [app-url-api.md](https://github.com/todbot/blink1/blob/main/docs/app-url-api.md),
but implemented as a tiny portable C webserver.

To build:
```
cd blink1-tool
make blink1-tiny-server
```

Use:
```
./blink1-tiny-server -p 8934
```
By default the HTML API examples are built-in and served. To try them out,
open a browser to: http://localhost:8934/

To disable the HTML examples, use:
```
./blink1-tiny-server -p 8000 -no-html
```

Usage:
```
% ./blink1-tiny-server --help
Usage: 
  blink1-tiny-server [options]
where [options] can be:
  --port port, -p port           port to listen on (default 8934)
  --host host, -H host           host to listen on ('127.0.0.1' or '0.0.0.0')
  --no-html                      do not serve static HTML help
  --logging                      log accesses to stdout
  --version                      version of this program
  --help, -h                     this help page

Supported URIs:
  /blink1/ -- Simple status page
  /blink1/id -- Get blink1 serial number, list all found blink1 serials
  /blink1/on -- Turn blink(1) full bright white
  /blink1/off -- Turn blink(1) dark
  /blink1/red -- Turn blink(1) solid red
  /blink1/green -- Turn blink(1) solid green
  /blink1/blue -- Turn blink(1) solid blue
  /blink1/cyan -- Turn blink(1) solid cyan
  /blink1/yellow -- Turn blink(1) solid yellow
  /blink1/magenta -- Turn blink(1) solid magenta
  /blink1/fadeToRGB -- Turn blink(1) specified RGB color by 'rgb' arg
  /blink1/lastColor -- Return the last rgb color sent to blink(1)
  /blink1/blink -- Blink the blink(1) a specified 'rgb' color
  /blink1/patterns -- List available patterns
  /blink1/pattern/play -- Play color pattern specified by 'pname' or 'pattern' arg
  /blink1/pattern/stop -- Stop color pattern playing
  /blink1/random -- turn the blink(1) a random color
  /blink1/servertickle/on -- Enable servertickle, uses 'millis' or 'time' arg
  /blink1/servertickle/off -- Disable servertickle

Supported query arguments: (not all urls support all args)
  'rgb'    -- hex RGB color code. e.g. 'rgb=FF9900' or 'rgb=%23FF9900
  'time'   -- time in seconds. e.g. 'time=0.5' 
  'bright' -- brightness, 1-255, 0=full e.g. half-bright 'bright=127'
  'ledn'   -- which LED to set. 0=all/1=top/2=bot, e.g. 'ledn=0'
  'millis' -- milliseconds to fade, or blink, e.g. 'millis=500'
  'count'  -- number of times to blink, for /blink1/blink, e.g. 'count=3'
  'pattern'-- color pattern string (e.g. '3,00ffff,0.2,0,000000,0.2,0')
  'pname'  -- color pattern name from pattern list (e.g. 'red flash') 

Examples: 
  /blink1/blue?bright=127 -- set blink1 blue, at half-intensity 
  /blink1/fadeToRGB?rgb=FF00FF&millis=500 -- fade to purple over 500ms
  /blink1/pattern/play?pattern=3,00ffff,0.2,0,000000,0.2,0 -- blink cyan 3 times
  /blink1/servertickle?on=1&millis=5000 -- turn servertickle on with 5 sec timer  
 
```
