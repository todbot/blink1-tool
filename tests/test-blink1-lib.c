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
}

// ---------------------------------------------------------------------------

int main(void)
{
    msg_setquiet(1); // silence parsePattern's error output for bad-input tests

    test_parsecolor();
    test_hexread();
    test_adjustBrightness();
    test_degamma();
    test_parsePattern();
    test_toPatternString();
    test_hsbtorgb();

    printf("\n%d/%d tests passed\n", tests_run - tests_failed, tests_run);
    return (tests_failed > 0) ? 1 : 0;
}
