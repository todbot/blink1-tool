/* 
 * blink1-tool.c -- command-line tool for controlling blink(1) usb rgb leds
 *
 * 2012, Tod E. Kurt, http://todbot.com/blog/ , http://thingm.com/
 *
 *
 * Fade to RGB value #FFCC33 in 50 msec:
 * ./blink1-tool --hidwrite 0x63,0xff,0xcc,0x33,0x00,0x32
 * ./blink1-tool -m 50 -rgb 0xff,0xcc,0x33
 *
 * Read EEPROM position 1:
 * ./blink1-tool --hidwrite 0x65,0x01 && ./blink1-tool --hidread
 * ./blink1-tool --eeread 1
 *
 * Write color pattern entry {#EEDD44, 50} to position 2
 * ./blink1-tool --hidwrite 0x50,0xee,0xdd,0x44,0x00,0x32,0x2
 *
 *
 */

#include <stdio.h>
#include <stdarg.h>    // vararg stuff
#include <string.h>    // for memset(), strcmp(), et al
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>    // for getopt_long()
#include <time.h>
#include <unistd.h>    // getuid()

#include "blink1-lib.h"

// set to 1 to enable mk3 features (or really the display of those features)
#define ENABLE_MK3  0

// normally this is obtained from git tags and filled out by the Makefile
#ifndef BLINK1_VERSION
#define BLINK1_VERSION "v0.0"
#endif

#ifdef _WIN32
#define getpid _getpid
#endif

const int millisDefault = 300;
const int32_t delayMillisDefault = 500;
int millis = -1;
int32_t delayMillis = -1;
int numDevicesToUse = 1;

blink1_device* dev;
uint32_t  deviceIds[blink1_max_devices];

int verbose;
int quiet=0;

/*
  TBD: replace printf()s with something like this
void logpri(int loglevel, char* fmt, ...)
{
    if( loglevel < verbose ) return;
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}
*/

// --------------------------------------------------------------------------- 

//
static void usage(char *myName)
{
    fprintf(stderr,
"Usage: \n"
"  %s <cmd> [options]\n"
"where <cmd> is one of:\n"
"  --list                      List connected blink(1) devices \n"
"  --rgb=<red>,<green>,<blue>  Fade to RGB value\n"
"  --rgb=[#]RRGGBB             Fade to RGB value, as hex color code\n"
"  --hsb=<hue>,<sat>,<bri>     Fade to HSB value\n"
"  --blink <numtimes>          Blink on/off (use --rgb to blink a color)\n"
"  --flash <numtimes>          Flash on/off (same as blink)\n"
"  --on | --white              Turn blink(1) full-on white \n"
"  --off                       Turn blink(1) off \n"
"  --red                       Turn blink(1) red \n"
"  --green                     Turn blink(1) green \n"
"  --blue                      Turn blink(1) blue \n"
"  --cyan                      Turn blink(1) cyan (green + blue) \n"
"  --magenta                   Turn blink(1) magenta (red + blue) \n"
"  --yellow                    Turn blink(1) yellow (red + green) \n"
"  --rgbread                   Read last RGB color sent (post gamma-correction)\n"
"  --setpattline <pos>         Write pattern RGB val at pos (--rgb/hsb to set)\n"
"  --getpattline <pos>         Read pattern RGB value at pos\n" 
"  --savepattern               Save RAM color pattern to flash (mk2)\n"
"  --clearpattern              Erase color pattern completely \n"            
"  --play <1/0,pos>            Start playing color pattern (at pos)\n"
"  --play <1/0,start,end,cnt>  Playing color pattern sub-loop (mk2)\n"
"  --playstate                 Return current status of pattern playing (mk2)\n"
"  --playpattern <patternstr>  Play Blink1Control pattern string in blink1-tool\n"
"  --writepattern <patternstr> Write Blink1Control pattern string to blink(1)\n"
"  --readpattern               Download full blink(1) patt as Blink1Control str\n"
"  --servertickle <1/0>[,1/0]  Turn on/off servertickle (w/on/off, uses -t msec)\n"
"  --chase, --chase=<num,start,stop> Multi-LED chase effect. <num>=0 runs forever\n"
"  --random, --random=<num>    Flash a number of random colors, num=1 if omitted \n"
"  --glimmer, --glimmer=<num>  Glimmer a color with --rgb (num times)\n"
" Nerd functions: \n"
"  --fwversion                 Display blink(1) firmware version \n"
"  --version                   Display blink1-tool version info \n"
#if 0
"  --gobootload                Enable bootloader (mk3 only)\n"
#endif
"and [options] are: \n"
"  -d dNums --id all|deviceIds Use these blink(1) ids (from --list) \n"
"  -g -nogamma                 Disable autogamma correction\n"
"  -m ms,   --millis=millis    Set millisecs for color fading (default 300)\n"
"  -q, --quiet                 Mutes all stdout output (supercedes --verbose)\n"
"  -t ms,   --delay=millis     Set millisecs between events (default 500)\n"
"  -l <led>, --led=<led>       Which LED to use, 0=all/1=top/2=bottom (mk2)\n"
"  --ledn 1,3,5,7              Specify a list of LEDs to light\n"
"  -v, --verbose               verbose debugging msgs\n"
"\n"
"Examples \n"
"  blink1-tool -m 100 --rgb=255,0,255    # Fade to #FF00FF in 0.1 seconds \n"
"  blink1-tool -t 2000 --random=100      # Every 2 seconds new random color\n"
"  blink1-tool --led 2 --random=100      # Random colors on both LEDs \n"
"  blink1-tool --rgb 0xff,0x00,0x00 --blink 3  # blink red 3 times\n"
"  blink1-tool --rgb '#FF9900'           # Make blink1 pumpkin orange\n"
"  blink1-tool --rgb FF9900 --led 2      # Make blink1 orange on lower LED\n"
"  blink1-tool --chase=5,3,18            # Chase pattern 5 times, on leds 3-18\n"
"Pattern Examples \n"
"    # Play purple-green flash 10 times (pattern runs in blink1-tool so blocks)\n" 
"  blink1-tool --playpattern \'10,#ff00ff,0.1,0,#00ff00,0.1,0\'\n"
"    # Change the 2nd color pattern line to #112233 with a 0.5 sec fade\n"
"  blink1-tool -m 500 --rgb 112233 --setpattline 1 \n"
"    # Erase all lines of the color pattern and save to flash \n"
"  blink1-tool --clearpattern ; blink1-tool --savepattern \n"
"Setting Startup Params Examples (mk2 v206+ & mk3 only):\n"
"  blink1-tool --setstartup 1,5,7,10  # enable, play 5-7 loop 10 times\n"
"  blink1-tool --savepattern          # must do this to save to flash \n"
""
#if ENABLE_MK3 == 1
"\n"            
"User notes (mk3 only):\n"
"  blink1-tool --writenote 1 -n 'hello there' \n"
"  blink1-tool --readnote 1 \n"
"\n"
#endif
"\n"
"Notes \n"
" - To blink a color with specific timing, specify 'blink' command last:\n"
"   blink1-tool -t 200 -m 100 --rgb ff00ff --blink 5 \n"
" - If using several blink(1)s, use '-d all' or '-d 0,2' to select 1st,3rd: \n"
"   blink1-tool -d all -t 50 -m 50 -rgb 00ff00 --blink 10 \n"
"\n"
            ,myName);
//"  --hidread                  Read a blink(1) USB HID GetFeature report \n"
//"  --hidwrite <listofbytes>   Write a blink(1) USB HID SetFeature report \n"
}

// local states for the "cmd" option variable
enum { 
    CMD_NONE = 0,
    CMD_LIST,
    CMD_EEREAD,
    CMD_EEWRITE,
    CMD_RGB,
    CMD_HSB,
    CMD_RGBREAD,
    CMD_SETPATTLINE,
    CMD_GETPATTLINE,
    CMD_SAVEPATTERN,
    CMD_OFF,
    CMD_ON,
    CMD_RED,
    CMD_GRN,
    CMD_BLU,
    CMD_CYAN,
    CMD_MAGENTA,
    CMD_YELLOW,
    CMD_BLINK,
    CMD_GLIMMER,
    CMD_PLAY,
    CMD_STOP,
    CMD_GETPLAYSTATE,
    CMD_RANDOM,
    CMD_CHASE,
    CMD_VERSION,
    CMD_FWVERSION,
    CMD_SERVERDOWN,
    CMD_PLAYPATTERN,
    CMD_WRITEPATTERN,
    CMD_CLEARPATTERN,
    CMD_READPATTERN,
    CMD_WRITENOTE,
    CMD_READNOTE,
    CMD_SETSTARTUP,
    CMD_GETSTARTUP,
    CMD_GOBOOTLOAD,
    CMD_SETRGB,
    CMD_TESTTEST
};

//
// Fade to RGB for multiple blink1 devices.
// Uses globals numDevicesToUse, deviceIds, quiet
//
int blink1_fadeToRGBForDevices( uint16_t mils, uint8_t rr,uint8_t gg, uint8_t bb, uint8_t nn ) {
    blink1_device* d;
    int rc;
    for( int i=0; i< numDevicesToUse; i++ ) {
        d = blink1_openById( deviceIds[i] );
        if( d == NULL ) continue;
        msg("set dev:%X:%d to rgb:0x%2.2x,0x%2.2x,0x%2.2x over %d msec\n",
            deviceIds[i], nn, rr,gg,bb, mils, nn);
        if( nn==0 ) {
            rc = blink1_fadeToRGB(d, mils, rr,gg,bb);
        } else {
            rc = blink1_fadeToRGBN(d,mils, rr,gg,bb, nn);
        }
        if( rc == -1 && !quiet ) { // on error, do something, anything. 
            printf("error on fadeToRGBForDevices\n");
        }
        blink1_close( d );
    }
    return 0; // FIXME
}



//
int main(int argc, char** argv)
{
    int nogamma = 0;
    
    int16_t arg = 0;  // generic int arg for cmds that take an arg
    char*  argbuf[150]; // generic str arg for cmds that take an arg
    uint8_t chasebuf[3]; // could use other buf

    uint8_t cmdbuf[blink1_buf_size]; 
    rgb_t rgbbuf = {0,0,0};
    
    int ledn = 0;  // deprecated, soon to be removed
    uint8_t ledns[18];
    uint8_t ledns_cnt=0;
    // FIXME: what was I thinking with this 'ledns'

    int  rc;
    uint8_t tmpbuf[100]; // only used for hsb parsing
    //char serialnumstr[serialstrmax] = {'\0'}; 
    uint8_t reportid = 1; // unused normally, just for testing
    
    srand( time(NULL) * getpid() );
    memset( cmdbuf, 0, sizeof(cmdbuf));

    static int cmd  = CMD_NONE;

    // parse options
    int option_index = 0, opt;
    char* opt_str = "qvhm:t:d:gl:";
    static struct option loptions[] = {
        {"verbose",    optional_argument, 0,      'v'},
        {"quiet",      optional_argument, 0,      'q'},
        {"millis",     required_argument, 0,      'm'},
        {"delay",      required_argument, 0,      't'},
        {"id",         required_argument, 0,      'd'},
        {"led",        required_argument, 0,      'l'},
        {"ledn",       required_argument, 0,      'l'},
        {"nogamma",    no_argument,       0,      'g'},
        {"help",       no_argument,       0,      'h'},
        {"list",       no_argument,       &cmd,   CMD_LIST },
        {"eeread",     required_argument, &cmd,   CMD_EEREAD },
        {"eewrite",    required_argument, &cmd,   CMD_EEWRITE },
        {"rgb",        required_argument, &cmd,   CMD_RGB },
        {"hsb",        required_argument, &cmd,   CMD_HSB },
        {"rgbread",    no_argument,       &cmd,   CMD_RGBREAD},
        {"readrgb",    no_argument,       &cmd,   CMD_RGBREAD},
        {"savepattline",required_argument,&cmd,   CMD_SETPATTLINE },//backcompat
        {"setpattline",required_argument, &cmd,   CMD_SETPATTLINE },
        {"getpattline",required_argument, &cmd,   CMD_GETPATTLINE },
        {"savepattern",no_argument,       &cmd,   CMD_SAVEPATTERN },
        {"off",        no_argument,       &cmd,   CMD_OFF },
        {"on",         no_argument,       &cmd,   CMD_ON },
        {"white",      no_argument,       &cmd,   CMD_ON },
        {"red",        no_argument,       &cmd,   CMD_RED },
        {"green",      no_argument,       &cmd,   CMD_GRN },
        {"blue",       no_argument,       &cmd,   CMD_BLU},
        {"cyan",       no_argument,       &cmd,   CMD_CYAN},
        {"magenta",    no_argument,       &cmd,   CMD_MAGENTA},
        {"yellow",     no_argument,       &cmd,   CMD_YELLOW},
        {"blink",      required_argument, &cmd,   CMD_BLINK},
        {"flash",      required_argument, &cmd,   CMD_BLINK},
        {"glimmer",    optional_argument, &cmd,   CMD_GLIMMER},
        {"play",       required_argument, &cmd,   CMD_PLAY},
        {"stop",       no_argument,       &cmd,   CMD_STOP},
        {"playstate",  no_argument,       &cmd,   CMD_GETPLAYSTATE},
        {"random",     optional_argument, &cmd,   CMD_RANDOM },
        {"chase",      optional_argument, &cmd,   CMD_CHASE },
        {"running",    optional_argument, &cmd,   CMD_CHASE },
        {"version",    no_argument,       &cmd,   CMD_VERSION },
        {"fwversion",  no_argument,       &cmd,   CMD_FWVERSION },
        //{"serialnumread", no_argument,    &cmd,   CMD_SERIALNUMREAD },
        //{"serialnumwrite",required_argument, &cmd,CMD_SERIALNUMWRITE },
        {"servertickle", required_argument, &cmd, CMD_SERVERDOWN },
        {"playpattern",  required_argument, &cmd, CMD_PLAYPATTERN },
        {"writepattern", required_argument, &cmd, CMD_WRITEPATTERN },
        {"readpattern",  no_argument,     &cmd,   CMD_READPATTERN },
        {"clearpattern", no_argument,     &cmd,   CMD_CLEARPATTERN },
        {"setstartup", required_argument, &cmd,   CMD_SETSTARTUP},
        {"getstartup", no_argument,       &cmd,   CMD_GETSTARTUP},
        {"testtest",   no_argument,       &cmd,   CMD_TESTTEST },
        {"reportid",   required_argument, 0,      'i' },
        {"writenote",  required_argument, &cmd,   CMD_WRITENOTE},
        {"readnote",   required_argument, &cmd,   CMD_READNOTE},
        {"notestr",    required_argument, 0,      'n'},
        {"gobootload", no_argument,       &cmd,   CMD_GOBOOTLOAD},
        {"setrgb",     required_argument, &cmd,   CMD_SETRGB },
        {NULL,         0,                 0,      0}
    };
    while(1) {
        opt = getopt_long(argc, argv, opt_str, loptions, &option_index);
        if (opt==-1) break; // parsed all the args
        switch (opt) {
         case 0:             // deal with long opts that have no short opts
            switch(cmd) {
            case CMD_RGB:
            case CMD_SETRGB:
                parsecolor( &rgbbuf, optarg);
                break;
            case CMD_HSB:
                hexread( tmpbuf, optarg, 4);
                hsbtorgb( &rgbbuf, tmpbuf );
                cmd = CMD_RGB; // haha! 
                break;
            case CMD_EEREAD:
            case CMD_EEWRITE:
            case CMD_SETPATTLINE:
            case CMD_GETPATTLINE:
            case CMD_PLAY:
            case CMD_SERVERDOWN:
            case CMD_SETSTARTUP: // FIXME
                hexread(cmdbuf, optarg, sizeof(cmdbuf));  // cmd w/ hexlist arg
                break;
            case CMD_BLINK:
            case CMD_WRITENOTE:
            case CMD_READNOTE:
                arg = (optarg) ? strtol(optarg,NULL,0) : 1;// cmd w/ number arg
                break;
            case CMD_RANDOM:
            case CMD_CHASE:
                if(optarg) hexread(chasebuf, optarg, sizeof(chasebuf));
            case CMD_GLIMMER:
                arg = (optarg) ? strtol(optarg,NULL,0) : 0;// cmd w/ number arg
                break;
            case CMD_PLAYPATTERN:
            case CMD_WRITEPATTERN:
                strncpy( (char*)argbuf, optarg, sizeof(argbuf) );
                break;
            case CMD_ON:
                rgbbuf.r = 255; rgbbuf.g = 255; rgbbuf.b = 255;
                break;
            case CMD_OFF:
                rgbbuf.r = 0; rgbbuf.g = 0; rgbbuf.b = 0;
                break;
            case CMD_RED:
                rgbbuf.r = 255;
                break;
            case CMD_GRN:
                rgbbuf.g = 255; 
                break;
            case CMD_BLU:
                rgbbuf.b = 255; 
                break;
            case CMD_CYAN:
                rgbbuf.g = 255; rgbbuf.b = 255; 
                break;
            case CMD_MAGENTA:
                rgbbuf.r = 255; rgbbuf.b = 255; 
                break;
            case CMD_YELLOW:
                rgbbuf.r = 255; rgbbuf.g = 255; 
                break;
            } // switch(cmd)
            break;
        case 'g':
            nogamma = 1;
            break;
        //case 'a':
        //   openall = 1;
        //   break;
        case 'm':
            millis = strtol(optarg,NULL,10);
            break;
        case 'n':
            strncpy( (char*)argbuf, optarg, sizeof(argbuf) );
            break;
        case 't':
            delayMillis = strtol(optarg,NULL,10);
            break;
        case 'l':
            ledn = strtol(optarg,NULL,10);
            ledns_cnt = hexread(ledns, optarg, sizeof(ledns));
            break;
        case 'q':
            if( optarg==NULL ) quiet++;
            else quiet = strtol(optarg,NULL,0);
            msg_setquiet(quiet);
            break;
        case 'v':
            if( optarg==NULL ) verbose++;
            else verbose = strtol(optarg,NULL,0);
            if( verbose > 3 ) {
                fprintf(stderr,"going REALLY verbose\n");
            }
            break;
        case 'i': // report id, for testing
          reportid = strtol(optarg,NULL,10);
          break;
        case 'd':  // devices to use
            if( strcmp(optarg,"all") == 0 ) {
                numDevicesToUse = 0; //blink1_max_devices;
                for( int i=0; i< blink1_max_devices; i++) {
                    deviceIds[i] = i;
                }
            } 
            else { // if( strcmp(optarg,",") != -1 ) { // comma-separated list
                char* pch;
                //int base = 0;
                pch = strtok( optarg, " ,");
                numDevicesToUse = 0;
                while( pch != NULL ) { 
                    int base = (strlen(pch)==8) ? 16:0;
                    deviceIds[numDevicesToUse++] = strtol(pch,NULL,base);
                    pch = strtok(NULL, " ,");
                }
                //if( !quiet ) {
                //    for( int i=0; i<numDevicesToUse; i++ ) 
                //        printf("using deviceId[%d]: %d\n", i, deviceIds[i]);
                //}
            }
            break;
        case 'h':
            usage( "blink1-tool" );
            exit(1);
            break;
        }
    } // while(1) arg parsing

    if(argc < 2){
        usage( "blink1-tool" );
        exit(1);
    }

    // get a list of all devices and their paths
    int count = blink1_enumerate();

    if( cmd == CMD_VERSION ) { 
        char verbuf[40] = "";
        if( count ) { 
            dev = blink1_openById( deviceIds[0] );
            rc = blink1_getVersion(dev);
            blink1_close(dev);
            snprintf(verbuf, sizeof(verbuf), ", fw version: %d", rc);
        }
        msg("blink1-tool version: %s%s\n",BLINK1_VERSION,verbuf);
        exit(0);
    }

    // rationalize various options to known-good state
    if( delayMillis==-1 ) delayMillis = delayMillisDefault;
    if( millis == -1 ) millis = millisDefault;
    if( ledns_cnt == 0 ) { ledns[0] = 0; ledns_cnt = 1;  }

    if( count == 0  ) {
        msg("no blink(1) devices found\n");
        exit(1);
    }

    if( numDevicesToUse == 0 ) numDevicesToUse = count; 

    if( verbose ) { 
        printf("deviceId[0] = %X\n", deviceIds[0]);
        printf("cached list:\n");
        for( int i=0; i< count; i++ ) { 
            printf("%d: serial: '%s' '%s'\n", 
                   i, blink1_getCachedSerial(i), blink1_getCachedPath(i) );
        }
    }

    // actually open up the device to start talking to it
    if(verbose) printf("openById: %X\n", deviceIds[0]);
    dev = blink1_openById( deviceIds[0] );

    if( dev == NULL ) { 
        msg("cannot open blink(1), bad id or serial number\n");
        exit(1);
    }

    // FIXME: verify mk2 does better gamma correction 
    // (now thinking maybe it doesn't, or is not perfectly visually linear)
#if 0
    if( blink1_isMk2(dev) )  { 
        if( verbose ) printf("blink1(1)mk2 detected. disabling degamma\n");
        blink1_disableDegamma();  
        if( nogamma ) {
            blink1_enableDegamma();  
            if( verbose ) printf("overriding, re-enabling gamma\n");
        }
    }
    else {
        // for original mk1 owners who want to disable degamma
        if( nogamma ) {      //FIXME: confusing
            msg("disabling auto degamma\n");
            blink1_disableDegamma();  
        }
    }
#else
    if( nogamma ) {      //FIXME: confusing
        msg("disabling auto degamma\n");
        blink1_disableDegamma();  
    }
#endif

    // begin command processing

    if( cmd == CMD_LIST ) {
        blink1_close(dev);
        printf("blink(1) list: \n");
        for( int i=0; i< count; i++ ) {
            dev = blink1_openBySerial( blink1_getCachedSerial(i) );
            rc = blink1_getVersion(dev);
            blink1_close(dev);
            printf("id:%d - serialnum:%s %s fw version:%d\n", i, blink1_getCachedSerial(i), 
                   (blink1_isMk2ById(i)) ? "(mk2)":"", rc);
        }
#ifdef USE_HIDDATA
        printf("(Listing not supported in HIDDATA builds)\n"); 
#endif
    }
    else if( cmd == CMD_EEREAD ) {  // FIXME
        msg("eeread:  addr 0x%2.2x = ", cmdbuf[0]);
        uint8_t val = 0;
        rc = blink1_eeread(dev, cmdbuf[0], &val );
        if( rc==-1 ) { // on error
            printf("error\n");
        } else { 
            printf("%2.2x\n", val);
        }
    }
    else if( cmd == CMD_EEWRITE ) {  // FIXME
        msg("eewrite: \n");
        rc = blink1_eewrite(dev, cmdbuf[0], cmdbuf[1] );
        if( rc==-1  && !quiet ) { // error
            printf("error\n");
        }
    }
    else if( cmd == CMD_FWVERSION ) {
        blink1_close(dev);
        for( int i=0; i<count; i++ ) {
            dev = blink1_openById( deviceIds[i] );
            if( dev == NULL ) continue;
            rc = blink1_getVersion(dev);
            printf("id:%d - firmware:%d serialnum:%s %s\n", i, rc,
                   blink1_getCachedSerial(i),
                   (blink1_isMk2ById(i)) ? "(mk2)":"");
            blink1_close(dev);
        }
    }
    else if( cmd == CMD_RGB || cmd == CMD_ON  || cmd == CMD_OFF ||
             cmd == CMD_RED || cmd == CMD_BLU || cmd == CMD_GRN ||
             cmd == CMD_CYAN || cmd == CMD_MAGENTA || cmd == CMD_YELLOW ) { 
        blink1_close(dev); // close global device, open as needed
        
        uint8_t r = rgbbuf.r;
        uint8_t g = rgbbuf.g;
        uint8_t b = rgbbuf.b;

        for( int i=0; i< ledns_cnt; i++ ) {
            blink1_fadeToRGBForDevices( millis, r,g,b, ledns[i] );
        }
    }
    else if( cmd == CMD_SETRGB ) { // for testing the 'n' protocol option
      msg("set rgb to %d,%d,%d\n", rgbbuf.r, rgbbuf.g, rgbbuf.b, ledn );
      blink1_setRGB( dev, rgbbuf.r, rgbbuf.g, rgbbuf.b);
    }
    else if( cmd == CMD_RGBREAD ) { 
        uint8_t r,g,b;
        uint16_t msecs;
        msg("reading led %d rgb: ", ledn );
        rc = blink1_readRGB(dev, &msecs, &r,&g,&b, ledn );
        if( rc==-1 && !quiet ) {
            printf("error on readRGB\n");
        }
        printf("0x%2.2x,0x%2.2x,0x%2.2x\n", r,g,b);
    }
    else if( cmd == CMD_PLAY || cmd == CMD_STOP ) { 
        //if( cmd == CMD_STOP ) play = 0
        uint8_t play     = cmdbuf[0];
        uint8_t startpos = cmdbuf[1];
        uint8_t endpos   = cmdbuf[2]; 
        uint8_t count    = cmdbuf[3];

        msg("%s color pattern from pos %d-%d (%d times)\n", 
                   ((play)?"playing":"stopping"),startpos,endpos,count);

        rc = blink1_playloop(dev, play, startpos,endpos,count);
        if( rc == -1 && !quiet ) { 
            // hmm, do what here
        }
    }
    else if( cmd == CMD_GETPLAYSTATE ) { 
        msg("playstate: ");
        uint8_t playing;
        uint8_t startpos;
        uint8_t endpos;
        uint8_t playcount;
        uint8_t playpos;
        rc = blink1_readPlayState(dev, &playing, &startpos, &endpos, 
                                  &playcount, &playpos);
        printf("playing:%d start-end:%d-%d count:%d pos:%d\n",
               playing, startpos, endpos, playcount, playpos);
    }
    else if( cmd == CMD_SAVEPATTERN ) {
        msg("writing pattern to flash\n");
        rc = blink1_savePattern(dev);
        if( rc==-1 && !quiet ) {
            printf("error on savePattern\n");
        }
    }
    else if( cmd == CMD_SETPATTLINE ) {
        uint8_t r = rgbbuf.r;
        uint8_t g = rgbbuf.g;
        uint8_t b = rgbbuf.b;
        uint8_t p = cmdbuf[0];
        msg("saving rgb: 0x%2.2x,0x%2.2x,0x%2.2x @ %d, ms:%d\n",r,g,b,p,millis);
        if( ledn>=0 ) { // NOTE: only works for fw 204+ devices
            blink1_setLEDN(dev, ledn);  // FIXME: doesn't check return code
        }
        rc = blink1_writePatternLine(dev, millis, r,g,b, p );
        if( rc==-1 && !quiet ) {
            printf("error on writePatternLine\n");
        }
    }
    else if( cmd == CMD_GETPATTLINE ) { 
        uint8_t p = cmdbuf[0];
        uint8_t r,g,b,n;
        uint16_t msecs;
        msg("reading rgb at pos %2d: ", p );
        rc = blink1_readPatternLineN(dev, &msecs, &r,&g,&b, &n, p );
        if( rc==-1 && !quiet ) {
            printf("error on writePatternLine\n");
        }
        printf("r,g,b = 0x%2.2x,0x%2.2x,0x%2.2x (%d) ms:%d\n", r,g,b, n, msecs);
    }
    else if( cmd == CMD_RANDOM ) { 
        int cnt = blink1_getCachedCount();
        if( arg==0 ) arg = 1;
        if( cnt>1 ) blink1_close(dev); // close global device, open as needed
        msg("random %d times: \n", arg);
        for( int i=0; i<arg; i++ ) { 
            uint8_t r = rand()%255;
            uint8_t g = rand()%255;
            uint8_t b = rand()%255 ;
            uint8_t id = rand() % blink1_getCachedCount();

            msg("%d: %d/%d : %2.2x,%2.2x,%2.2x \n", 
                i, id, blink1_getCachedCount(), r,g,b);

            blink1_device* mydev = dev;
            if( cnt > 1 ) mydev = blink1_openById( id );
            if( ledn == 0 ) { 
                rc = blink1_fadeToRGB(mydev, millis,r,g,b);
            } else {
                uint8_t n = 1 + rand() % ledn;
                rc = blink1_fadeToRGBN(mydev, millis,r,g,b,n);
            }
            if( rc == -1 && !quiet ) { // on error, do something, anything.
                printf("error during random\n");
                //break;
            }
            if( cnt > 1 ) blink1_close( mydev );
            
            blink1_sleep(delayMillis);
        }
    }
    // this whole thing is a huge mess currently // FIXME
    else if( cmd == CMD_CHASE) {
        if( ledn == 0 ) ledn = 18;

        int loopcnt       = (chasebuf[0] > 0) ? ((int)(chasebuf[0]))-1 : -1;
        uint8_t led_start = (chasebuf[1]) ? chasebuf[1] : 1;
        uint8_t led_end   = (chasebuf[2]) ? chasebuf[2] : 18;
        int chase_length  = led_end-led_start+1;

        // pick the color
        uint8_t do_rand = 0;
        uint8_t c[3] = { rgbbuf.r, rgbbuf.g, rgbbuf.b };
        if( c[0] == 0 && c[1] == 0 && c[2] == 0 ) { // no rgb specified
            c[0] = rand()%255; c[1] = rand()%255; c[2] = rand()%255; 
            do_rand =1;
        }
        char ledstr[16];
        snprintf(ledstr, sizeof(ledstr), "#%2.2x%2.2x%2.2x",
            rgbbuf.r,rgbbuf.g,rgbbuf.b);
        msg("chase effect %d to %d (with %d leds), color %s, ",
            led_start, led_end, chase_length,
            ((do_rand) ? "random" : ledstr));
        if (loopcnt < 0) msg("forever\n");
        else             msg("%d times\n", loopcnt+1);

        // make gradient
        uint8_t led_grad[chase_length][3];
        for( int i=0; i<chase_length; i++ ) {
            int temp = chase_length-i-1;
            led_grad[temp][0] = c[0] * i / chase_length;
            led_grad[temp][1] = c[1] * i / chase_length;
            led_grad[temp][2] = c[2] * i / chase_length;
        }

        // do the animation
        uint8_t first=1;
        do {
            for( int i=0; i < chase_length; ++i) { // i = front led lit
                for( int j = 0; j<chase_length; ++j) {
                    int grad_index=i-j;
                    if (grad_index < 0) grad_index+=chase_length;
                    uint8_t r = led_grad[grad_index][0];
                    uint8_t g = led_grad[grad_index][1];
                    uint8_t b = led_grad[grad_index][2];
                    if ((j <= i) || (!first)) {
                        rc = blink1_fadeToRGBN(dev, 10 + (millis/chase_length), r,g,b,led_start+j);
                    }
                }
                blink1_sleep(delayMillis/chase_length);
            }
            first = 0;
        } while( loopcnt-- );
    }
    else if( cmd == CMD_BLINK ) { 
        int16_t n = arg; 
        uint8_t r = rgbbuf.r;
        uint8_t g = rgbbuf.g;
        uint8_t b = rgbbuf.b;
        if( r == 0 && b == 0 && g == 0 ) {
            r = g = b = 255;
        }
        blink1_close(dev);
        msg("blink %d times rgb:%x,%x,%x: \n", n,r,g,b);
        if( n == 0 ) n = -1; // repeat forever
        while( n==-1 || n-- ) { 
            rc = blink1_fadeToRGBForDevices( millis,r,g,b,ledn);
            blink1_sleep(delayMillis);
            rc = blink1_fadeToRGBForDevices( millis,0,0,0,ledn);
            blink1_sleep(delayMillis);
         }
    }
    else if( cmd == CMD_GLIMMER ) {
        uint8_t n = arg;
        uint8_t r = rgbbuf.r;
        uint8_t g = rgbbuf.g;
        uint8_t b = rgbbuf.b;
        if( n == 0 ) n = 3;
        if( r == 0 && b == 0 && g == 0 ) {
            r = g = b = 127;
        }
        msg("glimmering %d times rgb:#%2.2x%2.2x%2.2x: \n", n,r,g,b);
        for( int i=0; i<n; i++ ) {
            blink1_fadeToRGBN(dev, millis,r,g,b, 1);
            blink1_fadeToRGBN(dev, millis,r/2,g/2,b/2, 2);
            blink1_sleep(delayMillis/2);
            blink1_fadeToRGBN(dev, millis,r/2,g/2,b/2, 1);
            blink1_fadeToRGBN(dev, millis,r,g,b, 2);
            blink1_sleep(delayMillis/2);
        }
        // turn them both off
        blink1_fadeToRGBN(dev, millis, 0,0,0, 1);
        blink1_fadeToRGBN(dev, millis, 0,0,0, 2);
    }
    else if( cmd == CMD_SERVERDOWN ) { 
        //int on  = arg;
        int on = cmdbuf[0];
        int st = cmdbuf[1];
        int startpos = cmdbuf[2];
        int endpos   = cmdbuf[3];
        msg("setting servertickle %s (@ %ld millis)\n",((on)?"ON":"OFF"),delayMillis);
        blink1_serverdown( dev, on, delayMillis, st, startpos,endpos );
    }
    else if( cmd == CMD_PLAYPATTERN ) {
        blink1_close(dev);
        msg("play pattern: %s\n",argbuf);

        int repeats = -1;
        patternline_t pattern[32];
        int pattlen = parsePattern( (char*)argbuf, &repeats, pattern);
        msg("repeats: %d\n", repeats);
        if( repeats==0 ) repeats=-1;
        
        while( repeats==-1 || repeats-- ) { 
            for( int i=0; i<pattlen; i++ ) {
                patternline_t pat = pattern[i];
                //msg("%d: %2.2x,%2.2x,%2.2x : %d : %d\n", i, pat.color.r,pat.color.r,pat.color.b, pat.millis,pat.ledn );
                msg("%d:",repeats);
                blink1_fadeToRGBForDevices( pat.millis/2, pat.color.r,pat.color.g,pat.color.b, pat.ledn);
                blink1_sleep( pat.millis );
            }
        }
    }
    else if( cmd == CMD_WRITEPATTERN ) {
        msg("write pattern: %s\n", argbuf);

        int repeats = -1;
        patternline_t pattern[32];
        int pattlen = parsePattern( (char*)argbuf, &repeats, pattern);
        //msg("repeats: %d is ignored for writepattern\n", repeats);
        
        for( int i=0; i<pattlen; i++ ) {
            patternline_t pat = pattern[i];
              
            if( pat.ledn>0 ) {
                blink1_setLEDN(dev, pat.ledn);
            }
            msg("writing %d: %2.2x,%2.2x,%2.2x : %d : %d\n", i, pat.color.r,pat.color.r,pat.color.b, pat.millis,pat.ledn );
            rc = blink1_writePatternLine(dev, pat.millis/2, pat.color.r, pat.color.g, pat.color.b, i);
        }

    }
    else if( cmd == CMD_CLEARPATTERN ) {
        msg("clearing pattern...");
        for( int i=0; i<16; i++ ) { // FIXME: pattern length
          rc = blink1_writePatternLine(dev, 0, 0,0,0, i );            
        }
        msg("done\n");
    }
    else if( cmd == CMD_READPATTERN ) {
        msg("read pattern:\n");
        uint8_t r,g,b,n;
        uint16_t msecs;
        char str[1024] = "{0"; // repeats forever
        for( int i=0; i<32; i++ ) {
            rc = blink1_readPatternLineN(dev, &msecs, &r,&g,&b, &n, i );
            //if( !(msecs==0 && r==0 && g==0 && b==0) ) { 
            sprintf(str, "%s,#%2.2x%2.2x%2.2x,%0.2f,%d", str, r,g,b, (msecs/1000.0),n);
            //}
        }
        sprintf(str,"%s}",str);
        msg("%s\n",str);        
    }
    else if( cmd == CMD_SETSTARTUP ) {
      msg("set startup params:");
      uint8_t bootmode  = cmdbuf[0];
      uint8_t playstart = cmdbuf[1];
      uint8_t playend   = cmdbuf[2];
      uint8_t playcount = cmdbuf[3];
      msg(" bootmode: %d, play start/end/count: %d/%d/%d\n",
          bootmode, playstart,playend,playcount);
      rc = blink1_setStartupParams(dev, bootmode, playstart, playend, playcount);
      if( rc == -1 ) {
        msg("error: %d\n",rc);
      }
    }
    else if( cmd == CMD_GETSTARTUP ) {
      msg("get startup params:");
      uint8_t bootmode;
      uint8_t playstart;
      uint8_t playend;
      uint8_t playcount;
      rc = blink1_getStartupParams(dev, &bootmode, &playstart, &playend, &playcount);
      msg(" bootmode: %d, play start/end/count: %d/%d/%d\n",
          bootmode, playstart,playend,playcount);
    }
    else if( cmd == CMD_WRITENOTE ) {
      uint8_t noteid = arg;
      uint8_t* notebuf = (uint8_t*)argbuf; 
      blink1_writeNote( dev, noteid, notebuf);
    }
    else if( cmd == CMD_READNOTE ) {
      uint8_t noteid = arg;
      uint8_t notebuf[blink1_note_size];
      uint8_t* notebufp = notebuf; // why do I need to do this?
    
      blink1_readNote( dev, noteid, &notebufp);

      printf("note %d: %s\n", noteid, notebuf);
    }
    else if( cmd == CMD_GOBOOTLOAD ) {
      msg("Changing blink(1) mk3 to bootloader...\n");
      msg("Use dfu-util to upload new firmare\n");
      msg("Or replug device to go back to normal\n");
      rc = blink1_goBootloader(dev);
      if( rc == 0 ) {
        msg("blink(1) in bootloader mode\n");
      } else {
        msg("error triggering bootloader\n");
      }
    }
    else if( cmd == CMD_TESTTEST ) {
      msg("test test reportid:%d\n",reportid);
      rc = blink1_testtest(dev, reportid);
    }


    return 0;
}

