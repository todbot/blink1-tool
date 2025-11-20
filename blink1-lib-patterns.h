#ifndef __BLINK1_LIB_PATTERNS_H__
#define __BLINK1_LIB_PATTERNS_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _blink1_pattern_info
{
    char name[100];  // name of pattern
    char str[200];   // string format of pattern data
} blink1_pattern_info;



// list of all the patterns
static const blink1_pattern_info blink1_patterns[]
= {
    {"red flash",        "9,#ff0000,0.5,0,#000000,0.5,0"},
    {"green flash",      "9,#00ff00,0.5,0,#000000,0.5,0"},
    {"blue flash",       "9,#0000ff,0.5,0,#000000,0.5,0"},
    {"white flash",      "9,#ffffff,0.5,0,#000000,0.5,0"},
    {"yellow flash",     "9,#ffff00,0.5,0,#000000,0.5,0"},
    {"purple flash",     "9,#ff00ff,0.5,0,#000000,0.5,0"},
    {"groovy",           "3,#ff4cff,1.0,0,#630000,0.2,0,#0000ff,0.1,0"},
    {"off",              "1,#000000,0.1,0"},
    {"policecar",        "6,#ff0000,0.3,1,#0000ff,0.3,2,#000000,0.1,0,#ff0000,0.3,2,#0000ff,0.3,1,#000000,0.1,0"},
    {"fireengine",       "6,#ff0000,0.3,1,#ff0000,0.3,2,#000000,0.1,0,#ff0000,0.3,2,#ff0000,0.3,1,#000000,0.1,0"},
    {"palette colors",   "3,#e7009a,1,0,#3d00e7,1,0,#00b8e7,1,0,#00e71e,1,0,#d7e700,1,0,#e70000,1,0,#e7e7e7,1,0"},
    {"CMYK",             "3,#00fff2,0.8,0,#000000,0.3,0,#65003c,0.8,0,#000000,0.3,0,#ffd905,0.8,0,#000000,0.1,0,#000000,0.7,0"},
    {"RGB",              "3,#ff0000,0.8,0,#000000,0.3,0,#00ff00,0.8,0,#000000,0.3,0,#0000ff,0.8,0,#000000,0.3,0"},
    {"undervolt",        "1,#821500,1,0,#634100,1,0,#554f00,1,0,#395800,1,0,#00580b,1,0,#005025,1,0,#005844,1,0,#00465a,1,0"},
    {"fire shrine",      "3,#751100,0,1,#ff1d00,0,2,#ff1000,0.4,1,#680300,0.4,2,#000000,0.9,1,#e01200,0,0,#000000,2,0"},
    {"molten lava",      "3,#ff0000,0.2,1,#ff0000,0.2,2,#bd0000,0.1,1,#bd0000,0.1,2,#690000,0.1,1,#690000,0.1,2,#3f0000,0.2,1,#3f0000,0.2,2"},
    {"lighting storm",   "3,#6f756f,0,1,#ffffff,0,2,#ffffff,0.4,1,#686868,0.4,2,#000000,0.9,1,#e0e0e0,0,0,#000000,2,0"},
    {"rain",             "1,#0a01ff,0.1,1,#04004f,0,1,#1701ff,0.4,2,#04004f,0,2,#0019ff,0.3,1,#04004f,0.3,1,#1b01ff,0.3,2,#04004f,0,2"},
    {"nightfall",        "1,#001980,1,0,#000000,10,0"},
    {"dawn",             "1,#000000,1,0,#ff6800,15,0"},
    {"dancefloor",       "6,#ff0004,0.1,1,#ff0004,0.1,2,#f2ff00,0.1,1,#f2ff00,0.1,2,#00ff37,0.1,1,#00ff2a,0.1,2,#ff00aa,0.1,1,#ff00b6,0.1,2"},
    {"rave",             "6,#8b8800,0,0,#010b9e,0.1,0,#009b00,0.1,0,#a5008c,0.1,0,#01998e,0.1,0,#9b0007,0.1,0,#0114a5,0.1,0,#8c8d85,0.1,0"},
    {"sexy",             "3,#e7009a,0.4,1,#ff007b,0,2,#ff00dc,0.4,1,#680029,0.4,2,#000000,0.9,1,#e0006c,0,0,#53004d,2,0"},
    {"calmdown",         "3,#00ff33,2,1,#ff00e9,2,2,#ff0004,2,1,#003fff,2,2,#faff00,2,1,#ffffff,2,1"},
    {"emergency",        "3,#ff7c01,0,2,#732f00,0.1,0,#ff7e00,0,1,#602700,0.3,1"},
    {"lowbattery",       "3,#7a0000,0.1,0,#000000,0,0,#7e0000,0.1,0,#000000,0.1,0,#000000,3,0"},
    {"EKG",              "5,#0c4f00,0,0,#29ff00,0,0,#0c4f00,0.1,0,#29ff00,0.1,0,#0c4f00,2.1,0"},
    {"patternA",         "3,#ff4cff,0.7,0,#630000,0.2,0,#00ff00,0.1,0"},
    {"patternB",         "3,#ff4cff,0.7,0,#630000,0.2,0,#0000ff,0.1,0"},
};


const char* blink1_pattern_find(const char* name)
{
    size_t n = sizeof(blink1_patterns) / sizeof(blink1_patterns[0]);
    for (size_t i = 0; i < n; i++) {
        if (strcmp(blink1_patterns[i].name, name) == 0) {
            return blink1_patterns[i].str;
            //return &blink1_patterns[i];
        }
    }
    return NULL; // not found
}

#ifdef __cplusplus
}
#endif

#endif
