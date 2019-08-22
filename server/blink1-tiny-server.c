/*
 *
 * blink1-tiny-server -- a small cross-platform REST/JSON server for
 *                       controlling a blink(1) device
 *
 *
 * Supported URLs:
 *
 *  localhost:8000/blink1/on
 *  localhost:8000/blink1/off
 *  localhost:8000/blink1/red
 *  localhost:8000/blink1/green
 *  localhost:8000/blink1/blue
 *  localhost:8000/blink1/blink?rgb=%23ff0ff&time=1.0&count=3
 *  localhost:8000/blink1/fadeToRGB?rgb=%23ff00ff&time=1.0
 *
 *
 */

#include <getopt.h>    // for getopt_long()

#include "mongoose.h"

#include "blink1-lib.h"

// normally this is obtained from git tags and filled out by the Makefile
#ifndef BLINK1_VERSION
#define BLINK1_VERSION "v0.0"
#endif

const char* blink1_server_name = "blink1-tiny-server";
const char* blink1_server_version = BLINK1_VERSION;

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

typedef struct Url_Info
{
    char url[100];
    char desc[100];
} url_info;

// FIXME: how to make Emacs format these better?
url_info supported_urls[]
= {
   {"/blink1/",      "simple status page"},
   {"/blink1/on",    "turn blink(1) full bright white"},
   {"/blink1/red",   "turn blink(1) solid red"},
   {"/blink1/green", "turn blink(1) solid green"},
   {"/blink1/blue",  "turn blink(1) solid blue"},
   {"/blink1/fadeToRGB", "turn blink(1) specified RGB color"},
   {"/blink/blink",  "blink the blink(1) the specified RGB color"},
   {"/blink/random",  "turn the blink(1) a random color"}
};


void usage()
{
    
    fprintf(stderr,
"Usage: \n"
"  %s [options]\n"
"where [options] can be:\n"
"  --port port, -p port    port to listen on (default 8000)\n"
"  --help, -h              this help page\n"
"\n",
            blink1_server_name);

    fprintf(stderr,
"Supported URIs:\n");
    for( int i=0; i< sizeof(supported_urls)/sizeof(url_info); i++ ) {
        fprintf(stderr,"  %s -- %s\n", supported_urls[i].url, supported_urls[i].desc);
    }
    fprintf(stderr,"\n");
    fprintf(stderr,
"Supported query arguments: (not all urls support all args)\n"
"  'rgb'    -- hex RGB color code. e.g. 'rgb=%%23FF9900'\n"
"  'time'   -- time in seconds. e.g. 'time=0.5' \n"
"  'millis' -- time in milliseconds. e.g. 'millis=500' \n"
"  'bright' -- brightness, 1-255, 0=full e.g. half-bright 'bright=127'\n"
"  'ledn'   -- which LED to set. 0=all/1=top/2=bot, e.g. 'ledn=0'\n"
"  'milils' -- milliseconds to fade, or blink, e.g. 'millis=500'\n"
"  'count'  -- number of times to blink, for /blink1/blink, e.g. 'count=3'\n"
"\n"
"Examples: \n"
"  /blink1/blue?bright=127 -- set blink1 blue, at half-intensity \n"
"  /blink1/fadeToRGB?rgb=%%23FF00FF&millis=500 -- fade to purple over 500ms\n"
            
"\n"
            
            );
}

// used in ev_handler below
// uses variables bright,r,g,b,ledn, result
#define do_blink1_color() \
    blink1_device* dev = blink1_open(); \
    blink1_adjustBrightness( bright, &r, &g, &b); \
    if( blink1_fadeToRGBN( dev, millis, r,g,b, ledn ) == -1 ) {   \
        fprintf(stderr, "fadeToRGBN: error blink1 device error\n"); \
        sprintf(result, "%s: error, couldn't find blink1", result); \
    } \
    else { \
        sprintf(result, "blink1 set color #%2.2x%2.2x%2.2x", r,g,b);  \
    } \
    blink1_close(dev); 


static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    struct http_message *hm = (struct http_message *) ev_data;

    if( ev != MG_EV_HTTP_REQUEST ) {
        return;
    }

    uint8_t r,g,b;
    uint8_t ledn=0, bright=0;
    char result[1000];
    result[0] = 0;
    char uristr[1000];
    char tmpstr[1000];
    int rc;
    uint16_t millis = 100;
    rgb_t rgb = {0,0,0};
    uint8_t count = 1;

    struct mg_str* uri = &hm->uri;
    struct mg_str* querystr = &hm->query_string;

    snprintf(uristr, uri->len+1, "%s", uri->p);

    if( mg_get_http_var(querystr, "millis", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "time", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = 1000 * strtof(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "rgb", tmpstr, sizeof(tmpstr)) > 0 ) {
        parsecolor( &rgb, tmpstr);
        r = rgb.r; g = rgb.g; b = rgb.b;
    }
    if( mg_get_http_var(querystr, "count", tmpstr, sizeof(tmpstr)) > 0 ) {
        count = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "ledn", tmpstr, sizeof(tmpstr)) > 0 ) {
        ledn = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "bright", tmpstr, sizeof(tmpstr)) > 0 ) {
        bright = strtod(tmpstr,NULL);
    }

    if( mg_vcmp( uri, "/") == 0 ) {
        sprintf(result, "Welcome to %s api server."
                "All URIs start with '/blink1'. Supported URIs:\n", blink1_server_name);
        for( int i=0; i< sizeof(supported_urls)/sizeof(url_info); i++ ) {
            sprintf(result,"%s %s - %s\n", result, supported_urls[i].url, supported_urls[i].desc);
        } // FIXME: result is fixed length
    }
    else if( mg_vcmp( uri, "/blink1") == 0 ||
             mg_vcmp( uri, "/blink1/") == 0  ) {
        sprintf(result, "blink1 status");
    }
    else if( mg_vcmp( uri, "/blink1/off") == 0 ) {
        sprintf(result, "blink1 off");
        r = 0; g = 0; b = 0;
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/on") == 0 ) {
        sprintf(result, "blink1 on");
        r = 255; g = 255; b = 255;
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/red") == 0 ) {
        sprintf(result, "blink1 red");
        r = 255; g = 0; b = 0;
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/green") == 0 ) {
        sprintf(result, "blink1 green");
        r = 0; g = 255; b = 0;
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/blue") == 0 ) {
        sprintf(result, "blink1 on");
        r = 0; g = 0; b = 255;
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/fadeToRGB") == 0 ) {
        sprintf(result, "blink1 fadeToRGB");
        do_blink1_color();
    }
    else if( mg_vcmp( uri, "/blink1/blink") == 0 ) {
        sprintf(result, "blink1 blink");
        if( r==0 && g==0 && b==0 ) { r = 255; g = 255; b = 255; }
        blink1_device* dev = blink1_open();
        blink1_adjustBrightness( bright, &r, &g, &b);
        for( int i=0; i<count; i++ ) {
            blink1_fadeToRGBN( dev, millis/2, r,g,b, ledn );
            blink1_sleep( millis/2 ); // fixme
            blink1_fadeToRGBN( dev, millis/2, 0,0,0, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else if( mg_vcmp( uri, "/blink1/random") == 0 ) {
        sprintf(result, "blink1 random");
        srand( time(NULL) * getpid() );
        blink1_device* dev = blink1_open();
        for( int i=0; i<count; i++ ) {
            r = rand() % 255;
            g = rand() % 255;
            b = rand() % 255 ;
            blink1_adjustBrightness( bright, &r, &g, &b);
            blink1_fadeToRGBN( dev, millis/2, r,g,b, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else {
        sprintf(result, "%s; unrecognized uri", result);
        //mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
    }

    if( result[0] != '\0' ) {
        sprintf(tmpstr, "#%2.2x%2.2x%2.2x", r,g,b );
        mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc,
                             "{\n"
                             "\"uri\":  \"%s\",\n"
                             "\"result\":  \"%s\",\n"
                             "\"millis\": \"%d\",\n"
                             "\"rgb\": \"%s\",\n"
                             "\"bright\": \"%d\",\n"
                             "\"ledn\": \"%d\",\n"
                             "\"count\": \"%d\",\n"
                             "\"millis\": \"%d\",\n"
                             "\"version\": \"%s\"\n"
                             "}\n",
                             uristr,
                             result,
                             millis,
                             tmpstr,
                             bright,
                             ledn,
                             count,
                             millis,
                             blink1_server_version
                             );
        mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
    }

}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct mg_bind_opts bind_opts;
    int i;
    char *cp;
    const char *err_str;

    mg_mgr_init(&mgr, NULL);

    // parse options
    int option_index = 0, opt;
    char* opt_str = "qvhp:";
    static struct option loptions[] = {
      //{"verbose",    optional_argument, 0,      'v'},
      //{"quiet",      optional_argument, 0,      'q'},
        {"port",       required_argument, 0,      'p'},
        {"help",       no_argument, 0,            'h'},
        {"version",    no_argument, 0,            'V'},
    };
    
    while(1) {
        opt = getopt_long(argc, argv, opt_str, loptions, &option_index);
        if (opt==-1) break; // parsed all the args
        switch (opt) {
        case 'V': 
            printf("%s version %s\n", blink1_server_name,blink1_server_version);
            exit(1);
            break;
        case 'v':
            break;
        case 'p':
            //port = strtol(optarg,NULL,10);
            s_http_port = optarg; //argv[++i];
            break;
        case 'h':
            usage();
            exit(1);
            break;
        }
    } //while(1) arg parsing

   
    // Set HTTP server options 
    memset(&bind_opts, 0, sizeof(bind_opts));
    bind_opts.error_string = &err_str;

    nc = mg_bind_opt(&mgr, s_http_port, ev_handler, bind_opts);
    if (nc == NULL) {
        fprintf(stderr, "Error starting server on port %s: %s\n", s_http_port,
                *bind_opts.error_string);
        exit(1);
    }

    mg_set_protocol_http_websocket(nc);

    s_http_server_opts.enable_directory_listing = "no";

    printf("%s version %s: running on port %s\n",
           blink1_server_name, blink1_server_version, s_http_port);

    for (;;) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return 0;
}
