/**
 * blink(1) C library -- aka "blink1-lib"
 *
 * Part of the blink(1) open source hardware project
 * See https://github.com/todbot/blink1 for details
 *
 * 2012-2022, Tod Kurt, http://todbot.com/blog/ , http://thingm.com/
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>  // for toupper()
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#define   swprintf   _snwprintf
#else
//#include <unistd.h>    // for usleep()
#endif

#include "blink1-lib.h"

int msg_quiet = 0;

// blink1 copy of some hid_device_info and other bits.
// this seems kinda dumb, though. is there a better way?
typedef struct blink1_info_ {
    blink1_device* dev;  // device, if opened, NULL otherwise
    char path[pathstrmax];  // platform-specific device path
    char serial[serialstrmax];
    int type;  // from blink1types
} blink1_info;

static blink1_info blink1_infos[cache_max];
static int blink1_cached_count = 0;  // number of cached entities

static int blink1_enable_degamma = 1;

int blink1_lib_verbose = 0;

// set in Makefile to debug HIDAPI stuff
#ifdef DEBUG_PRINTF
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(...) { if(blink1_lib_verbose) { fprintf(stderr, __VA_ARGS__); } }
#endif

// addresses in EEPROM for mk1 blink(1) devices
#define blink1_eeaddr_osccal        0
#define blink1_eeaddr_bootmode      1
#define blink1_eeaddr_serialnum     2
#define blink1_serialnum_len        4
#define blink1_eeaddr_patternstart (blink1_eeaddr_serialnum + blink1_serialnum_len)

void blink1_sortCache(void);

const char * const deviceTypeStrings[] =
    {
     "unknown",
     "mk1", "mk2", "mk3"
    };

//----------------------------------------------------------------------------
// implementation-varying code

#if USE_HIDDATA
#include "blink1-lib-lowlevel-hiddata.h"
#else
//#if USE_HIDAPI
#include "blink1-lib-lowlevel-hidapi.h"
#endif
// default to USE_HIDAPI unless specifically told otherwise


// -------------------------------------------------------------------------
// everything below here doesn't need to know about USB details
// except for a "blink1_device*"
// -------------------------------------------------------------------------

//
// blink1 hardware api
//

//
int blink1_getCachedCount(void)
{
    return blink1_cached_count;
}

//
const char* blink1_getCachedPath(int i)
{
    if( i > blink1_getCachedCount()-1 ) return NULL;
    return blink1_infos[i].path;
}
//
const char* blink1_getCachedSerial(int i)
{
    if( i > blink1_getCachedCount()-1 ) return NULL;
    return blink1_infos[i].serial;
}

int blink1_getCacheIndexByPath( const char* path )
{
    for( int i=0; i< cache_max; i++ ) {
        if( strcmp( blink1_infos[i].path, (const char*) path ) == 0 ) return i;
    }
    return -1;
}

int blink1_getCacheIndexById( uint32_t i )
{
    if( i > blink1_max_devices ) { // then i is a serial number not an array index
        char serialstr[serialstrmax];
        snprintf(serialstr, sizeof(serialstr), "%X", i); // convert to wchar_t*
        return blink1_getCacheIndexBySerial( serialstr );
    }
    return i;
}

int blink1_getCacheIndexBySerial( const char* serial )
{
    for( int i=0; i< cache_max; i++ ) {
        if( strcasecmp( blink1_infos[i].serial, serial ) == 0 ) return i;
    }
    return -1;
}

int blink1_getCacheIndexByDev( blink1_device* dev )
{
    for( int i=0; i< cache_max; i++ ) {
        if( blink1_infos[i].dev == dev ) return i;
    }
    return -1;
}

const char* blink1_getSerialForDev(blink1_device* dev)
{
    int i = blink1_getCacheIndexByDev( dev );
    if( i>=0 ) return blink1_infos[i].serial;
    return NULL;
}

int blink1_clearCacheDev( blink1_device* dev )
{
    int i = blink1_getCacheIndexByDev( dev );
    if( i>=0 ) blink1_infos[i].dev = NULL; // FIXME: hmmmm
    return i;
}

blink1Type_t blink1_deviceTypeById( int i )
{
    //if( i>=0  && blink1_infos[i].type == BLINK1_MK2 ) return 1;
    //if( i>= 0 ) {
    return blink1_infos[i].type;
    //}
    //return 0;
}

// returns BLINK1_MK2, BLINK1_MK3, or BLINK1
blink1Type_t blink1_deviceType( blink1_device* dev )
{
  return blink1_deviceTypeById( blink1_getCacheIndexByDev(dev) );
}

const char* blink1_deviceTypeToStr(blink1Type_t t)
{
    return deviceTypeStrings[t];
}


int blink1_isMk2ById( int i )
{
    if( i>=0  && blink1_infos[i].type == BLINK1_MK2 ) return 1;
    return 0;
}

int blink1_isMk2( blink1_device* dev )
{
    return blink1_isMk2ById( blink1_getCacheIndexByDev(dev) );
}


//
int blink1_getVersion(blink1_device *dev)
{
    char buf[blink1_buf_size] = {blink1_report_id, 'v' };
    int len = sizeof(buf);

    int rc = blink1_read(dev, buf, len);
    if( rc != -1 ) // also no error
        rc = ((buf[3]-'0') * 100) + (buf[4]-'0');
    // rc is now version number or error
    // FIXME: we don't know vals of errcodes
    return rc;
}

// mk1 only, not supported on mk2
int blink1_eeread(blink1_device *dev, uint16_t addr, uint8_t* val)
{
    char buf[blink1_buf_size] = {blink1_report_id, 'e', addr };
    int len = sizeof(buf);

    int rc = blink1_write(dev, buf, len );
    blink1_sleep( 50 ); // FIXME:
    if( rc != -1 ) // no error
        rc = blink1_read(dev, buf, len );
    if( rc != -1 )
        *val = buf[3];
    return rc;
}

// mk1 only, not supported on mk2
int blink1_eewrite(blink1_device *dev, uint16_t addr, uint8_t val)
{
    char buf[blink1_buf_size] = {blink1_report_id, 'E', addr, val };

    int rc = blink1_write(dev, buf, sizeof(buf) );

    return rc;
}

// FIXME: this doesn't work, use hidapi info instead
int blink1_serialnumread(blink1_device *dev, uint8_t** serialnum)
{
    int rc = -1;
    for( int i=0; i<blink1_serialnum_len; i++ ) { // serial num is 8 chars long
        //blink1_eeread( dev, blink1_eeaddr_serialnum+i, (serialnum+i) );
    }
    return rc;
}

//
static uint8_t parseHex(char c)
{
    c = toupper(c);
    if (c >= '0' && c <= '9')  return (c - '0');
    if (c >= 'A' && c <= 'F')  return (c - 'A')+10;
    return 0;
}

// serialnum comes in as an ascii set of 8 characters representing
// 4-bytes
int blink1_serialnumwrite(blink1_device *dev, uint8_t* serialnumstr)
{
    uint8_t serialnum[4];
    serialnum[0] = parseHex( serialnumstr[0] )*16 + parseHex( serialnumstr[1] );
    serialnum[1] = parseHex( serialnumstr[2] )*16 + parseHex( serialnumstr[3] );
    serialnum[2] = parseHex( serialnumstr[4] )*16 + parseHex( serialnumstr[5] );
    serialnum[3] = parseHex( serialnumstr[6] )*16 + parseHex( serialnumstr[7] );

    int rc = 0;
    for( int i=0; i<blink1_serialnum_len; i++ ) { // serialnum is 4 chars long
        blink1_sleep(50); //FIXME:
        uint8_t v = serialnum[i];
        int rc = blink1_eewrite( dev, blink1_eeaddr_serialnum+i, v);
        if( rc == -1 ) { // try again
            LOG("blink1_serialwrite: oops, trying again on char %d\n",i);
            rc = blink1_eewrite(dev,blink1_eeaddr_serialnum+i, v);
            if( rc == -1 ) {
                LOG("blink1_serialwrite: error on try again\n");
                break;
            }
        }

    }
    return rc;
}

int blink1_fadeToRGBN(blink1_device *dev,  uint16_t fadeMillis,
                      uint8_t r, uint8_t g, uint8_t b, uint8_t n)
{
    int dms = fadeMillis/10;  // millis_divided_by_10

    char buf[blink1_buf_size];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'c';   // command code for 'fade to rgb'
    buf[2] = ((blink1_enable_degamma) ? blink1_degamma(r) : r );
    buf[3] = ((blink1_enable_degamma) ? blink1_degamma(g) : g );
    buf[4] = ((blink1_enable_degamma) ? blink1_degamma(b) : b );
    buf[5] = (dms >> 8);
    buf[6] = dms % 0xff;
    buf[7] = n;

    int rc = blink1_write(dev, buf, sizeof(buf) );

    return rc;
}


//
int blink1_fadeToRGB(blink1_device *dev,  uint16_t fadeMillis,
                     uint8_t r, uint8_t g, uint8_t b)
{
    int dms = fadeMillis/10;  // millis_divided_by_10

    uint8_t buf[9];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'c';   // command code for 'fade to rgb'
    buf[2] = ((blink1_enable_degamma) ? blink1_degamma(r) : r );
    buf[3] = ((blink1_enable_degamma) ? blink1_degamma(g) : g );
    buf[4] = ((blink1_enable_degamma) ? blink1_degamma(b) : b );
    buf[5] = (dms >> 8);
    buf[6] = dms % 0xff;
    buf[7] = 0;

    int rc = blink1_write(dev, buf, sizeof(buf) );

    return rc;
}

//
int blink1_setRGB(blink1_device *dev, uint8_t r, uint8_t g, uint8_t b )
{
    uint8_t buf[blink1_buf_size];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'n';   // command code for "set rgb now"
    buf[2] = ((blink1_enable_degamma) ? blink1_degamma(r) : r );     // red
    buf[3] = ((blink1_enable_degamma) ? blink1_degamma(g) : g );     // grn
    buf[4] = ((blink1_enable_degamma) ? blink1_degamma(b) : b );     // blu
    buf[5] = 0;
    buf[6] = 0;
    buf[7] = 0;

    int rc = blink1_write(dev, buf, sizeof(buf) );

    return rc;
}

// mk2 devices only
int blink1_readRGB(blink1_device *dev, uint16_t* fadeMillis,
                   uint8_t* r, uint8_t* g, uint8_t* b,
                   uint8_t ledn)
{
    if( ! blink1_isMk2(dev) ) {
        return blink1_readRGB_mk1( dev, fadeMillis, r,g,b);
    }
    uint8_t buf[blink1_buf_size] = { blink1_report_id, 'r', 0,0,0, 0,0,ledn };

    int rc = blink1_read(dev, buf, sizeof(buf) );
    if( rc != -1 ) {
        *r = buf[2];
        *g = buf[3];
        *b = buf[4];
        *fadeMillis = ((buf[5]<<8) + (buf[6] &0xff)) * 10;
    }
    return rc;
}


//
// args:
// - on == 1 or 0, enable or disable
// - millis == milliseconds to wait until triggering
// - st == 1 or 0, stay lit or set off()  (mk2 firmware only)
// FIXME: bug in firmware? yes in v204 devices, max time is 62 secs
// - millis = 100000 => dms = 10000, dms_hi =  39, dms_lo =  16 :: real time = 34 secs
// - millis =  50000 => dms =  5000, dms_hi =  19, dms_lo = 136 :: real time = 50 secs
// - millis = 652800 => dms = 65280, dms_hi = 255, dms_lo =   0 :: real time = 62 secs
// - millis =  62000 => dms =  6200, dms_hi =  24, dms_lo =  56 :: real time = 62 secs
//
int blink1_serverdown(blink1_device *dev, uint8_t on, uint32_t millis, uint8_t st,
                      uint8_t startpos, uint8_t endpos)
{
    uint16_t dms = millis / 10;  // millis_divided_by_10

    // printf("serverdown: millis: %u, dms: %d = %d / %d\n", millis, dms, (dms>>8), (dms % 0xff) );
    uint8_t buf[blink1_buf_size];
    buf[0] = blink1_report_id;
    buf[1] = 'D';
    buf[2] = on;
    buf[3] = (dms>>8);
    buf[4] = (dms % 0xff);
    buf[5] = st;  // mk2 only
    buf[6] = startpos;
    buf[7] = endpos;

    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

//
int blink1_play(blink1_device *dev, uint8_t play, uint8_t startpos)
{
    return blink1_playloop(dev, play, startpos, 0,0);
}

// mk2 devices only
int blink1_playloop(blink1_device *dev, uint8_t play, uint8_t startpos,uint8_t endpos, uint8_t count)
{
    uint8_t buf[blink1_buf_size];
    buf[0] = blink1_report_id;
    buf[1] = 'p';
    buf[2] = play;
    buf[3] = startpos;
    buf[4] = endpos;
    buf[5] = count;
    buf[6] = 0;
    buf[7] = 0;

    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

// mk2 devices only
int blink1_readPlayState(blink1_device *dev, uint8_t* playing,
                         uint8_t* playstart, uint8_t* playend,
                         uint8_t* playcount, uint8_t* playpos)
{
    uint8_t buf[blink1_buf_size] = { blink1_report_id, 'S', 0,0,0, 0,0,0 };

    int rc = blink1_read(dev, buf, sizeof(buf) );
    if( rc != -1 ) {
        *playing   = buf[2];
        *playstart = buf[3];
        *playend   = buf[4];
        *playcount = buf[5];
        *playpos   = buf[6];
    }
    return rc;
}


//
int blink1_writePatternLine(blink1_device *dev, uint16_t fadeMillis,
                            uint8_t r, uint8_t g, uint8_t b,
                            uint8_t pos)
{
    int dms = fadeMillis/10;  // millis_divided_by_10
    r = (blink1_enable_degamma) ? blink1_degamma(r) : r ;
    g = (blink1_enable_degamma) ? blink1_degamma(g) : g ;
    b = (blink1_enable_degamma) ? blink1_degamma(b) : b ;

    uint8_t buf[blink1_buf_size] =
        {blink1_report_id, 'P', r,g,b, (dms>>8), (dms % 0xff), pos };
    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

//
int blink1_readPatternLine(blink1_device *dev, uint16_t* fadeMillis,
                           uint8_t* r, uint8_t* g, uint8_t* b,
                           uint8_t pos)
{
    uint8_t ledn;
    return blink1_readPatternLineN( dev, fadeMillis, r,g,b, &ledn, pos);
}

int blink1_readPatternLineN(blink1_device *dev, uint16_t* fadeMillis,
                            uint8_t* r, uint8_t* g, uint8_t* b, uint8_t* ledn,
                            uint8_t pos)
{
    uint8_t buf[blink1_buf_size] = { blink1_report_id, 'R', 0,0,0, 0,0, pos };

    int rc = blink1_read(dev, buf, sizeof(buf) );
    if( rc != -1 ) {
        *r = buf[2];
        *g = buf[3];
        *b = buf[4];
        *fadeMillis = ((buf[5]<<8) + (buf[6] &0xff)) * 10;
        *ledn = buf[7];
    }
    return rc;
}

// mk2 devices only, mk1 devices save on each writePatternLine()
int blink1_savePattern( blink1_device *dev )
{
    uint8_t buf[blink1_buf_size];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'W';   // command code for "write pattern to flash"
    buf[2] = 0xBE;
    buf[3] = 0xEF;
    buf[4] = 0xCA;
    buf[5] = 0xFE;
    buf[6] = 0x00;
    buf[7] = 0x00;
    int rc = blink1_write(dev, buf, sizeof(buf) );
    rc = 0; // assume success
    // note rc will always return -1
    // because of issue with flash programming timing out USB
    return rc;
}

// only for devcies with fw val 204+
int blink1_setLEDN( blink1_device* dev, uint8_t ledn)
{
    uint8_t buf[blink1_buf_size];

    buf[0] = blink1_report_id;     // report id
    buf[1] = 'l';   // command code for "set ledn"
    buf[2] = ledn;
    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

// only for devices with fw val 206+ or mk3
int blink1_getStartupParams( blink1_device* dev, uint8_t* bootmode,
                             uint8_t* playstart, uint8_t* playend, uint8_t* playcount)
{
  uint8_t buf[blink1_buf_size] = { blink1_report_id, 'b', 0,0,0, 0,0, 0 };

  int rc = blink1_read(dev, buf, sizeof(buf) );
  if( rc != -1 ) {
    *bootmode  = buf[2];
    *playstart = buf[3];
    *playend   = buf[4];
    *playcount = buf[5];
  }
  return rc;
}

// only for devices with fw val 206+ or mk3
// FIXME: make 'params' a struct
int blink1_setStartupParams( blink1_device* dev, uint8_t bootmode,
                             uint8_t playstart, uint8_t playend, uint8_t playcount)
{
    uint8_t buf[blink1_buf_size];
    buf[0] =  blink1_report_id;
    buf[1] = 'B';
    buf[2] = bootmode;   // (0=normal, 1=play, 2=off)
    buf[3] = playstart;
    buf[4] = playend;
    buf[5] = playcount;
    buf[6] = 0;          // unused1
    buf[7] = 0;          // unused2
    int rc = blink1_write(dev, buf, sizeof(buf) );
    return rc;
}

// only for mk3
int blink1_writeNote( blink1_device* dev, uint8_t noteid, const uint8_t* notebuf)
{
  uint8_t buf[blink1_buf2_size] = { blink1_report2_id, 'F', noteid };
  memcpy( buf+3, notebuf, blink1_note_size ); // FIXME: notes are 100 bytes
  //hexdump(stdout, (char*)notebuf, 15);
  //hexdump(stdout, buf, 15);
  int rc = blink1_write(dev, buf, sizeof(buf));
  if( rc == -1 ) {
    printf("blink1_writeNote: oops error\n");
  }
  return rc;
}

// only for mk3
int blink1_readNote( blink1_device* dev, uint8_t noteid,  uint8_t** notebuf)
{
    uint8_t buf[blink1_buf2_size] = { blink1_report2_id, 'f', noteid  };

    int rc = blink1_read(dev, buf, sizeof(buf) );
    //int rc = blink1_write(dev, buf, sizeof(buf)-1 ); // why does this need to be one shorter?
    //rc = blink1_read_nosend(dev, buf, sizeof(buf) );

    uint8_t* notedata = buf+3;
    memcpy( *notebuf, notedata, blink1_note_size); // skip over report id, cmd FIXME: harccoded 100

    return rc;
}

// only for mk3
int blink1_bootloaderGo( blink1_device* dev )
{
  uint8_t buf[blink1_buf2_size] = { blink1_report2_id, 'G', 'o','B','o','o','t',0 };
  int rc = blink1_read(dev, buf, sizeof(buf));
  if( rc == -1 ) {
    printf("blink1_bootloaderLock: oops error\n");
  }
  else {
    printf("blink1 response: '%s'\n", buf);
    if( strncmp("GOBOOT", (char*)buf+1, 6) != 0 ) {
      printf("blink1_bootloaderLock: bootloader is locked\n");
      rc = 01; // fail
    }
  }
  return rc;  // >0 on good completion
}

// only for mk3
int blink1_bootloaderLock( blink1_device* dev )
{
  uint8_t buf[blink1_buf2_size] = { blink1_report2_id, 'L', 'o','c','k','B','o','o','t','l','o','a','d' };
  int rc = blink1_read(dev, buf, sizeof(buf));
  if( rc == -1 ) {
    printf("blink1_bootlaoderLock: oops error\n");
  }
  else {
    printf("blink1 response: %s\n", buf+1);
    if( strncmp("LOCKED", (char*)buf+1, 6) != 0 ) {
      rc = -1;
    }
  }
  return rc;
}

// only for mk3
// idbuf must be at least blink1_report2_size-2 big
//
int blink1_getId( blink1_device *dev, uint8_t** idbuf )
{
    uint8_t buf[blink1_report2_size] = { blink1_report2_id, 'U', 0,0,0, 0,0,0 };
    int rc = blink1_read(dev, buf, sizeof(buf));
    if( rc != -1 ) {
      memcpy( idbuf, buf+2, blink1_report2_size-2); // skip over report id & cmd
    }
    else {
      printf("getId: read error: rc=%d\n", rc);
    }
    return rc;
}

//
int blink1_testtest( blink1_device *dev, uint8_t reportid )
{
    uint8_t count = (reportid==1) ? blink1_buf_size : blink1_buf2_size;
    uint8_t buf[blink1_report2_size] = { reportid, '!', 0,0,0, 0,0,0 };

    int rc = blink1_write(dev, buf, count );
    blink1_sleep( 50 ); //FIXME:
    if( rc != -1 ) { // no error
        rc = blink1_read(dev, buf, count);
        for( int i=0; i<count; i++ ) {
            printf("%2.2x,",(uint8_t)buf[i]);
        }
        printf("\n");
    }
    else {
        printf("testtest error: rc=%d\n", rc);
    }
    return rc;
}



/* ------------------------------------------------------------------------- */

void blink1_enableDegamma()
{
    blink1_enable_degamma = 1;
}
void blink1_disableDegamma()
{
    blink1_enable_degamma = 0;
}

/**
 * Using a brightness value, update an r,g,b triplet
 * Modifies r,g,b in place
 */
void blink1_adjustBrightness( uint8_t brightness, uint8_t* r, uint8_t* g, uint8_t* b)
{
    if( brightness ) {
        *r = (*r * brightness) >> 8;
        *g = (*g * brightness) >> 8;
        *b = (*b * brightness) >> 8;
    }
}

#if 0
// a simple logarithmic -> linear mapping as a sort of gamma correction
// maps from 0-255 to 0-255
static int blink1_degamma_log2lin( int n )
{
  //return  (int)(1.0* (n * 0.707 ));  // 1/sqrt(2)
    return (((1<<(n/32))-1) + ((1<<(n/32))*((n%32)+1)+15)/32);
}
#endif

// from http://rgb-123.com/ws2812-color-output/
//   GammaE=255*(res/255).^(1/.45)
//
uint8_t GammaE[] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5,
6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 11, 11,
11, 12, 12, 13, 13, 13, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18,
19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 27, 27, 28,
29, 29, 30, 31, 31, 32, 33, 34, 34, 35, 36, 37, 37, 38, 39, 40,
40, 41, 42, 43, 44, 45, 46, 46, 47, 48, 49, 50, 51, 52, 53, 54,
55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
71, 72, 73, 74, 76, 77, 78, 79, 80, 81, 83, 84, 85, 86, 88, 89,
90, 91, 93, 94, 95, 96, 98, 99,100,102,103,104,106,107,109,110,
111,113,114,116,117,119,120,121,123,124,126,128,129,131,132,134,
135,137,138,140,142,143,145,146,148,150,151,153,155,157,158,160,
162,163,165,167,169,170,172,174,176,178,179,181,183,185,187,189,
191,193,194,196,198,200,202,204,206,208,210,212,214,216,218,220,
222,224,227,229,231,233,235,237,239,241,244,246,248,250,252,255};
//
static int blink1_degamma_better( int n )
{
    return GammaE[n];
}

//
int blink1_degamma( int n )
{
    //return blink1_degamma_log2lin(n);
    return blink1_degamma_better(n);
}

// qsort char* string comparison function
int cmp_blink1_info_serial(const void *a, const void *b)
{
    blink1_info* bia = (blink1_info*) a;
    blink1_info* bib = (blink1_info*) b;

    return strncmp( bia->serial,
                    bib->serial,
                    serialstrmax);
}

void blink1_sortCache(void)
{
    size_t elemsize = sizeof( blink1_info ); //

    qsort( blink1_infos,
           blink1_cached_count,
           elemsize,
           cmp_blink1_info_serial);
}


//
int blink1_vid(void)
{
    //uint8_t  rawVid[2] = {USB_CFG_VENDOR_ID};
    //int vid = rawVid[0] + 256 * rawVid[1];
    return BLINK1_VENDOR_ID;
}
//
int blink1_pid(void)
{
    //uint8_t  rawPid[2] = {USB_CFG_DEVICE_ID};
    //int pid = rawPid[0] + 256 * rawPid[1];
    return BLINK1_DEVICE_ID;
}

// simple cross-platform millis sleep func
void blink1_sleep(uint32_t millis)
{
#ifdef WIN32
            Sleep(millis);
#else
            usleep( millis * 1000);
#endif
}


//
// blink1 utility api
//

// take an array of bytes and spit them out as a hex string
void hexdump(FILE* fp, uint8_t *buffer, int len)
{
    int     i;
    //FILE    *fp = stdout;

    for(i = 0; i < len; i++){
        if(i != 0){
            if(i % 16 == 0){
                fprintf(fp, "\n");
            }else{
                fprintf(fp, " ");
            }
        }
        fprintf(fp, "0x%02x", buffer[i] & 0xff);
    }
    if(i != 0)
        fprintf(fp, "\n");
}

// parse a comma-delimited string containing numbers (dec,hex) into a byte arr
int hexread(uint8_t *buffer, char *string, int buflen)
{
    char    *s;
    int     pos = 0;
    if( string==NULL ) return -1;
    memset(buffer,0,buflen);  // bzero() not defined on Win32?
    while((s = strtok(string, ", ")) != NULL && pos < buflen){
        string = NULL;
        buffer[pos++] = (char)strtol(s, NULL, 0);
    }
    return pos;
}

// integer-only hsbtorgb
// from: http://web.mit.edu/storborg/Public/hsvtorgb.c
void hsbtorgb( rgb_t* rgb, uint8_t* hsb )
{
    uint8_t h = hsb[0];
    uint8_t s = hsb[1];
    uint8_t v = hsb[2];

    unsigned char region, fpart, p, q, t;
    uint8_t r,g,b;

    if(s == 0) {          // color is grayscale
        r = g = b = v;
        return;
    }

    region = h / 43;      // make hue 0-5
    fpart = (h - (region * 43)) * 6; // find remainder part, make it from 0-255

    // calculate temp vars, doing integer multiplication
    p = (v * (255 - s)) >> 8;
    q = (v * (255 - ((s * fpart) >> 8))) >> 8;
    t = (v * (255 - ((s * (255 - fpart)) >> 8))) >> 8;

    // assign temp vars based on color cone region
    switch(region) {
        case 0:   r = v; g = t; b = p; break;
        case 1:   r = q; g = v; b = p; break;
        case 2:   r = p; g = v; b = t; break;
        case 3:   r = p; g = q; b = v; break;
        case 4:   r = t; g = p; b = v; break;
        default:  r = v; g = p; b = q; break;
    }
    rgb->r=r;
    rgb->g=g;
    rgb->b=b;
}

void remove_whitespace(char *str)
{
    char *p;
    size_t len = strlen(str);

    for(p = str; *p; p ++, len --) {
        while(isspace(*p)) memmove(p, p+1, len--);
    }
}

// parse a color in form either "#ff00ff" or "FF00FF"
// or "255,0,255" or "0xff,0x00,0xff"
void parsecolor(rgb_t* color, char* colorstr)
{
    // parse hex color code like "#FF00FF" or "FF00FF"
    if( strchr(colorstr,',')==NULL && (colorstr[0] == '#' || strlen(colorstr)==6) ) {
        colorstr = (colorstr[0] == '#') ? colorstr+1 : colorstr;
        uint32_t colorint = strtol(colorstr, NULL, 16);
        color->r = (colorint >> 16) & 0xff;
        color->g = (colorint >>  8) & 0xff;
        color->b = (colorint >>  0) & 0xff;
    } else { // else it's a list like "0xff,0x00,0xff" or "255,0,255"
        hexread((uint8_t*)color, colorstr, 3);  // FIXME: hardcoded size
    }
}

//
// Parse pattern into an array of patternlines
// - number repeats
// - pattern array (contains {color,millis,ledn}
// - pattern length
int parsePattern( char* str, int* repeats, patternline_t* pattern )
{
    remove_whitespace(str);
    char* s;
    s = strtok( str, ", ");
    if(  s != NULL ) {
      *repeats = strtol(s,NULL,0);
    }

    int i=0;
    s = strtok(NULL, ","); // prep next parse
    while( s != NULL ) {
        //printf("s:'%s'\n",s);
        parsecolor( &pattern[i].color, s );

        s = strtok(NULL, ",");
        if( s == NULL ) { msg("bad pattern: no millis\n"); break; }
        pattern[i].millis = atof(s) * 1000;

        s = strtok(NULL, ",");
        if( s == NULL ) { msg("bad pattern: no led\n"); break; }
        pattern[i].ledn = strtol(s,NULL,0);

        i++;

        s = strtok(NULL, ",");
        if( s == NULL ) break;
    }
    int pattlen = i;
    return pattlen;
}

/**
 * printf that can be shut up
 */
void msg(char* fmt, ...)
{
    va_list args;
    va_start(args,fmt);
    if( !msg_quiet ) {
        vprintf(fmt,args);
    }
    va_end(args);
}

/**
 * set the level of quiet for msg  FIXME:
 */
void msg_setquiet(int q)
{
    msg_quiet = q;
}
