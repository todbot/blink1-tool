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

#include <getopt.h>    // for getopt_long_only()

#include "mongoose.h"

#include "blink1-lib.h"

//#include "dict.h"
#include "Dictionary.h"
#include "Dictionary.c"

// normally this is obtained from git tags and filled out by the Makefile
#ifndef BLINK1_VERSION
#define BLINK1_VERSION "v0.0"
#endif

const char* blink1_server_name = "blink1-tiny-server";
const char* blink1_server_version = BLINK1_VERSION;

static const char *s_http_port = "8000";
static struct mg_serve_http_opts s_http_server_opts;

DictionaryRef       patterndict;
DictionaryCallbacks patterndictc;
        
typedef struct _url_info
{
    char url[100];
    char desc[100];
} url_info;

// FIXME: how to make Emacs format these better?
url_info supported_urls[]
= {
    {"/blink1/",              "simple status page"},
    {"/blink1/id",            "get blink1 serial number"},
    {"/blink1/on",            "turn blink(1) full bright white"},
    {"/blink1/red",           "turn blink(1) solid red"},
    {"/blink1/green",         "turn blink(1) solid green"},
    {"/blink1/blue",          "turn blink(1) solid blue"},
    {"/blink1/fadeToRGB",     "turn blink(1) specified RGB color"},
    {"/blink1/blink",         "blink the blink(1) the specified RGB color"},
    {"/blink1/pattern/play",  "play color pattern specified by 'pattern' arg"},
    {"/blink1/random",        "turn the blink(1) a random color"}
};


void usage()
{

    fprintf(stderr,
"Usage: \n"
"  %s [options]\n"
"where [options] can be:\n"
"  --port port, -p port    port to listen on (default 8000)\n"
"  --version                version of this program\n"
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
"  'rgb'    -- hex RGB color code. e.g. 'rgb=FF9900' or 'rgb=%%23FF9900\n"
"  'time'   -- time in seconds. e.g. 'time=0.5' \n"
"  'bright' -- brightness, 1-255, 0=full e.g. half-bright 'bright=127'\n"
"  'ledn'   -- which LED to set. 0=all/1=top/2=bot, e.g. 'ledn=0'\n"
"  'millis' -- milliseconds to fade, or blink, e.g. 'millis=500'\n"
"  'count'  -- number of times to blink, for /blink1/blink, e.g. 'count=3'\n"
"  'pattern'-- color pattern string (e.g. '3,00ffff,0.2,0,000000,0.2,0')\n"
"\n"
"Examples: \n"
"  /blink1/blue?bright=127 -- set blink1 blue, at half-intensity \n"
"  /blink1/fadeToRGB?rgb=FF00FF&millis=500 -- fade to purple over 500ms\n"
"  /blink1/pattern/play?pattern=3,00ffff,0.2,0,000000,0.2,0 -- blink cyan 3 times\n"
            
"\n"
            
            );
}

void blink1_do_color(rgb_t rgb, uint32_t millis, uint32_t id,
                    uint8_t ledn, uint8_t bright, char* status)
{
    blink1_enumerate();
    blink1_device* dev = blink1_openById(id);
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
    blink1_close(dev);
}

//
// 
void DictionaryPrintAsJsonMg(struct mg_connection *nc, DictionaryRef d )
{
    size_t                  i;
    struct DictionaryItem * item;

    if( d == NULL || d->items == NULL ) { return; }

    for( i = 0; i < d->size; i++ ) {
        item = d->items[ i ];
        
        while( item ) {
            const char* v = item->value;
            if( v[0] == '[' || v[0] == '{' ) { // hack if value is json array/object 
                mg_printf_http_chunk(nc,"\"%s\": %s,\n", item->key,item->value);
            } else { 
                mg_printf_http_chunk(nc,"\"%s\": \"%s\",\n", item->key,item->value);
            }
            item = item->next;
        }
    }
}


static void ev_handler(struct mg_connection *nc, int ev, void *ev_data)
{
    struct http_message *hm = (struct http_message *) ev_data;

    if( ev != MG_EV_HTTP_REQUEST ) {
        return;
    }

    uint32_t id=0;
    uint8_t ledn=0, bright=0;
    char status[1000];  status[0] = 0;
    char uristr[1000];
    char tmpstr[1000];
    char pattstr[1000];
    char pnamestr[1000];
    //int rc;
    uint16_t millis = 0;
    rgb_t rgb = {0,0,0}; // for parsecolor
    uint8_t count = 0;

    DictionaryCallbacks resultsdictc = DictionaryStandardStringCallbacks();
    DictionaryRef resultsdict = DictionaryCreate( 100, &resultsdictc );
    
    struct mg_str* uri = &hm->uri;
    struct mg_str* querystr = &hm->query_string;

    snprintf(uristr, uri->len+1, "%s", uri->p);

    DictionaryInsert(resultsdict, "uri", uristr);
    DictionaryInsert(resultsdict, "version", blink1_server_version);
    
    // parse all possible query args (it's just easier this way)
    if( mg_get_http_var(querystr, "millis", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "time", tmpstr, sizeof(tmpstr)) > 0 ) {
        millis = 1000 * strtof(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "rgb", tmpstr, sizeof(tmpstr)) > 0 ) {
        parsecolor( &rgb, tmpstr);
    }
    if( mg_get_http_var(querystr, "count", tmpstr, sizeof(tmpstr)) > 0 ) {
        count = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "id", tmpstr, sizeof(tmpstr)) > 0 ) {
        char* pch;
        pch = strtok(tmpstr, " ,");
        int base = (strlen(pch)==8) ? 16:0;
        id = strtol(pch,NULL,base);
    }
    if( mg_get_http_var(querystr, "ledn", tmpstr, sizeof(tmpstr)) > 0 ) {
        ledn = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "bright", tmpstr, sizeof(tmpstr)) > 0 ) {
        bright = strtod(tmpstr,NULL);
    }
    if( mg_get_http_var(querystr, "pattern", tmpstr, sizeof(tmpstr)) > 0 ) {
        strcpy(pattstr, tmpstr);
    }
    if( mg_get_http_var(querystr, "pname", tmpstr, sizeof(tmpstr)) > 0 ) {
        strcpy(pnamestr, tmpstr);
    }
    
    // parse URI requests
    if( mg_vcmp( uri, "/") == 0 ) {
        sprintf(status, "Welcome to %s api server. "
                "All URIs start with '/blink1'. \nSupported URIs:\n", blink1_server_name);
        for( int i=0; i< sizeof(supported_urls)/sizeof(url_info); i++ ) {
            sprintf(status+strlen(status), " %s - %s\n", 
                    supported_urls[i].url, supported_urls[i].desc);
        } // FIXME: result is fixed length
    }
    else if( mg_vcmp( uri, "/blink1") == 0 ||
             mg_vcmp( uri, "/blink1/") == 0  ) {
        sprintf(status, "blink1 status");
    }
    else if( mg_vcmp( uri, "/blink1/id") == 0 ||
             mg_vcmp( uri, "/blink1/id/") == 0 ||
             mg_vcmp( uri, "/blink1/enumerate") == 0  ) {
        sprintf(status, "blink1 id");
        int c = blink1_enumerate();

        sprintf(tmpstr,"[");
        for( int i=0; i< c; i++ ) {
            sprintf(tmpstr+strlen(tmpstr), "\"%s\"", blink1_getCachedSerial(i));
            if( i!=c-1 ) { sprintf(tmpstr+strlen(tmpstr), ","); } // ugh
        }
        sprintf(tmpstr+strlen(tmpstr), "]");
        DictionaryInsert(resultsdict, "blink1_serialnums", tmpstr);
        
        const char* blink1_serialnum = blink1_getCachedSerial(0);
        if( blink1_serialnum ) {
            sprintf(tmpstr, "%s00000000", blink1_serialnum);
            DictionaryInsert(resultsdict, "blink1_id", tmpstr);
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
    else if( mg_vcmp( uri, "/blink1/fadeToRGB") == 0 ) {
        sprintf(status, "blink1 fadeToRGB");
        blink1_do_color(rgb, millis, id, ledn, bright, status);
    }
    else if( mg_vcmp(uri, "/blink1/blink") == 0 ) {
        sprintf(status, "blink1 blink");
        if( rgb.r==0 && rgb.g==0 && rgb.b==0 ) { rgb.r=255;rgb.g=255;rgb.b=255; }
        if( count==0 ) { count = 3; }
        if( millis==0 ) { millis = 300; }
        int repeats = -1;
        patternline_t pattern[32];
        blink1_adjustBrightness( bright, &rgb.r, &rgb.g, &rgb.b);
        sprintf(tmpstr, "%d,#%2.2x%2.2x%2.2x,%f,%d,#000000,%f,0",
                count, rgb.r,rgb.g,rgb.b, (float)millis/1000.0, ledn,
                (float)millis/1000.0);
        msg("pattstr:%s\n", tmpstr);
        int pattlen = parsePattern( tmpstr, &repeats, pattern);
        
        blink1_enumerate();
        blink1_device* dev = blink1_openById(id);
        for( int i=0; i<pattlen; i++ ) {
            patternline_t pat = pattern[i];
            blink1_setLEDN(dev, pat.ledn);
            msg("  writing line %d: %2.2x,%2.2x,%2.2x : %d : %d\n",
                  i, pat.color.r,pat.color.g,pat.color.b, pat.millis, pat.ledn );
            blink1_writePatternLine(dev, pat.millis, pat.color.r, pat.color.g, pat.color.b, i);
        }
        blink1_playloop(dev, 1, 0/*startpos*/, pattlen-1/*endpos*/, count/*count*/);
        blink1_close(dev);
    }
    /*
    else if( mg_vcmp(uri, "/blink1/pattern") == 0 ||
             mg_vcmp(uri, "/blink1/pattern/") == 0 ) {

        tmpstr
        DictionaryInsert(statussdict, "patterns", tmpstr);
        
    }
    else if( mg_vcmp(uri, "/blink1/pattern/add") == 0 ) {
        sprintf(result, "blink1 pattern add");
        DictionaryInsert(patterndict, pnamestr, pattstr);
        DictionaryInsert(resultsdict, "pattern", pattstr);
    }
    */
    else if( mg_vcmp(uri, "/blink1/pattern/play") == 0 ) {
        sprintf(status, "blink1 pattern play");
        /*
        if( pnamestr[0] != 0 && pattstr[0] != 0 ) { 
            DictionaryInsert(patterndict, pnamestr, pattstr);
        }
        if( pnamestr[0] != 0 ) {
            DictionaryGetValue(patterndict, pnamestr);
        }
        */
        if( pattstr[0] == 0 ) { // no pattern
        }
        
        patternline_t pattern[32];
        int repeats = -1;
        int pattlen = parsePattern( pattstr, &repeats, pattern);
        if( !count ) { count = repeats; }
        blink1_enumerate();
        blink1_device* dev = blink1_openById(id);
        msg("pattlen:%d, repeats:%d\n", pattlen,repeats);
        for( int i=0; i<pattlen; i++ ) {
            patternline_t pat = pattern[i];
            blink1_setLEDN(dev, pat.ledn);
            msg("    writing line %d: %2.2x,%2.2x,%2.2x : %d : %d\n",
                  i, pat.color.r,pat.color.g,pat.color.b, pat.millis, pat.ledn );
            blink1_writePatternLine(dev, pat.millis, pat.color.r, pat.color.g, pat.color.b, i);
        }
        blink1_playloop(dev, 1, 0/*startpos*/, pattlen-1/*endpos*/, count/*count*/);
        blink1_close(dev);
    }
    else if( mg_vcmp( uri, "/blink1/blinkserver") == 0 ) {
        sprintf(status, "blink1 blink");
        //if( r==0 && g==0 && b==0 ) { r = 255; g = 255; b = 255; }
        if( millis==0 ) { millis = 200; }
        blink1_enumerate();
        blink1_device* dev = blink1_openById(id);
        //blink1_adjustBrightness( bright, &r, &g, &b);
        //msg("rgb:%d,%d,%d\n",r,g,b);
        for( int i=0; i<count; i++ ) {
            blink1_fadeToRGBN( dev, millis/2, rgb.r,rgb.g,rgb.b, ledn );
            blink1_sleep( millis/2 ); // fixme
            blink1_fadeToRGBN( dev, millis/2, 0,0,0, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else if( mg_vcmp( uri, "/blink1/random") == 0 ) {
        sprintf(status, "blink1 random");
        if( count==0 ) { count = 1; }
        if( millis==0 ) { millis = 200; }
        srand( time(NULL) * getpid() );
        blink1_enumerate();
        blink1_device* dev = blink1_openById(id);
        for( int i=0; i<count; i++ ) {
            uint8_t r = rand() % 255;
            uint8_t g = rand() % 255;
            uint8_t b = rand() % 255 ;
            blink1_adjustBrightness( bright, &r, &g, &b);
            blink1_fadeToRGBN( dev, millis/2, r,g,b, ledn );
            blink1_sleep( millis/2 ); // fixme
        }
        blink1_close(dev);
    }
    else if( s_http_server_opts.document_root != NULL ) {
        mg_serve_http(nc, hm, s_http_server_opts); /* Serve static content */
    }
    else {
        sprintf(status+strlen(status), ": unrecognized uri");
    }

    if( status[0] != '\0' ) {
        sprintf(tmpstr, "#%2.2x%2.2x%2.2x", rgb.r,rgb.g,rgb.b );
        mg_printf(nc, "%s", "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n");
        mg_printf_http_chunk(nc,
                             "{\n");

        DictionaryPrintAsJsonMg(nc, resultsdict);
        
        mg_printf_http_chunk(nc,
                             "\"millis\": %d,\n" 
                             "\"rgb\": \"%s\",\n"
                             "\"ledn\": %d,\n"
                             "\"bright\": %d,\n"
                             "\"count\": %d,\n"
                             "\"status\":  \"%s\"\n",
                             millis,
                             tmpstr,
                             ledn,
                             bright,
                             count,
                             status
                             );

        mg_printf_http_chunk(nc,
                             "}\n"
                             );
        
        mg_send_http_chunk(nc, "", 0); /* Send empty chunk, the end of response */
    }

}

int main(int argc, char *argv[]) {
    struct mg_mgr mgr;
    struct mg_connection *nc;
    struct mg_bind_opts bind_opts;
    //int i;
    //char *cp;
    const char *err_str;

    mg_mgr_init(&mgr, NULL);

    patterndictc = DictionaryStandardStringCallbacks();
    patterndict = DictionaryCreate( 100, &patterndictc );
        
    // parse options
    int option_index = 0, opt;
    char* opt_str = "qvhp:";
    static struct option loptions[] = {
      //{"verbose",    optional_argument, 0,      'v'},
      //{"quiet",      optional_argument, 0,      'q'},
        {"document_root", required_argument, 0,   'd'},
        {"port",       required_argument, 0,      'p'},
        {"help",       no_argument, 0,            'h'},
        {"version",    no_argument, 0,            'V'},
        {NULL,         0,           0,             0 },
    };
    
    while(1) {
        opt = getopt_long_only(argc, argv, opt_str, loptions, &option_index);
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
        case 'd':
            s_http_server_opts.document_root = optarg;
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
