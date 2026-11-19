/*
 * tests/test-blink1-lib.c -- unit tests for pure functions in blink1-lib
 * No blink(1) device required.
 *
 * Build & run via: make test-blink1-lib
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../blink1-lib.h"
#include "../blink1-lib-patterns.h"

// ---------------------------------------------------------------------------
// Minimal test harness
// ---------------------------------------------------------------------------

static int tests_run    = 0;
static int tests_failed = 0;

#define CHECK(label, cond) do { \
    tests_run++; \
    if (cond) { \
        printf("PASS  %s\n", label); \
    } else { \
        printf("FAIL  %s  (line %d)\n", label, __LINE__); \
        tests_failed++; \
    } \
} while(0)

// ---------------------------------------------------------------------------
// parsecolor
// ---------------------------------------------------------------------------

static void test_parsecolor(void)
{
    rgb_t c;

    char s_hash[] = "#ff00ff";
    parsecolor(&c, s_hash);
    CHECK("parsecolor #ff00ff r=255", c.r == 255);
    CHECK("parsecolor #ff00ff g=0",   c.g == 0);
    CHECK("parsecolor #ff00ff b=255", c.b == 255);

    char s_nohash[] = "FF00FF";
    parsecolor(&c, s_nohash);
    CHECK("parsecolor FF00FF r=255",  c.r == 255);
    CHECK("parsecolor FF00FF g=0",    c.g == 0);
    CHECK("parsecolor FF00FF b=255",  c.b == 255);

    char s_0x[] = "0xFF00FF";
    parsecolor(&c, s_0x);
    CHECK("parsecolor 0xFF00FF r=255", c.r == 255);
    CHECK("parsecolor 0xFF00FF g=0",   c.g == 0);
    CHECK("parsecolor 0xFF00FF b=255", c.b == 255);

    char s_dec[] = "255,0,255";
    parsecolor(&c, s_dec);
    CHECK("parsecolor 255,0,255 r=255", c.r == 255);
    CHECK("parsecolor 255,0,255 g=0",   c.g == 0);
    CHECK("parsecolor 255,0,255 b=255", c.b == 255);

    char s_hexlist[] = "0xff,0x00,0xff";
    parsecolor(&c, s_hexlist);
    CHECK("parsecolor 0xff,0x00,0xff r=255", c.r == 255);
    CHECK("parsecolor 0xff,0x00,0xff g=0",   c.g == 0);
    CHECK("parsecolor 0xff,0x00,0xff b=255", c.b == 255);

    // mixed-case hex
    char s_mixed[] = "#FF00ff";
    parsecolor(&c, s_mixed);
    CHECK("parsecolor #FF00ff r=255", c.r == 255);
    CHECK("parsecolor #FF00ff g=0",   c.g == 0);
    CHECK("parsecolor #FF00ff b=255", c.b == 255);

    char s_mixed2[] = "fF00fF";
    parsecolor(&c, s_mixed2);
    CHECK("parsecolor fF00fF r=255",  c.r == 255);
    CHECK("parsecolor fF00fF g=0",    c.g == 0);
    CHECK("parsecolor fF00fF b=255",  c.b == 255);

    char s_black[] = "#000000";
    parsecolor(&c, s_black);
    CHECK("parsecolor #000000 r=0", c.r == 0);
    CHECK("parsecolor #000000 g=0", c.g == 0);
    CHECK("parsecolor #000000 b=0", c.b == 0);

    char s_white[] = "#ffffff";
    parsecolor(&c, s_white);
    CHECK("parsecolor #ffffff r=255", c.r == 255);
    CHECK("parsecolor #ffffff g=255", c.g == 255);
    CHECK("parsecolor #ffffff b=255", c.b == 255);
}

// distinct channel values so r/g/b swaps get caught
#define CHECK_RGB(label, c, rr,gg,bb) \
    CHECK(label, (c).r == (rr) && (c).g == (gg) && (c).b == (bb))

static void test_parsecolor_formats(void)
{
    rgb_t c;

    char s1[] = "#123456";     parsecolor(&c, s1);
    CHECK_RGB("parsecolor #123456 channel order",   c, 0x12,0x34,0x56);
    char s2[] = "123456";      parsecolor(&c, s2);
    CHECK_RGB("parsecolor 123456 channel order",    c, 0x12,0x34,0x56);
    char s3[] = "0x123456";    parsecolor(&c, s3);
    CHECK_RGB("parsecolor 0x123456 channel order",  c, 0x12,0x34,0x56);
    char s4[] = "18,52,86";    parsecolor(&c, s4);
    CHECK_RGB("parsecolor 18,52,86 channel order",  c, 18,52,86);
    char s5[] = "0x12,0x34,0x56"; parsecolor(&c, s5);
    CHECK_RGB("parsecolor 0x12,0x34,0x56",          c, 0x12,0x34,0x56);
    char s6[] = "18, 52, 86";  parsecolor(&c, s6);
    CHECK_RGB("parsecolor decimal with spaces",     c, 18,52,86);

    char s_up[] = "#ABCDEF";   parsecolor(&c, s_up);
    CHECK_RGB("parsecolor #ABCDEF",                 c, 0xab,0xcd,0xef);
    char s_lo[] = "#abcdef";   parsecolor(&c, s_lo);
    CHECK_RGB("parsecolor #abcdef",                 c, 0xab,0xcd,0xef);

    // partial decimal lists zero-fill the rest
    char s_one[] = "200";      parsecolor(&c, s_one);
    CHECK_RGB("parsecolor single decimal 200",      c, 200,0,0);
    char s_two[] = "200,100";  parsecolor(&c, s_two);
    CHECK_RGB("parsecolor two decimals 200,100",    c, 200,100,0);

    char s_empty[] = "";
    c = (rgb_t){1,2,3};        parsecolor(&c, s_empty);
    CHECK_RGB("parsecolor empty string is black",   c, 0,0,0);
}

// ---------------------------------------------------------------------------
// hexread
// ---------------------------------------------------------------------------

static void test_hexread(void)
{
    uint8_t buf[8];
    int n;

    char s_dec[] = "1,2,3";
    n = hexread(buf, s_dec, sizeof(buf));
    CHECK("hexread decimal count=3",  n == 3);
    CHECK("hexread decimal [0]=1",    buf[0] == 1);
    CHECK("hexread decimal [1]=2",    buf[1] == 2);
    CHECK("hexread decimal [2]=3",    buf[2] == 3);

    char s_hex[] = "0xff,0x00,0x01";
    n = hexread(buf, s_hex, sizeof(buf));
    CHECK("hexread hex count=3",      n == 3);
    CHECK("hexread hex [0]=255",      buf[0] == 255);
    CHECK("hexread hex [1]=0",        buf[1] == 0);
    CHECK("hexread hex [2]=1",        buf[2] == 1);

    char s_single[] = "42";
    n = hexread(buf, s_single, sizeof(buf));
    CHECK("hexread single count=1",   n == 1);
    CHECK("hexread single [0]=42",    buf[0] == 42);

    // buflen cap
    char s_many[] = "1,2,3,4,5";
    n = hexread(buf, s_many, 3);
    CHECK("hexread buflen capped at 3", n == 3);

    char s_spaces[] = " 10, 20 ,30";
    n = hexread(buf, s_spaces, sizeof(buf));
    CHECK("hexread spaces count=3",   n == 3);
    CHECK("hexread spaces [2]=30",    buf[2] == 30);

    // NULL input
    n = hexread(buf, NULL, sizeof(buf));
    CHECK("hexread NULL returns -1",  n == -1);
}

// ---------------------------------------------------------------------------
// blink1_adjustBrightness
// ---------------------------------------------------------------------------

static void test_adjustBrightness(void)
{
    uint8_t r, g, b;

    // brightness=0 means no change
    r = 100; g = 200; b = 50;
    blink1_adjustBrightness(0, &r, &g, &b);
    CHECK("adjustBrightness 0 no-op r", r == 100);
    CHECK("adjustBrightness 0 no-op g", g == 200);
    CHECK("adjustBrightness 0 no-op b", b == 50);

    // brightness=128: r = 255*128>>8 = 127
    r = 255; g = 255; b = 255;
    blink1_adjustBrightness(128, &r, &g, &b);
    CHECK("adjustBrightness 128 on 255 → 127", r == 127);
    CHECK("adjustBrightness 128 on 255 g",     g == 127);
    CHECK("adjustBrightness 128 on 255 b",     b == 127);

    // brightness=255: r = 100*255>>8 = 99
    r = 100; g = 100; b = 100;
    blink1_adjustBrightness(255, &r, &g, &b);
    CHECK("adjustBrightness 255 on 100 → 99", r == 99);

    // zero stays zero
    r = 0; g = 0; b = 0;
    blink1_adjustBrightness(255, &r, &g, &b);
    CHECK("adjustBrightness on 0 stays 0", r == 0 && g == 0 && b == 0);
}

// ---------------------------------------------------------------------------
// blink1_degamma
// ---------------------------------------------------------------------------

static void test_degamma(void)
{
    CHECK("degamma 0 → 0",   blink1_degamma(0)   == 0);
    CHECK("degamma 128 → 55", blink1_degamma(128) == 55);
    CHECK("degamma 255 → 255", blink1_degamma(255) == 255);
    // monotonically non-decreasing spot check
    CHECK("degamma monotone 64<=128", blink1_degamma(64) <= blink1_degamma(128));
    CHECK("degamma monotone 128<=192", blink1_degamma(128) <= blink1_degamma(192));
}

// ---------------------------------------------------------------------------
// parsePattern
// ---------------------------------------------------------------------------

static void test_parsePattern(void)
{
    patternline_t pattern[32];
    int repeats, n;

    // 2-line pattern
    char s_two[] = "3,#ff0000,0.5,0,#0000ff,0.5,0";
    n = parsePattern(s_two, &repeats, pattern);
    CHECK("parsePattern 2-line count",    n == 2);
    CHECK("parsePattern repeats=3",       repeats == 3);
    CHECK("parsePattern [0] r=255",       pattern[0].color.r == 255);
    CHECK("parsePattern [0] g=0",         pattern[0].color.g == 0);
    CHECK("parsePattern [0] b=0",         pattern[0].color.b == 0);
    CHECK("parsePattern [0] millis=500",  pattern[0].millis == 500);
    CHECK("parsePattern [0] ledn=0",      pattern[0].ledn == 0);
    CHECK("parsePattern [1] r=0",         pattern[1].color.r == 0);
    CHECK("parsePattern [1] g=0",         pattern[1].color.g == 0);
    CHECK("parsePattern [1] b=255",       pattern[1].color.b == 255);
    CHECK("parsePattern [1] millis=500",  pattern[1].millis == 500);
    CHECK("parsePattern [1] ledn=0",      pattern[1].ledn == 0);

    // 1-line with ledn=2 and 1-second fade
    char s_one[] = "1,#ffffff,1.0,2";
    n = parsePattern(s_one, &repeats, pattern);
    CHECK("parsePattern 1-line count",   n == 1);
    CHECK("parsePattern 1-line repeats", repeats == 1);
    CHECK("parsePattern 1-line r=255",   pattern[0].color.r == 255);
    CHECK("parsePattern 1-line g=255",   pattern[0].color.g == 255);
    CHECK("parsePattern 1-line b=255",   pattern[0].color.b == 255);
    CHECK("parsePattern 1-line millis",  pattern[0].millis == 1000);
    CHECK("parsePattern 1-line ledn=2",  pattern[0].ledn == 2);

    // zero repeats
    char s_zero[] = "0,#ff0000,0.5,0";
    n = parsePattern(s_zero, &repeats, pattern);
    CHECK("parsePattern repeats=0", repeats == 0);

    // bad: missing ledn field
    char s_bad[] = "1,#ff0000,0.5";
    n = parsePattern(s_bad, &repeats, pattern);
    CHECK("parsePattern missing ledn → -1", n == -1);

    // bad: missing time field
    char s_bad2[] = "1,#ff0000";
    n = parsePattern(s_bad2, &repeats, pattern);
    CHECK("parsePattern missing time → -1", n == -1);

    // non-hex color goes through hexread(), must not break pattern tokenizing
    char s_nonhex[] = "2,0,0.5,1,#00ff00,0.5,2";
    n = parsePattern(s_nonhex, &repeats, pattern);
    CHECK("parsePattern non-hex color count",  n == 2);
    CHECK("parsePattern non-hex [0] ledn=1",   pattern[0].ledn == 1);
    CHECK("parsePattern non-hex [1] g=255",    pattern[1].color.g == 255);
    CHECK("parsePattern non-hex [1] ledn=2",   pattern[1].ledn == 2);

    // too many lines: must stop at blink1_pattern_lines_max, not overrun
    struct {
        patternline_t p[blink1_pattern_lines_max];
        patternline_t guard;
    } big;
    memset(&big.guard, 0xAA, sizeof(big.guard));
    char s_long[1000] = "1";
    for( int i=0; i < blink1_pattern_lines_max + 8; i++ ) {
        strcat(s_long, ",#ffffff,0.1,0");
    }
    n = parsePattern(s_long, &repeats, big.p);
    CHECK("parsePattern overlong capped",  n == blink1_pattern_lines_max);
    CHECK("parsePattern overlong guard",   big.guard.millis == 0xAAAA);
}

static void test_parsePattern_colors(void)
{
    patternline_t p[blink1_pattern_lines_max];
    int repeats, n;

    char s_order[] = "1,#123456,0.5,0";
    n = parsePattern(s_order, &repeats, p);
    CHECK("pattern #123456 count",           n == 1);
    CHECK_RGB("pattern #123456 channel order", p[0].color, 0x12,0x34,0x56);

    char s_nohash[] = "1,123456,0.5,0";
    n = parsePattern(s_nohash, &repeats, p);
    CHECK("pattern no-hash hex count",       n == 1);
    CHECK_RGB("pattern no-hash hex color",   p[0].color, 0x12,0x34,0x56);

    char s_0x[] = "1,0x123456,0.5,0";
    n = parsePattern(s_0x, &repeats, p);
    CHECK("pattern 0x hex count",            n == 1);
    CHECK_RGB("pattern 0x hex color",        p[0].color, 0x12,0x34,0x56);

    char s_upper[] = "1,#ABCDEF,0.5,0,#abcdef,0.5,0";
    n = parsePattern(s_upper, &repeats, p);
    CHECK("pattern mixed-case count",        n == 2);
    CHECK_RGB("pattern uppercase hex",       p[0].color, 0xab,0xcd,0xef);
    CHECK_RGB("pattern lowercase hex",       p[1].color, 0xab,0xcd,0xef);

    char s_ws[] = " 3, #ff0000 , 0.5,  1 ,\t#00ff00,0.25,2 ";
    n = parsePattern(s_ws, &repeats, p);
    CHECK("pattern whitespace count",        n == 2);
    CHECK("pattern whitespace repeats",      repeats == 3);
    CHECK_RGB("pattern whitespace [0]",      p[0].color, 255,0,0);
    CHECK("pattern whitespace [0] ledn=1",   p[0].ledn == 1);
    CHECK_RGB("pattern whitespace [1]",      p[1].color, 0,255,0);
    CHECK("pattern whitespace [1] millis",   p[1].millis == 250);

    char s_trail[] = "1,#ff0000,0.5,0,";
    n = parsePattern(s_trail, &repeats, p);
    CHECK("pattern trailing comma ignored",  n == 1);

    char s_repeats_only[] = "5";
    n = parsePattern(s_repeats_only, &repeats, p);
    CHECK("pattern repeats-only count=0",    n == 0);
    CHECK("pattern repeats-only repeats=5",  repeats == 5);

    char s_empty[] = "";
    n = parsePattern(s_empty, &repeats, p);
    CHECK("pattern empty count=0",           n == 0);

    char s_ledn[] = "1,#ffffff,0.1,18";
    n = parsePattern(s_ledn, &repeats, p);
    CHECK("pattern ledn=18 (mk3 max)",       n == 1 && p[0].ledn == 18);

    char s_times[] = "1,#000000,0,0,#000000,10,0,#000000,65.5,0";
    n = parsePattern(s_times, &repeats, p);
    CHECK("pattern time 0 → 0ms",            n == 3 && p[0].millis == 0);
    CHECK("pattern time 10 → 10000ms",       p[1].millis == 10000);
    CHECK("pattern time 65.5 → 65500ms",     p[2].millis == 65500);

    // every centisecond time must survive atof()*1000 truncation
    int bad_times = 0;
    for( int cs=0; cs <= 6553; cs++ ) {
        char s[64];
        snprintf(s, sizeof(s), "1,#000000,%d.%02d,0", cs/100, cs%100);
        if( parsePattern(s, &repeats, p) != 1 || p[0].millis != cs*10 ) {
            bad_times++;
        }
    }
    CHECK("pattern all centisecond times exact", bad_times == 0);

    // exactly the max fits, with nothing dropped
    char s_max[1000] = "1";
    for( int i=0; i < blink1_pattern_lines_max; i++ ) {
        char line[32];
        snprintf(line, sizeof(line), ",#%02x0000,0.1,0", i);
        strcat(s_max, line);
    }
    n = parsePattern(s_max, &repeats, p);
    CHECK("pattern exactly max lines",       n == blink1_pattern_lines_max);
    CHECK("pattern max last line color",
          p[blink1_pattern_lines_max-1].color.r == blink1_pattern_lines_max-1);
}

// built-in patterns must parse and survive a toPatternString round trip
static void test_builtin_patterns(void)
{
    int cnt = sizeof(blink1_patterns)/sizeof(blink1_pattern_info);
    int bad_parse = 0, bad_roundtrip = 0;
    for( int i=0; i<cnt; i++ ) {
        patternline_t p1[blink1_pattern_lines_max], p2[blink1_pattern_lines_max];
        char s[1000], out[1000];
        int r1, r2;
        snprintf(s, sizeof(s), "%s", blink1_patterns[i].str);
        int n1 = parsePattern(s, &r1, p1);
        if( n1 <= 0 ) {
            printf("      bad parse: '%s'\n", blink1_patterns[i].name);
            bad_parse++;
            continue;
        }
        toPatternString(p1, n1, r1, out);
        int n2 = parsePattern(out, &r2, p2);
        int same = (n2 == n1 && r2 == r1);
        for( int j=0; same && j<n1; j++ ) {  // not memcmp(), struct has padding
            same = p1[j].color.r == p2[j].color.r && p1[j].color.g == p2[j].color.g &&
                   p1[j].color.b == p2[j].color.b && p1[j].millis == p2[j].millis &&
                   p1[j].ledn == p2[j].ledn;
        }
        if( !same ) {
            printf("      bad round trip: '%s' → '%s'\n", blink1_patterns[i].name, out);
            bad_roundtrip++;
        }
    }
    CHECK("builtin patterns all parse",       bad_parse == 0);
    CHECK("builtin patterns all round-trip",  bad_roundtrip == 0);
}

// ---------------------------------------------------------------------------
// toPatternString
// ---------------------------------------------------------------------------

static void test_toPatternString(void)
{
    patternline_t pattern[32];
    char out[512];
    int repeats;

    // NULL pattern → 0
    int n = toPatternString(NULL, 0, 0, out);
    CHECK("toPatternString NULL → 0", n == 0);

    // 1-line round-trip (accumulation bug doesn't affect single line)
    char s1[] = "1,#ffffff,1.0,2";
    parsePattern(s1, &repeats, pattern);
    toPatternString(pattern, 1, repeats, out);
    CHECK("toPatternString 1-line", strcmp(out, "1,#ffffff,1.00,2") == 0);

    // 2-line round-trip
    char s2[] = "3,#ff0000,0.5,0,#0000ff,0.5,0";
    parsePattern(s2, &repeats, pattern);
    toPatternString(pattern, 2, repeats, out);
    CHECK("toPatternString 2-line", strcmp(out, "3,#ff0000,0.50,0,#0000ff,0.50,0") == 0);

    // 3-line round-trip — validates the len+= accumulation fix
    char s3[] = "2,#ff0000,0.5,0,#00ff00,0.5,1,#0000ff,0.5,2";
    parsePattern(s3, &repeats, pattern);
    toPatternString(pattern, 3, repeats, out);
    CHECK("toPatternString 3-line", strcmp(out, "2,#ff0000,0.50,0,#00ff00,0.50,1,#0000ff,0.50,2") == 0);
}

// ---------------------------------------------------------------------------
// hsbtorgb
// ---------------------------------------------------------------------------

static void test_hsbtorgb(void)
{
    rgb_t rgb;

    // pure red: h=0, s=255, v=255 → r=255, g=0, b=0
    uint8_t hsb_red[] = {0, 255, 255};
    rgb = (rgb_t){0, 0, 0};
    hsbtorgb(&rgb, hsb_red);
    CHECK("hsbtorgb red r=255", rgb.r == 255);
    CHECK("hsbtorgb red g=0",   rgb.g == 0);
    CHECK("hsbtorgb red b=0",   rgb.b == 0);

    // black: v=0 → all zeros
    uint8_t hsb_black[] = {0, 255, 0};
    rgb = (rgb_t){0, 0, 0};
    hsbtorgb(&rgb, hsb_black);
    CHECK("hsbtorgb black r=0", rgb.r == 0);
    CHECK("hsbtorgb black g=0", rgb.g == 0);
    CHECK("hsbtorgb black b=0", rgb.b == 0);

    // grayscale: s=0, v=128 → r=g=b=128
    uint8_t hsb_gray[] = {0, 0, 128};
    rgb = (rgb_t){0, 0, 0};
    hsbtorgb(&rgb, hsb_gray);
    CHECK("hsbtorgb grayscale r=128", rgb.r == 128);
    CHECK("hsbtorgb grayscale g=128", rgb.g == 128);
    CHECK("hsbtorgb grayscale b=128", rgb.b == 128);

    // hue wheel: green at 85, blue at 170, each primary dominant
    uint8_t hsb_green[] = {85, 255, 255};
    hsbtorgb(&rgb, hsb_green);
    CHECK("hsbtorgb hue 85 is green",  rgb.g == 255 && rgb.r < 8 && rgb.b == 0);
    uint8_t hsb_blue[] = {170, 255, 255};
    hsbtorgb(&rgb, hsb_blue);
    CHECK("hsbtorgb hue 170 is blue",  rgb.b == 255 && rgb.r == 0 && rgb.g < 16);

    // brightness scales the dominant channel
    uint8_t hsb_dimred[] = {0, 255, 100};
    hsbtorgb(&rgb, hsb_dimred);
    CHECK("hsbtorgb v=100 red",        rgb.r == 100 && rgb.g == 0 && rgb.b == 0);
}

// ---------------------------------------------------------------------------

int main(void)
{
    msg_setquiet(1); // silence parsePattern's error output for bad-input tests

    test_parsecolor();
    test_parsecolor_formats();
    test_hexread();
    test_adjustBrightness();
    test_degamma();
    test_parsePattern();
    test_parsePattern_colors();
    test_builtin_patterns();
    test_toPatternString();
    test_hsbtorgb();

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return (tests_failed > 0) ? 1 : 0;
}
