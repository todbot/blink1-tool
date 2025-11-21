/*
 *
 * blink1-tiny-server -- a small cross-platform REST/JSON server for
 *                       controlling a blink(1) device
 *
 * 2012-2022, Tod Kurt, http://todbot.com/blog/ , http://thingm.com/
 *
 * Example supported URLs:
 *
 *  localhost:8934/blink1/on
 *  localhost:8934/blink1/off
 *  localhost:8934/blink1/red
 *  localhost:8934/blink1/green
 *  localhost:8934/blink1/blue
 *  localhost:8934/blink1/blink?rgb=%23ff0ff&time=1.0&count=3
 *  localhost:8934/blink1/fadeToRGB?rgb=%23ff00ff&time=1.0
 *
 *
 */

#include <getopt.h>    // for getopt_long_only()
#include <sys/time.h>
#include <signal.h>

#include "mongoose.h"  // HTTP server
#include "parson.h"    // JSON build and parse

#include "blink1-lib.h"
#include "blink1-lib-patterns.h"

// normally this is obtained from git tags and filled out by the Makefile
#ifndef BLINK1_VERSION
#define BLINK1_VERSION "v0.0"
#endif

const char* blink1_server_name = "blink1-tiny-server";
const char* blink1_server_version = BLINK1_VERSION;

static bool show_html = true;
static bool enable_logging = false;

static char http_listen_host[120] = "localhost";  // or 0.0.0.0 for any
static int http_listen_port = 8934;               // was 8000
static char http_listen_url[100];             // will be "http://localhost:8934"

typedef struct cache_info_ {
    blink1_device* dev;  // device, if opened, NULL otherwise
    int64_t atime;  // time last used
} cache_info;

static int64_t idle_atime = 1000 /* milliseconds */;
static cache_info cache_infos[cache_max];

static rgb_t last_rgb = {0,0,0};

typedef struct _url_info {
    char url[100];
    char desc[100];
} url_info;

static const url_info supported_urls[]
= {
    {"/blink1/",              "Simple status page"},
    {"/blink1/id",            "Get blink1 serial number, list all found blink1 serials"},
    {"/blink1/on",            "Turn blink(1) full bright white"},
    {"/blink1/off",           "Turn blink(1) dark"},
    {"/blink1/red",           "Turn blink(1) solid red"},
    {"/blink1/green",         "Turn blink(1) solid green"},
    {"/blink1/blue",          "Turn blink(1) solid blue"},
    {"/blink1/cyan",          "Turn blink(1) solid cyan"},
    {"/blink1/yellow",        "Turn blink(1) solid yellow"},
    {"/blink1/magenta",       "Turn blink(1) solid magenta"},
    {"/blink1/fadeToRGB",     "Turn blink(1) specified RGB color by 'rgb' arg"},
    {"/blink1/lastColor",     "Return the last rgb color sent to blink(1)"},
    {"/blink1/blink",         "Blink the blink(1) a specified 'rgb' color"},
    {"/blink1/patterns",      "List available patterns"},
    {"/blink1/pattern/play",  "Play color pattern specified by 'pname' or 'pattern' arg"},
    {"/blink1/pattern/stop",  "Stop color pattern playing"},
    {"/blink1/random",        "turn the blink(1) a random color"},
    {"/blink1/servertickle/on","Enable servertickle, uses 'millis' or 'time' arg"},
    {"/blink1/servertickle/off","Disable servertickle"}
};

void usage()
{
    fprintf(stderr,
"Usage: \n"
"  %s [options]\n"
"where [options] can be:\n"
"  --port port, -p port           port to listen on (default %d)\n"
"  --host host, -H host           host to listen on ('127.0.0.1' or '0.0.0.0')\n"
"  --no-html                      do not serve static HTML help\n"
"  --logging                      log accesses to stdout\n"
"  --version                      version of this program\n"
"  --help, -h                     this help page\n"
"\n",
        blink1_server_name, http_listen_port);

    fprintf(stderr,
"Supported URIs:\n");
    for( int i=0; i< sizeof(supported_urls)/sizeof(url_info); i++ ) {
        fprintf(stderr,"  %s -- %s\n", supported_urls[i].url, supported_urls[i].desc);
    }
    fprintf(stderr,"\n");
    fprintf(stderr,
"Supported query arguments: (not all urls support all args)\n"
"  'rgb'    -- hex RGB color code. e.g. 'rgb=FF9900' or 'rgb=%%23FF9900\n"
"  'time'   -- time in seconds. e.g. 'time=0.5' \n"
"  'bright' -- brightness, 1-255, 0=full e.g. half-bright 'bright=127'\n"
"  'ledn'   -- which LED to set. 0=all/1=top/2=bot, e.g. 'ledn=0'\n"
"  'millis' -- milliseconds to fade, or blink, e.g. 'millis=500'\n"
"  'count'  -- number of times to blink, for /blink1/blink, e.g. 'count=3'\n"
"  'pattern'-- color pattern string (e.g. '3,00ffff,0.2,0,000000,0.2,0')\n"
"  'pname'  -- color pattern name from pattern list (e.g. 'red flash') \n"
"\n"
"Examples: \n"
"  /blink1/blue?bright=127 -- set blink1 blue, at half-intensity \n"
"  /blink1/fadeToRGB?rgb=FF00FF&millis=500 -- fade to purple over 500ms\n"
"  /blink1/pattern/play?pattern=3,00ffff,0.2,0,000000,0.2,0 -- blink cyan 3 times\n"
"  /blink1/servertickle?on=1&millis=5000 -- turn servertickle on with 5 sec timer\n"
"\n"
        );
}

void cache_flush(int idle_threshold_millis);

blink1_device* cache_getDeviceById(uint32_t id)
{
    int i = blink1_getCacheIndexById(id);
    blink1_device* dev=NULL;
    if( i>=0 ) {
        dev = cache_infos[i].dev;
    }
    // printf("cache_getDeviceById: %p from %d at %d\n", dev, id, i);
    if( !dev ) {
        dev = blink1_openById(id);
        if( !dev ) {
            cache_flush(0);
            blink1_enumerate();
            dev = blink1_openById(id);
            if( !dev ) {
                return NULL;
            }
        }
        i = blink1_getCacheIndexByDev(dev);
        // printf("cache_getDeviceById: %p to %d \n", dev, i);
        if( i>=0 ) {
            cache_infos[i].dev = dev;
        }
    }
    // printf("cache_getDeviceById: return %p\n", dev);
    return dev;
}

#define cache_return(dev) { cache_return_internal(dev); dev=NULL; }

void cache_return_internal( blink1_device* dev )
{
    int i = blink1_getCacheIndexByDev(dev);
    // printf("cache_return_internal: %p at %d\n", dev, i);
    if( i>=0 ) {
        cache_infos[i].atime = mg_millis();
    }
    else {
        blink1_close(dev);
    }
}

void cache_flush(int idle_threshold_millis)
{
    int64_t deadline = mg_millis() - idle_threshold_millis;
    int count = blink1_getCachedCount();
    for( int i=0; i< count; i++ ) {
        if( cache_infos[i].dev && cache_infos[i].atime < deadline ) {
            // printf("DEBUG cache_flush: id=%d handle=%p atime=%lld\n", i, cache_infos[i].dev, cache_infos[i].atime);
            blink1_close(cache_infos[i].dev)
            cache_infos[i].atime = 0;
        }
    }
}
void blink1_do_color(rgb_t rgb, uint32_t millis, uint32_t id,
                    uint8_t ledn, uint8_t bright, char* status)
{
    last_rgb.r = rgb.r; 
    last_rgb.g = rgb.g; 
    last_rgb.b = rgb.b;
    
    blink1_device* dev = cache_getDeviceById(id);
    if( !dev ) {
        sprintf(status+strlen(status), ": error: no blink1 found");
        return;
    }
    
    blink1_adjustBrightness( bright, &rgb.r, &rgb.g, &rgb.b);
    if( millis==0 ) { millis = 200; }
    int rc = blink1_fadeToRGBN( dev, millis, rgb.r,rgb.g,rgb.b, ledn );
    if( rc == -1 ) {
        fprintf(stderr, "error, couldn't fadeToRGB on blink1\n");
        sprintf(status+strlen(status), ": error, couldn't fadeToRGB on blink1");
    }
    else {
        sprintf(status, "blink1 set color #%2.2x%2.2x%2.2x", rgb.r,rgb.g,rgb.b);
    }
    cache_return(dev);
}

static void log_access(struct mg_connection *c, char* uri_str, int resp_code) {
    //CLF format: 127.0.0.1 - frank [10/Oct/2000:13:55:36 -0700] "GET /apache_pb.gif HTTP/1.0" 200 2326
    time_t rawtime;
    time( &rawtime );
    char date_str[100];
    strftime(date_str, sizeof(date_str), "%d/%b/%Y:%H:%M:%S %z", localtime(&rawtime));
    char ip_str[20];
    mg_snprintf(ip_str, 20, "%d.%d.%d.%d", c->rem.ip[0],c->rem.ip[1],c->rem.ip[2],c->rem.ip[3] );
    printf("%s - [%s] \"%s %s HTTP/1.1\" %d %d\n", ip_str, date_str, "GET", uri_str, resp_code, 0 ); // can't get response length I guess
}


static void ev_handler(struct mg_connection *c, int ev, void *ev_data)
{
    if(ev != MG_EV_HTTP_MSG) {
        return;
    }

    struct mg_http_message *hm = (struct mg_http_message *) ev_data;

    uint32_t id=0;
    uint8_t ledn=0, bright=0;
    char status[1000] = "";
    char uri_str[1000] = "";
    char tmpstr[1000] = "";
    char pattstr[1000] = "";
    char pnamestr[1000] = "";

    uint16_t millis = 0;
    rgb_t rgb = {0,0,0}; // for parsecolor
    uint8_t count = 0;
    int resp_code = 404;  // no found by default

    JSON_Value *json_root_val = json_value_init_object();
    JSON_Object *json_root_obj = json_value_get_object(json_root_val);

    struct mg_str* uri = &hm->uri;
    struct mg_str* querystr = &hm->query;

    mg_snprintf(uri_str, uri->len+1, "%s", uri->ptr); // uri->ptr gives us char ptr

    // parse all possible query args (it's just easier this way)
    if( mg_http_get_var(querystr, "millis", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = strtod(tmpstr,NULL);
        json_object_set_number(json_root_obj, "millis", millis);
    }
    if( mg_http_get_var(querystr, "time", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = 1000 * strtof(tmpstr,NULL);
        json_object_set_number(json_root_obj, "millis", millis);
    }
    if( mg_http_get_var(querystr, "rgb", tmpstr, sizeof(tmpstr)) > 0 ) {
        parsecolor( &rgb, tmpstr);
        json_object_set_string(json_root_obj, "rgb", tmpstr);
    }
    if( mg_http_get_var(querystr, "count", tmpstr, sizeof(tmpstr)) > 0 ) {
        count = strtod(tmpstr,NULL);
        json_object_set_number(json_root_obj, "count", count);
    }
    if( mg_http_get_var(querystr, "id", tmpstr, sizeof(tmpstr)) > 0 ) {
        char* pch;
        pch = strtok(tmpstr, " ,");
        int base = (strlen(pch)==8) ? 16:0;
        id = strtol(pch,NULL,base);
    }
    if( mg_http_get_var(querystr, "ledn", tmpstr, sizeof(tmpstr)) > 0 ) {
        ledn = strtod(tmpstr,NULL);
        json_object_set_number(json_root_obj, "ledn", ledn);
    }
    if( mg_http_get_var(querystr, "bright", tmpstr, sizeof(tmpstr)) > 0 ) {
        bright = strtod(tmpstr,NULL);
        json_object_set_number(json_root_obj, "bright", bright);
    }
    if( mg_http_get_var(querystr, "pattern", tmpstr, sizeof(tmpstr)) > 0 ) {
        strcpy(pattstr, tmpstr);
        json_object_set_string(json_root_obj, "pattern", pattstr);
    }
    if( mg_http_get_var(querystr, "pname", tmpstr, sizeof(tmpstr)) > 0 ) {
        strcpy(pnamestr, tmpstr);
        json_object_set_string(json_root_obj, "pname", pnamestr);
    }

    if( mg_vcmp( uri, "/blink1") == 0 ||
             mg_vcmp( uri, "/blink1/") == 0  ) {
        sprintf(status, "blink1 status");
        uint16_t msecs = 0;
        blink1_device* dev = cache_getDeviceById(id);
        if( dev ) {
            int rc = blink1_readRGB(dev, &msecs, &rgb.r,&rgb.g,&rgb.b, 0);
            if( rc==-1 ) {
                printf("error on readRGB\n");
            }
            cache_return(dev);
        }
    }
    else if( mg_vcmp( uri, "/blink1/lastColor") == 0 ||
             mg_vcmp( uri, "/blink1/lastcolor") == 0 ) {
        sprintf(status, "blink1 lastColor");
        
        uint16_t msecs = 0;
        blink1_device* dev = cache_getDeviceById(id);
        if( dev ) {
           int rc = blink1_readRGB(dev, &msecs, &rgb.r, &rgb.g, &rgb.b, 0);
           if( rc==-1 ) {
               printf("error on readRGB\n");
           }
           cache_return(dev);
        }
        
        sprintf(tmpstr, "#%2.2x%2.2x%2.2x", last_rgb.r, last_rgb.g, last_rgb.b);
        json_object_set_string(json_root_obj, "lastColor", tmpstr);
    }
    else if( mg_vcmp( uri, "/blink1/id") == 0 ||
             mg_vcmp( uri, "/blink1/id/") == 0 ||
             mg_vcmp( uri, "/blink1/list") == 0 ||
             mg_vcmp( uri, "/blink1/list/") == 0 ||
             mg_vcmp( uri, "/blink1/enumerate") == 0 ) {
        sprintf(status, "blink1 id");
        cache_flush(0);
        int c = blink1_enumerate();

        JSON_Value* json_serials_val = json_value_init_array();
        JSON_Array * json_serials_arr = json_array(json_serials_val);
        for( int i=0; i< c; i++ ) {
            sprintf(tmpstr, "%s", blink1_getCachedSerial(i));
            json_array_append_string(json_serials_arr, tmpstr);
        }
        json_object_set_value(json_root_obj, "blink1_serialnums", json_serials_val);

        const char* blink1_serialnum = blink1_getCachedSerial(0);
        if( blink1_serialnum ) {
            sprintf(tmpstr, "%s00000000", blink1_serialnum);
            json_object_set_string(json_root_obj, "blink1_id", tmpstr);
        }
    }
    else if( mg_vcmp( uri, "/blink1/off") == 0 ) {
        sprintf(status, "blink1 off");
        rgb.r = 0; rgb.g = 0; rgb.b = 0;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/on") == 0 ) {
        sprintf(status, "blink1 on");
        rgb.r = 255; rgb.g = 255; rgb.b = 255;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/red") == 0 ) {
        sprintf(status, "blink1 red");
        rgb.r = 255; rgb.g = 0; rgb.b = 0;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/green") == 0 ) {
        sprintf(status, "blink1 green");
        rgb.r = 0; rgb.g = 255; rgb.b = 0;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/blue") == 0 ) {
        sprintf(status, "blink1 blue");
        rgb.r = 0; rgb.g = 0; rgb.b = 255;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/cyan") == 0 ) {
        sprintf(status, "blink1 cyan");
        rgb.r = 0; rgb.g = 255; rgb.b = 255;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/yellow") == 0 ) {
        sprintf(status, "blink1 yellow");
        rgb.r = 255; rgb.g = 255; rgb.b = 0;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/magenta") == 0 ) {
        sprintf(status, "blink1 magenta");
        rgb.r = 255; rgb.g = 0; rgb.b = 255;
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp( uri, "/blink1/fadeToRGB") == 0 ) {
        sprintf(status, "blink1 fadeToRGB");
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp(uri, "/blink1/blink") == 0 ) {
        sprintf(status, "blink1 blink");
        if( rgb.r==0 && rgb.g==0 && rgb.b==0 ) { rgb.r=255; rgb.g=255; rgb.b=255; }
        if( count==0 ) { count = 3; }
        if( millis==0 ) { millis = 300; }
        int repeats = -1;
        patternline_t pattern[32];
        blink1_adjustBrightness(bright, &rgb.r, &rgb.g, &rgb.b);
        sprintf(tmpstr, "%d,#%2.2x%2.2x%2.2x,%f,%d,#000000,%f,0",
                count, rgb.r,rgb.g,rgb.b, (float)millis/1000.0, ledn,
                (float)millis/1000.0);
        msg("pattstr:%s\n", tmpstr);
        int pattlen = parsePattern(tmpstr, &repeats, pattern);

        blink1_device* dev = cache_getDeviceById(id);
        for( int i=0; i<pattlen; i++ ) {
            patternline_t pat = pattern[i];
            blink1_setLEDN(dev, pat.ledn);
            msg("  writing line %d: %2.2x,%2.2x,%2.2x : %d : %d\n",
                  i, pat.color.r,pat.color.g,pat.color.b, pat.millis, pat.ledn );
            blink1_writePatternLine(dev, pat.millis, pat.color.r, pat.color.g, pat.color.b, i);
        }
        blink1_playloop(dev, 1, 0/*startpos*/, pattlen-1/*endpos*/, count/*count*/);
        cache_return(dev);
    }
    
    else if( mg_vcmp(uri, "/blink1/pattern") == 0 ||
             mg_vcmp(uri, "/blink1/patterns") == 0 ||
             mg_vcmp(uri, "/blink1/pattern/") == 0 ||
             mg_vcmp(uri, "/blink1/patterns/") == 0 ) {       
        sprintf(status, "blink1 pattern list");
        int cnt = sizeof(blink1_patterns)/sizeof(blink1_pattern_info);
        
        JSON_Value* json_patterns_val = json_value_init_array();
        JSON_Array * json_patterns_arr = json_array(json_patterns_val);
        for( int i=0; i< cnt; i++ ) {
            JSON_Value* patt_entry_val = json_value_init_object();
            JSON_Object* patt_entry_obj = json_object(patt_entry_val);
            json_object_set_string(patt_entry_obj, "name", blink1_patterns[i].name);
            json_object_set_string(patt_entry_obj, "pattern", blink1_patterns[i].str);
            json_array_append_value(json_patterns_arr, patt_entry_val);
        }
        json_object_set_value(json_root_obj, "patterns", json_patterns_val);

    }
    /*
    else if( mg_vcmp(uri, "/blink1/pattern/add") == 0 ) {
        sprintf(result, "blink1 pattern add");
    }
    */
    else if( mg_vcmp(uri, "/blink1/pattern/play") == 0 ) {
        sprintf(status, "blink1 pattern play");
        /*
        if( pnamestr[0] != 0 && pattstr[0] != 0 ) {

        }
        */
        if( pnamestr[0] != 0 ) {
            const char* pstr = blink1_pattern_find(pnamestr);
            sprintf("%s", pattstr, pstr);
        }

        if( pattstr[0] == 0 ) { // no pattern
        }

        patternline_t pattern[32];
        int repeats = -1;
        int pattlen = parsePattern(pattstr, &repeats, pattern);
        if( !count ) { count = repeats; }

        blink1_device* dev = cache_getDeviceById(id);

        msg("pname:%s pattstr:%s pattlen:%d, repeats:%d\n", pnamestr, pattlen,repeats);
        for( int i=0; i<pattlen; i++ ) {
            patternline_t pat = pattern[i];
            blink1_setLEDN(dev, pat.ledn);
            msg("    writing line %d: %2.2x,%2.2x,%2.2x : %d : %d\n",
                  i, pat.color.r,pat.color.g,pat.color.b, pat.millis, pat.ledn );
            blink1_writePatternLine(dev, pat.millis, pat.color.r, pat.color.g, pat.color.b, i);
        }
        blink1_playloop(dev, 1 /*play/pause*/, 0 /*startpos*/, pattlen-1 /*endpos*/, count /*count*/);
        cache_return(dev);
    }
    else if( mg_vcmp(uri, "/blink1/pattern/stop") == 0 ) {
        sprintf(status, "blink1 pattern stop");
        
        blink1_device* dev = cache_getDeviceById(id);
        blink1_playloop(dev, 0, 0/*startpos*/, 0/*endpos*/, 0/*count*/);
        cache_return(dev);
    }
    // like "/blink1/blink" but does it on the server
    else if( mg_vcmp( uri, "/blink1/blinkserver") == 0 ) {
        sprintf(status, "blink1 blinkserver");
        //if( r==0 && g==0 && b==0 ) { r = 255; g = 255; b = 255; }
        if( millis==0 ) { millis = 200; }

        blink1_device* dev = cache_getDeviceById(id);
        //blink1_adjustBrightness( bright, &r, &g, &b);
        //msg("rgb:%d,%d,%d\n",r,g,b);
        for( int i=0; i<count; i++ ) {
            blink1_fadeToRGBN( dev, millis/2, rgb.r,rgb.g,rgb.b, ledn );
            blink1_sleep( millis/2 ); // fixme
            blink1_fadeToRGBN( dev, millis/2, 0,0,0, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        cache_return(dev);
    }
    //else if( mg_http_match_uri(hm, "/blink1/servertickle/*")) { 
    else if( mg_vcmp( uri, "/blink1/servertickle/on") == 0 ||
             mg_vcmp( uri, "/blink1/servertickle/off") == 0 ) {
        bool st_on = (mg_vcmp(uri, "/blink1/servertickle/on") == 0);
        if( millis==0 ) { millis = 2000; }
        sprintf(status, "blink1 servertickle %s", st_on? "on":"off");
        uint8_t start_pos = 0;
        uint8_t end_pos = 0;
        uint8_t st_off_state = 0;
        blink1_device* dev = cache_getDeviceById(id);
        blink1_serverdown( dev, st_on, millis, st_off_state, start_pos, end_pos );
        cache_return(dev);

        json_object_set_string(json_root_obj, "on", st_on? "1":"0");
    }
    else if( mg_vcmp( uri, "/blink1/random") == 0 ) {
        sprintf(status, "blink1 random");
        if( count==0 ) { count = 1; }
        if( millis==0 ) { millis = 200; }
        blink1_device* dev = cache_getDeviceById(id);
        for( int i=0; i<count; i++ ) {
            uint8_t r = rand() % 255;
            uint8_t g = rand() % 255;
            uint8_t b = rand() % 255 ;
            blink1_adjustBrightness( bright, &r, &g, &b);
            blink1_fadeToRGBN( dev, millis/2, r,g,b, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        cache_return(dev);
    }

    // set JSON values that exist for all requests
    json_object_set_string(json_root_obj, "status", status);
    json_object_set_string(json_root_obj, "version", blink1_server_version);
    sprintf(tmpstr, "#%2.2x%2.2x%2.2x", rgb.r, rgb.g, rgb.b);
    json_object_set_string(json_root_obj, "rgb", tmpstr);
    json_object_set_number(json_root_obj, "millis", millis);

    
    // check if we've handled json        
    if( status[0] != '\0' ) {  // json handled
        resp_code = 200;
        sprintf(tmpstr, "#%2.2x%2.2x%2.2x", rgb.r,rgb.g,rgb.b );
        mg_printf(c, "HTTP/1.1 %d OK\r\n", resp_code);
        mg_printf(c, "Content-type: application/json\r\n");
        mg_printf(c, "X-Content-Type-Options: nosniff\r\n");
        mg_printf(c, "Transfer-Encoding: chunked\r\n\r\n");
        
        char* json_string = json_serialize_to_string_pretty(json_root_val);
        mg_http_printf_chunk(c, json_string);

        json_free_serialized_string(json_string);
        json_value_free(json_root_val);
    
        mg_http_write_chunk(c, "", 0); /* Send empty chunk, the end of response */
    }
    else {
        if( show_html ) {
            if( mg_vcmp( uri, "/") == 0 ) {
                resp_code = 302;
                mg_http_reply(c, resp_code, "Location: /index.html\r\n", "");
            }
            else {
                resp_code = 200; // I guess
                struct mg_http_serve_opts opts = {
                    .root_dir = "/",
                    .fs = &mg_fs_packed
                };
                mg_http_serve_dir(c, ev_data, &opts);
            }
        }
        else if ( mg_vcmp( uri, "/") == 0 ) {
            sprintf(status, "Welcome to %s api server. All URIs start with '/blink1'.", blink1_server_name);
            //sprintf(status, "%s. \nSupported URIs:\n", status);
            //for( int i=0; i< sizeof(supported_urls)/sizeof(url_info); i++ ) {
            //    sprintf(status+strlen(status), " %s - %s\n",
            //            supported_urls[i].url, supported_urls[i].desc);
        }

        else {
            mg_http_reply(c, 404, NULL, "Not found\n");        
        }
    }

    // access logging
    if( enable_logging ) { 
        log_access(c, uri_str, resp_code);
    }
}

// ----------------------------------------------------------------------

// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo) {
  s_signo = signo;
}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    struct mg_connection *c;
    int port;
    
    setbuf(stdout,NULL);  // turn off stdout buffering for Windows
    srand( time(NULL) * getpid() );
    
    // parse options
    int option_index = 0, opt;
    char* opt_str = "qvhp:H:U:A:Nl";
    static struct option loptions[] = {
      //{"verbose",    optional_argument, 0,      'v'},
      //{"quiet",      optional_argument, 0,      'q'},
      //{"baseurl",    required_argument, 0,      'U'},
        {"host",       required_argument, 0,      'H'},
        {"port",       required_argument, 0,      'p'},
        {"no-html",    no_argument,       0,      'N'},
        {"logging",    no_argument,       0,      'l'},
        {"help",       no_argument, 0,            'h'},
        {"version",    no_argument, 0,            'V'},
        {NULL,         0,           0,             0 },
    };

    while(1) {
        opt = getopt_long_only(argc, argv, opt_str, loptions, &option_index);
        if (opt==-1) break; // parsed all the args
        switch (opt) {
        case 'h':
            usage();
            exit(1);
            break;
        case 'V':
            printf("%s version %s\n", blink1_server_name,blink1_server_version);
            exit(1);
            break;
        case 'v':
            break;
        case 'N':
            show_html = false;
            break;
        case 'l':
            enable_logging = true;
            break;
        case 'H':
            strncpy(http_listen_host, optarg, sizeof(http_listen_host));
            break;
        case 'p':
            port = strtod(optarg,NULL);
            if( port > 0 && port < 65535 ) {
                http_listen_port = port;
            }
            else {
                printf("bad port specified: %s\n", optarg);
            }
            break;
        case 'd':
            // s_http_server_opts.document_root = optarg;
            // char path[MAX_PATH_SIZE];
            // cs_stat_t st;
            // for( int i=0; i< sizeof(index_files)/sizeof(char* const); i++ ) {
            //     snprintf(path, sizeof(path), "%s/%s", s_http_server_opts.document_root, index_files[i]);
            //     if( mg_stat(path, &st) == 0 ) {
            //         serve_index = true;
            //         break;
            //     }
            // }
            break;
        }
    } //while(1) arg parsing

    printf("%s version %s: running on http://%s:%d/ (%s)\n",
          blink1_server_name, blink1_server_version, http_listen_host,
          http_listen_port,  (show_html) ? "html help enabled": "no html help");

    snprintf(http_listen_url, sizeof(http_listen_url), "http://%s:%d/",
           http_listen_host, http_listen_port);

    // if( s_http_server_opts.document_root ) {
    // printf("  serving static HTML %s (with%s root index)\n",
    //            //s_http_server_opts.document_root, serve_index ? "" : "out");
    //            s_http_server_opts.fs = &mg_fs_packed; // Set packed ds as a file system
    //            mg_http_serve_dir(c, ev_data, &opts);
    // // }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    mg_mgr_init(&mgr);

    if ((c = mg_http_listen(&mgr, http_listen_url, ev_handler, &mgr)) == NULL) {
      MG_LOG(MG_LL_ERROR, ("Cannot listen on %s.", http_listen_url));
      exit(EXIT_FAILURE);
    }

    while (s_signo == 0) {
        mg_mgr_poll(&mgr, 1000);
        cache_flush(idle_atime);
    }
    mg_mgr_free(&mgr);

    return 0;
}
