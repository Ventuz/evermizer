// yes, this is a mess, but I really want to make single-c-file work
// this is the simplest way to get a static EXE and WASM
// from a common source, also without requiring project/make files
// see google doc for documentation on the actual patches used below
// the idea is to "manually" patch the game to a state where we simply swap
// out some numbers to make it random (without rewriting/relocating everything)
#define VERSION "v018"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <strings.h> // strcasecmp
#include <ctype.h>
#include <stdint.h>
#include <stdbool.h> 
#include "tinymt64.h"
#ifndef NO_UI // includes and helpers for UI
#if defined(WIN32) || defined(_WIN32)
#include <process.h>
#include <conio.h>
#define clrscr() system("cls");
#else
#include <termios.h>
#include <unistd.h>
#define clrscr() printf("\e[1;1H\e[2J")
char getch() {
    static struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    char c = getchar();
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    return c;
}
#endif
#endif
#ifdef WITH_ASSERT // include or override assert
#include <assert.h>
#else
#ifndef NO_ASSERT
#define NO_ASSERT
#pragma message "NOTE: Defaulting to NO_ASSERT"
#endif
#define assert(x) do { if(x){} } while(false);
#endif
#if defined(WIN32) || defined(_WIN32)
#define DIRSEP '\\'
#ifndef PRIx64
#define PRIx64 "I64x"
#endif
#else
#define DIRSEP '/'
#endif

// Utility functions
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef SWAP
#define SWAP(a,b,T) do {\
    T SWAP = a;\
    a = b;\
    b = SWAP;\
} while(0)
#endif
#if (!defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)) && !defined(_Static_assert)
#   define _Static_assert(a,b)
#endif
#if defined __GNUC__ && !defined __clang__ && __GNUC__>=9 && defined __OPTIMIZE__
#   define GCC_Static_assert _Static_assert // non-standard static assert
#else
#   define GCC_Static_assert(a,b) assert(a) // fall back to runtime assert
#endif
#ifndef __has_builtin
  #define __has_builtin(x) 0
#endif
#if defined __GNUC__ && !defined __STRICT_ANSI__ && (__GNUC__>5 || __has_builtin(__builtin_types_compatible_p))
#define BUILD_BUG_OR_ZERO(e) ((int)(sizeof(struct { int:(-!!(e)); })))
#define ZERO_IF_ARRAY(a) BUILD_BUG_OR_ZERO(__builtin_types_compatible_p(typeof(a), typeof(&a[0])))
#else
#define ZERO_IF_ARRAY(a) 0
#endif
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]) + ZERO_IF_ARRAY(a))
#ifdef NO_UI
static bool batch = true;
#else
static bool batch = false;
#endif
void die(const char* msg)
{
    if (msg) fprintf(stderr, "%s", msg);
#if (defined(WIN32) || defined(_WIN32)) && !defined(NO_UI)
    if (!batch) system("pause");
#else
    (void)batch; // ignore warnings
#endif
    exit(1);
}
void rtrim(char* s)
{
    char* t = s+strlen(s)-1;
    while (t>=s && *t == ' ') *t-- = '\0';
}
const char B32[] = "abcdefghijklmnopqrstuvwxyz234567=";
char b32(unsigned v) { return B32[v&0x1f]; }
#define APPLY_PATCH(buf, patch, loc) memcpy(buf+loc, patch, sizeof(patch)-1)

// RNG functions
static struct TINYMT64_T mt;
#define rand64() tinymt64_generate_uint64(&mt)
#define srand64(seed) tinymt64_init(&mt, seed)
void shuffle_u8(uint8_t *array, size_t n)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand64() / (UINT64_MAX / (n - i) + 1);
          uint8_t t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}
void shuffle_u8_pairs(uint8_t *array, size_t n/*pairs*/)
{
    if (n > 1) 
    {
        size_t i;
        for (i = 0; i < n - 1; i++) 
        {
          size_t j = i + rand64() / (UINT64_MAX / (n - i) + 1);
          uint8_t t;
          t = array[j*2];
          array[j+2] = array[i*2];
          array[i+2] = t;
          t = array[j*2+1];
          array[j+2+1] = array[i*2+1];
          array[i+2+1] = t;
        }
    }
}
uint8_t rand_u8(uint8_t min, uint8_t max)
{
    if (max<=min) return min;
    return (min+(uint8_t)(rand64()%(max-min+1)));
}
uint8_t rand_u8_except(uint8_t min, uint8_t max, uint8_t except)
{
    uint8_t res = rand_u8(min, max-1);
    if (res>=except) res++;
    return res;
}
uint8_t rand_amount(uint8_t min, uint8_t max, float cur_minus_expected)
{
    // cur_minus_expected >0: decrease max
    // cur_minus_expected <0: increase min
    // NOTE: a probability curve would better than just setting limits here.
    if (cur_minus_expected<-3.0) return rand_u8(min+1, max);
    if (cur_minus_expected> 3.0) return rand_u8(min, max-1);
    return rand_u8(min, max);
}


#include "data.h"


// Misc consts
const char DIFFICULTY_CHAR[] = {'e','n','h'};
const char* const DIFFICULTY_NAME[] = {"Easy","Normal","Hard"};
#define DEFAULT_openworld      true
#define DEFAULT_difficulty     1
#define DEFAULT_fixsequence    true
#define DEFAULT_fixcheats      true
#define DEFAULT_glitchless     true
#define DEFAULT_chaos          false
#define DEFAULT_ingredienizer  true
#define DEFAULT_alchemizer     true
#define DEFAULT_bossdropamizer true
#define DEFAULT_gourdomizer    true
#define DEFAULT_sniffamizer    true
#define DEFAULT_doggomizer     false
#define DEFAULT_enemizer       false
#define DEFAULT_musicmizer     false
#define DEFAULT_spoilerlog     false

#define D(t) DEFAULT_ ## t
#define DEFAULT_OW() do {\
    openworld   = D(openworld);\
    fixsequence = D(fixsequence);\
    fixcheats   = D(fixcheats);\
} while (false)
#define DEFAULT_RANDO() do {\
    openworld      = D(openworld);\
    fixsequence    = D(fixsequence);\
    fixcheats      = D(fixcheats);\
    ingredienizer  = D(ingredienizer);\
    alchemizer     = D(alchemizer);\
    bossdropamizer = D(bossdropamizer);\
    gourdomizer    = D(gourdomizer);\
/*  sniffamizer    = D(sniffamizer);\
    doggomizer     = D(doggomizer);\
    enemizer       = D(enemizer);*/\
    glitchless     = D(glitchless);\
    chaos          = D(chaos);\
    difficulty     = D(difficulty);\
    musicmizer     = D(musicmizer);\
    spoilerlog     = D(spoilerlog);\
} while (false)
#ifdef NO_RANDO
#define DEFAULT_SETTINGS() DEFAULT_OW()
#else
#define DEFAULT_SETTINGS() DEFAULT_RANDO()
#endif
#define C(t) (t != D(t))
#ifndef NO_RANDO
#define SETTINGS2STR(s)\
    snprintf(s, ARRAY_SIZE(s), "r%c%s%s%s%s%s%s%s%s" /*"%s%s%s"*/ "%s%s",\
        DIFFICULTY_CHAR[difficulty], C(chaos)?"c":"",\
        C(fixsequence)?"1":"",   C(fixcheats)?"2":"",\
        C(glitchless)?"3":"",    C(alchemizer)?"a":"",\
        C(ingredienizer)?"i":"", C(bossdropamizer)?"b":"",\
        C(gourdomizer)?"g":""/*, C(sniffamizer)?"s":"",\
        C(doggomizer)?"d":"",    C(enemizer)?"m":""*/,\
        C(musicmizer)?"m":"", C(spoilerlog)?"l":"");
#else
#define SETTINGS2STR(s)\
    snprintf(s, ARRAY_SIZE(s), "r%s%s",\
        C(fixsequence)?"1":"", C(fixcheats)?"2":"");
#endif

#ifdef NO_UI
#define FLAGS "[-o <dst file.sfc>|-d <dst directory>] "
#else
#define FLAGS "[-b|-i] [-o <dst file.sfc>|-d <dst directory>] "
#endif
#ifdef NO_RANDO
#define ARGS " [settings]"
#else
#define ARGS " [settings [seed]]"
#endif

// The actual program
void print_usage(const char* appname)
{
    fprintf(stderr, "Usage: %s " FLAGS "<src file.sfc>" ARGS "\n", appname);
#if defined(WIN32) || defined(_WIN32)
    fprintf(stderr, "       or simply drag & drop your ROM onto the EXE\n");
#ifndef NO_UI
    if (!batch) system("pause");
#endif
#endif
}
int main(int argc, const char** argv)
{
    const char* appname = argv[0];
    
    // verify at least one agument is given
    if (argc<2 || !argv[1] || !argv[1][0]) {
        print_usage(appname);
        die(NULL);
    }
    
    // default settings
    bool interactive = true;           // show settings ui
    bool openworld   = D(openworld);   // always enable windwalker and fix resulting bugs
    bool fixsequence = D(fixsequence); // fix sequence breaking glitches
    bool fixcheats   = D(fixcheats);   // fix difficulty breaking glitches, excluding atlas
    #ifdef NO_RANDO // open world + fixes only
    (void)alchemy_locations; // suppress warning
    (void)interactive; // no ui in OW-only (yet)
    #else // rando
    bool ingredienizer  = D(ingredienizer);  // randomize ingredients required for alchemy
    bool alchemizer     = D(alchemizer);     // shuffle spell drops
    bool bossdropamizer = D(bossdropamizer); // shuffle boss drops
    bool gourdomizer    = D(gourdomizer);    // shuffle gourds, not implemented
  //bool sniffamizer    = D(sniffamizer);    // shuffle sniffing spots
  //bool doggomizer     = D(doggomizer);     // shuffle dogs
  //bool enemizer       = D(enemizer);       // shuffle enemy spawns
    bool musicmizer     = D(musicmizer);     // random music
    bool glitchless     = D(glitchless);     // may not require glitches to complete
    bool chaos          = D(chaos);          // randomize more
    uint8_t difficulty  = D(difficulty);     // 0=easy, 1=normal, 2=hard
    bool spoilerlog     = D(spoilerlog);     // randomize more
    #endif
    
    const char* ofn=NULL;
    const char* dstdir=NULL;
    bool modeforced=false;
    
    // parse command line arguments
    while (true) {
        if (strcmp(argv[1], "-b") == 0) {
            modeforced = true;
            batch = true;
            interactive = false;
            argv++; argc--;
        } else if (strcmp(argv[1], "-i") == 0) {
            modeforced = true;
            interactive = true;
    #ifdef NO_UI
            fprintf(stderr, "Requested interactive mode, but not compiled in!\n");
            print_usage(appname);
            die(NULL);
            (void) interactive; // ignore unused variable
    #endif
            argv++; argc--;
        } else if (strcmp(argv[1], "-o") == 0) {
            ofn = argv[2];
            argv+=2; argc-=2;
            if (dstdir) die("Can't have -o and -d\n");
        } else if (strcmp(argv[1], "-d") == 0) {
            dstdir = argv[2];
            argv+=2; argc-=2;
            if (ofn) die("Can't have -o and -d\n");
        } else if (strcmp(argv[1], "--version") == 0) {
            printf("%s\n", VERSION);
            return 0;
        } else if (strcmp(argv[1], "--help") == 0) {
            print_usage(appname);
            return 0;
        } else {
            break;
        }
    }
    
    if (!modeforced) {
        #ifdef NO_RANDO
        interactive = argc<3;
        #else
        interactive = argc<4;
        #endif
    }
    
    // verify number of command line arguments
    #ifdef NO_RANDO
    if (argc<2 || !argv[1] || !argv[1][0] || argc>3) {
    #else
    if (argc<2 || !argv[1] || !argv[1][0] || argc>4) {
    #endif
        print_usage(appname);
        die(NULL);
    }
    
    // parse settings command line argument
    if (argc>=3) {
        for (const char* s=argv[2]; *s; s++) {
            char c = tolower(*s);
        #ifndef NO_RANDO
            for (size_t i=0; i<ARRAY_SIZE(DIFFICULTY_CHAR); i++) {
                if (c == DIFFICULTY_CHAR[i]) { difficulty = i; c = 0; }
            }
        #endif
            if (c == '1') fixsequence = !fixsequence;
            else if (c == '2') fixcheats = !fixcheats;
        #ifndef NO_RANDO
            else if (c == '3') glitchless = !glitchless;
            else if (c == 'c') chaos = !chaos;
            else if (c == 'a') alchemizer = !alchemizer;
            else if (c == 'i') ingredienizer = !ingredienizer;
            else if (c == 'b') bossdropamizer = !bossdropamizer;
            else if (c == 'g') gourdomizer = !gourdomizer;
          //else if (c == 's') sniffamizer = !sniffamizer;
          //else if (c == 'd') doggomizer = !doggomizer;
          //else if (c == 'y') enemizer = !enemizer;
            else if (c == 'm') musicmizer = !musicmizer;
            else if (c == 'l') spoilerlog = !spoilerlog;
        #endif
            else if (c == 'r') DEFAULT_SETTINGS();
            else if (c != 0) {
                fprintf(stderr, "Unknown setting '%c' in \"%s\"\n",
                        c, argv[2]);
                die(NULL);
            }
        }
    }
    
    // parse source file command line argument
    const char* src = argv[1];
    
    // load rom
    FILE* fsrc = fopen(src,"rb");
    if (!fsrc) die("Could not open input file!\n");
    fseek(fsrc, 0L, SEEK_END);
    size_t sz = ftell(fsrc);
    fseek(fsrc, 0L, SEEK_SET);
    if (sz != 3145728 && sz != 3145728+512) { fclose(fsrc); die("ROM has to be 3MB SFC with or without header!\n"); }
    
    const size_t rom_off = (sz == 3145728+512) ? 512 : 0;
    bool grow = false; // will be set by patches if gowing the rom is required
    #define GROW_BY (1024*1024) // 1MB to a round 4MB
    
    uint8_t* buf = (uint8_t*)malloc(sz+GROW_BY); // allow to grow by 1MB
    memset(buf+sz, 0, GROW_BY); // or 0xff?
    size_t len = fread(buf, 1, sz, fsrc);
    if (len!=sz) die("Could not read input file!\n");
    
    // check ROM header
    const char cart_header[] = "SECRET OF EVERMORE   \x31\x02\x0c\x03\x01\x33\x00";
    const size_t cart_header_loc = 0xFFC0;
    if (memcmp((char*)buf + rom_off + cart_header_loc, cart_header, sizeof(cart_header)-1) != 0)
    {
        size_t i = rom_off+cart_header_loc + 0x15;
        fprintf(stderr, "Wrong Header: %.21s %02X %02X %02X %02X %02x %02x %02x\n"
                        "Expected:     SECRET OF EVERMORE    31 02 0c 03 01 33 00\n",
                        (char*)buf+rom_off+cart_header_loc,
                        buf[i+0], buf[i+1], buf[i+2], buf[i+3],
                        buf[i+4], buf[i+5], buf[i+6]);
        
        die(NULL);
    }
    
    // show command line settings in batch mode
    char settings[15];
    //if (argc>2) strncpy(settings, argv[2], sizeof(settings)); else memcpy(settings, "rn", 3);
    SETTINGS2STR(settings);
    
    #ifndef NO_RANDO
    // random seed number
    time_t t = 0;
    srand64((uint64_t)time(&t));
    uint64_t seed = rand64();
    
    // parse command line seed number
    if (argc>3 && argv[3] && isxdigit(argv[3][0]))
        seed = strtoull(argv[3], NULL, 16);
    
    // show UI in interactive mode
    #ifndef NO_UI // TODO: UI for OW
    if (interactive)
    {
        char seedbuf[17];
        clrscr();
        printf("Evermizer " VERSION "\n");
        if (argc<4) {
            printf("Seed (ENTER for random): ");
            fflush(stdout);
            if (! fgets(seedbuf, sizeof(seedbuf), stdin)) die("\nAborting...\n");
            if (isxdigit(seedbuf[0])) seed = strtoull(seedbuf, NULL, 16);
        }
        while (true) {
            clrscr();
            printf("Evermizer " VERSION "\n");
            printf("Seed: %" PRIx64 "\n", seed);
            SETTINGS2STR(settings);
            printf("Settings: %-14s(Press R to reset)\n", settings);
            printf("\n");
            printf("Difficulty:      %-6s (E/N/H to change)\n", 
                   DIFFICULTY_NAME[difficulty]);
            printf("Chaos:               %s    (C to toggle)\n", chaos?"on ":"off");
            printf("Open World:          %s\n", openworld?"on ":"off");
            printf("Fix sequence:        %s    (1 to toggle)\n", fixsequence?   "on ":"off");
            printf("Fix cheats:          %s    (2 to toggle)\n", fixcheats?     "on ":"off");
            printf("Glitchless beatable: %s    (3 to toggle)\n", glitchless?    "on ":"off");
            printf("Alchemizer:          %s    (A to toggle)\n", alchemizer?    "on ":"off");
            printf("Ingredienizer:       %s    (I to toggle)\n", ingredienizer? "on ":"off");
            printf("Boss dropamizer:     %s    (B to toggle)\n", bossdropamizer?"on ":"off");
            printf("Gourdomizer:         %s    (G to toggle) [Dummy]\n", gourdomizer? "on ":"off");
          //printf("Sniffamizer:         %s    (S to toggle) [Dummy]\n", sniffamizer? "on ":"off");
          //printf("Doggomizer:          %s    (D to toggle) [Dummy]\n", doggomizer?  "on ":"off");
          //printf("Enemizer:            %s    (Y to toggle) [Dummy]\n", enemizer?    "on ":"off");
            printf("Musicmizer:          %s    (M to toggle) [Demo]\n",  musicmizer?  "on ":"off");
            printf("Spoiler Log:         %s    (L to toggle)\n", spoilerlog?    "on ":"off");
            printf("\n");
            printf("Press ESC to abort, ENTER to continue");
            fflush(stdout);
            char c = getch();
            if (c == '\x1b' || c == EOF || c == '\x04' || tolower(c) == 'q') {
                fclose(fsrc); free(buf);
                die("\nAborting...\n");
            }
            if (c == '\r' || c == '\n') break;
            c = tolower(c);
            for (size_t i=0; i<ARRAY_SIZE(DIFFICULTY_CHAR); i++)
                if (c == DIFFICULTY_CHAR[i]) difficulty = i;
            if (c == 'c') chaos = !chaos;
            if (c == '1') fixsequence = !fixsequence;
            if (c == '2') fixcheats = !fixcheats;
            if (c == '3') glitchless = !glitchless;
            if (c == 'a') alchemizer = !alchemizer;
            if (c == 'i') ingredienizer = !ingredienizer;
            if (c == 'b') bossdropamizer = !bossdropamizer;
            if (c == 'g') gourdomizer = !gourdomizer;
          //if (c == 's') sniffamizer = !sniffamizer;
          //if (c == 'd') doggomizer = !doggomizer;
          //if (c == 'y') enemizer = !enemizer;
            if (c == 'm') musicmizer = !musicmizer;
            if (c == 'l') spoilerlog = !spoilerlog;
            if (c == 'r') DEFAULT_SETTINGS();
        }
        clrscr();
    }
    #endif
    printf("Evermizer " VERSION "\n");
    printf("Seed: %" PRIx64 "\n", seed);
    srand64(seed);
    bool randomized = alchemizer || ingredienizer || bossdropamizer ||
                      gourdomizer /*|| sniffamizer || doggomizer ||enemizer*/;
    #else
    printf("SoE OpenWorld " VERSION "\n");
    #endif
    printf("Settings: %-10s\n\n", settings);
    
    // define patches
    #define DEF_LOC(n, location)\
        const size_t PATCH_LOC##n = location
    #define DEF(n, location, content)\
        const size_t PATCH_LOC##n = location;\
        const char PATCH##n[] = content
    #define UNUSED(n)\
        (void)PATCH_LOC##n;\
        (void)PATCH##n;
    
    #include "patches.h" // hand-written c code patches
    #include "gen.h" // generated from patches/
    #ifndef NO_RANDO
    DEF(JUKEBOX_SJUNGLE,      0x938664 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_RAPTORS_1,    0x9391fa - 0x800000, "\x29\x84\x00\x0f\x4d\x4d"); // CALL jukebox3, NOP, NOP
    DEF(JUKEBOX_RATPROS_3,    0x938878 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_DEFEND,       0x94e5b9 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_NJUNGLE,      0x939664 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_EJUNGLE,      0x93b28a - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_ECRUSTACIA,   0x95bb46 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_ECRUSTACIAR,  0x95ba0b - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_CRUSTACIAFP,  0x97c125 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_NOBILIAF,     0x95d72c - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_PALACE,       0x95d43f - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_NOBILIAFP,    0x97c579 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOX_STRONGHHE,    0x94e625 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOK_SWAMPPEPPER,  0x94dde6 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
    DEF(JUKEBOK_SWAMPSLEEP,   0x94def3 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1
  //DEF(JUKEBOX_MARKET1,      0x96b80a - 0x800000, "\x29\x70\x00\x0f"); // this requires different code if we really want to randomize this
  //DEF(JUKEBOX_MARKET2,      0x96b80f - 0x800000, "\x29\x70\x00\x0f"); // ^
  //DEF(JUKEBOX_NMARKET1,     0x95cb4e - 0x800000, "\x29\x70\x00\x0f"); // ^
  //DEF(JUKEBOX_NMARKET2,     0x95cb53 - 0x800000, "\x29\x70\x00\x0f"); // ^
  //DEF(JUKEBOX_SQUARE1,      0x95e216 - 0x800000, "\x29\x70\x00\x0f"); // ^
  //DEF(JUKEBOX_SQUARE2,      0x95e21b - 0x800000, "\x29\x70\x00\x0f"); // ^
  //DEF(JUKEBOX_RAPTORS_2,    0x9387b5 - 0x800000, "\x29\x79\x00\x0f\x4d\x4d"); // CALL jukebox2, NOP, NOP // can't change boss music :(
  //DEF(JUKEBOX_PRISON,       0x98b0fa - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // raptors glitch out
  //DEF(JUKEBOX_VOLCANO_PATH, 0x93ed69 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // raptors glitch out
  //DEF(JUKEBOX_BBM,          0x93c417 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // bbm bridges glitch out
  //DEF(JUKEBOX_WCRUSTACIA,   0x96bd85 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // exploding rocks
  //DEF(JUKEBOX_EHORACE,      0x96c4da - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // exploding rocks
  //DEF(JUKEBOX_PALACEG,      0x96d636 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // doggo fountain sounds
  //DEF(JUKEBOX_FEGATHERING,  0x94c312 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // naming character glitches out
  //DEF(JUKEBOX_WSWAMP,       0x948999 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // frippo sounds and leafpads
  //DEF(JUKEBOX_SWAMP,        0x9492d5 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // frippo sounds and leafpads
  //DEF(JUKEBOX_ACIDRAIN,     0x93af47 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // shopping menu glitches out
  //DEF(JUKEBOX_STRONGHHI,    0x94e7cf - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // shopping menu glitches out
  //DEF(JUKEBOX_BLIMPSCAVE,   0x95b377 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // shopping menu glitches out
  //DEF(JUKEBOX_FEVILLAGE,    0x94cea4 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // shopping menu glitches out
  //DEF(JUKEBOX_HALLS_MAIN,   0x9795af - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // disabled until tested
  //DEF(JUKEBOX_HALLS_NE,     0x97a381 - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // disabled until tested
  //DEF(JUKEBOX_HALLS_NE2,    0x97a16d - 0x800000, "\x29\x70\x00\x0f"); // CALL jukebox1 // disabled until tested
    #endif
    
    #ifndef NO_RANDO
    uint8_t alchemy[ALCHEMY_COUNT];
    _Static_assert(sizeof(alchemy) == 34, "Bad alchemy count");
    for (uint8_t i=0; i<ALCHEMY_COUNT; i++) alchemy[i] = i;
    struct formula ingredients[ALCHEMY_COUNT];
    // preset to vanilla for logic checking without ingredienizer
    {
        const uint8_t ingredient_types[] = INGREDIENT_TYPES; // includes laser
        for (size_t i=0; i<ALCHEMY_COUNT; i++) {
            ingredients[i].type1 = ingredient_types[2*alchemy_locations[i].id+0];
            ingredients[i].type2 = ingredient_types[2*alchemy_locations[i].id+1];
        }
    }
    uint8_t boss_drops[] = BOSS_DROPS;
    
    int rollcount=0;
    if (randomized)
        printf("Rolling");
    fflush(stdout);
    do {
        if (rollcount>198) die("\nCould not satifsy logic in 200 tries. Giving up.\n");
        if (rollcount>0) printf(".");
        if ((rollcount+strlen("Rolling"))%79 == 0) printf("\n"); else fflush(stdout); // 79 chars per line
        rollcount++;
        if (alchemizer) {
            shuffle_u8(alchemy, ALCHEMY_COUNT);
            if (difficulty==0 && !ingredienizer) {
                // make sure that one of acid rain, flash or speed
                // is obtainable before thraxx on easy
                uint8_t at = rand64()%2 ? HARD_BALL_IDX : FLASH_IDX;
                uint8_t spell = rand_u8(0, 2);
                spell = (spell==0) ? FLASH_IDX : (spell==1) ? ACID_RAIN_IDX : SPEED_IDX;
                for (size_t i=0; i<ALCHEMY_COUNT; i++) {
                    if (alchemy[i] == at) { // swap alchemy locations
                        alchemy[i] = alchemy[spell];
                        alchemy[spell] = at;
                        break;
                    }
                }
            }
        }
        if (ingredienizer) {
            const uint8_t min_single_cost = 1;
            const uint8_t max_single_cost = 3;
            const uint8_t max_spell_cost = MIN(4+difficulty, 2*max_single_cost);
            const uint8_t est_total_cost = 92-6 + difficulty*6; // 92/34 for vanilla
            uint8_t cheap_spell_location = (difficulty==0) ? (rand64()%2 ? HARD_BALL_IDX : FLASH_IDX) : 0xff;
            
            uint8_t cur_total_cost = 0;
            if (chaos) {
                for (uint8_t i=0; i<ALCHEMY_COUNT; i++) {
                    uint8_t type1;
                    uint8_t type2;
                    if (alchemy[i] == cheap_spell_location) {
                        type1 = rand_u8(0, 4);
                        type2 = rand_u8_except(0, 4, type1);
                        type1 = pre_thraxx_ingredients[type1];
                        type2 = pre_thraxx_ingredients[type2];
                    } else {
                        type1 = rand_u8(0, 21);
                        type2 = rand_u8_except(0, 21, type1);
                    }
                    uint8_t amount1 = rand_amount(min_single_cost, max_single_cost, 
                                          (float)cur_total_cost - (float)est_total_cost/ALCHEMY_COUNT*i);
                    if (i==LEVITATE_IDX && type1==DRY_ICE)
                        amount1 = 1; // only allow 1 dry ice for levitate
                    cur_total_cost += amount1;
                    uint8_t amount2 = rand_amount(min_single_cost, MIN(max_single_cost,max_spell_cost-amount1), 
                                          (float)cur_total_cost - (float)est_total_cost/ALCHEMY_COUNT*(0.5f+i));
                    if (i==LEVITATE_IDX && type2==DRY_ICE)
                        amount2 = 1; // only allow 1 dry ice for levitate
                    cur_total_cost += amount2;
                    ingredients[i].type1 = type1;
                    ingredients[i].type2 = type2;
                    ingredients[i].amount1 = amount1;
                    ingredients[i].amount2 = amount2;
                }
            } else {
                uint8_t ingredient_types[] = INGREDIENT_TYPES; // includes laser
                if (difficulty==0) { // shuffle original pairs for easy
                    shuffle_u8_pairs(ingredient_types, ARRAY_SIZE(ingredient_types)/2);
                    // make sure cheap_spell_location has only pre_threaxx_ingredients
                    for (size_t i=0; i<ALCHEMY_COUNT; i++) {
                        if (alchemy[i] != cheap_spell_location) continue;
                        if (can_buy_ingredient_pre_thraxx(ingredient_types[i*2+0]) &&
                            can_buy_ingredient_pre_thraxx(ingredient_types[i*2+1])) break;
                        for (size_t j=0; j<ALCHEMY_COUNT; j++) {
                            if (!can_buy_ingredient_pre_thraxx(ingredient_types[j*2+0]) ||
                                !can_buy_ingredient_pre_thraxx(ingredient_types[j*2+1])) continue;
                            // swap ingredients
                            SWAP(ingredient_types[i*2+0],ingredient_types[j*2+0], uint8_t);
                            SWAP(ingredient_types[i*2+1],ingredient_types[j*2+1], uint8_t);
                            break;
                        }
                        break;
                    }
                } else { // shuffle original ingredients
                    shuffle_u8(ingredient_types, ARRAY_SIZE(ingredient_types));
                }
                for (uint8_t i=0; i<ALCHEMY_COUNT; i++) {
                    uint8_t amount1 = rand_amount(min_single_cost, max_single_cost, 
                                          (float)cur_total_cost - (float)est_total_cost/ALCHEMY_COUNT*i);
                    if (i==LEVITATE_IDX && ingredient_types[i*2] == DRY_ICE)
                        amount1 = 1; // only allow 1 dry ice for levitate
                    cur_total_cost += amount1;
                    uint8_t amount2 = rand_amount(min_single_cost, MIN(max_single_cost,max_spell_cost-amount1), 
                                          (float)cur_total_cost - (float)est_total_cost/ALCHEMY_COUNT*(0.5f+i));
                    if (i==LEVITATE_IDX && ingredient_types[i*2+1] == DRY_ICE)
                        amount2 = 1; // only allow 1 dry ice for levitate
                    cur_total_cost += amount2;
                    ingredients[i].type1 = ingredient_types[i*2];
                    ingredients[i].type2 = ingredient_types[i*2+1];
                    ingredients[i].amount1 = amount1;
                    ingredients[i].amount2 = amount2;
                }
            }
        }
        if (bossdropamizer) {
            shuffle_u8(boss_drops, ARRAY_SIZE(boss_drops));
        }
        
        // general logic checking
        #define REROLL() continue;
        
        // boss drop logic: thraxx has to drop a weapon unless gourdomizer is on and we can get back from thraxx and a gourd has a weapon
        if (bossdropamizer)
        {
            if (!boss_drop_is_a_weapon(boss_drops[THRAXX_IDX])) REROLL();
        }
        
        struct formula* levitate_formula = &ingredients[LEVITATE_IDX];
        struct formula* revealer_formula = &ingredients[REVEALER_IDX];
        struct formula* atlas_formula = &ingredients[ATLAS_IDX];
        {
            if (levitate_formula->type1 == METEORITE ||
                levitate_formula->type2 == METEORITE)
                    REROLL(); // reroll, unbeatable or would give away a hint
            if ((levitate_formula->type1 == DRY_ICE && levitate_formula->amount1>1) ||
                (levitate_formula->type2 == DRY_ICE && levitate_formula->amount2>1))
                    REROLL();
            if ((levitate_formula->type1 == DRY_ICE && levitate_formula->amount1==1) ||
                (levitate_formula->type2 == DRY_ICE && levitate_formula->amount2==1))
            {
                // no other formula may use dry ice
                bool ok = true;
                for (size_t i=0; i<ALCHEMY_COUNT; i++) {
                    if (i != LEVITATE_IDX && (ingredients[i].type1 == DRY_ICE || ingredients[i].type2 == DRY_ICE)) {
                        ok = false;
                        break;
                    }
                }
                if (!ok) REROLL();
            }
            if (!can_buy_ingredients(revealer_formula)) REROLL(); // reroll, unbeatable or would give away a hint
            if (alchemy_missable(alchemy[LEVITATE_IDX])) REROLL(); // NOTE: alchemy[a] = b moves a to vanilla b location
            if (alchemy_missable(alchemy[REVEALER_IDX])) REROLL(); // reroll, unbeatable or would give away a hint
            // make sure atlas can be cast on easy in act1-3
            if (difficulty == 0) { // easy
                if (gourdomizer) {
                     if (atlas_formula->type1 == GREASE || atlas_formula->type2 == GREASE) REROLL();
                     if (atlas_formula->type1 == DRY_ICE || atlas_formula->type2 == DRY_ICE) REROLL();
                     if (atlas_formula->type1 == METEORITE || atlas_formula->type2 == METEORITE) REROLL();
                     if (atlas_formula->type1 == GUNPOWDER || atlas_formula->type2 == GUNPOWDER) REROLL();
                } else {
                     if (atlas_formula->type1 == GREASE && atlas_formula->amount1>1) REROLL();
                     if (atlas_formula->type2 == GREASE && atlas_formula->amount2>1) REROLL();
                     if (atlas_formula->type1 == DRY_ICE && atlas_formula->amount1>2) REROLL();
                     if (atlas_formula->type2 == DRY_ICE && atlas_formula->amount2>2) REROLL();
                     if (atlas_formula->type1 == METEORITE && atlas_formula->amount1>2) REROLL();
                     if (atlas_formula->type2 == METEORITE && atlas_formula->amount2>2) REROLL();
                     if (atlas_formula->type1 == GUNPOWDER && atlas_formula->amount1>2) REROLL();
                     if (atlas_formula->type2 == GUNPOWDER && atlas_formula->amount2>2) REROLL();
                }
            }
            // make sure we get one castable spell pre-thraxx on easy
            // NOTE: this should be guaranteed by generation
            if ((ingredienizer || alchemizer) && difficulty == 0) {
                uint8_t castable = 2;
                for (size_t i=0; i<ALCHEMY_COUNT; i++)
                    if ((alchemy[i]==FLASH_IDX || alchemy[i]==HARD_BALL_IDX) &&
                            !can_buy_pre_thraxx(&ingredients[i]))
                        castable--;
                if (castable<1) {
                    fprintf(stderr, "PRE-THRAXX INGREDIENTS BUG: "
                            "Please report this with seed and settings.\n");
                    REROLL();
                }
            }
            // make sure no spell uses the same ingredient twice
            if (ingredienizer) {
                bool ok = true;
                for (uint8_t i=0; i<ALCHEMY_COUNT; i++)
                    if (ingredients[i].type1 == ingredients[i].type2)
                        ok = false;
                if (!ok) REROLL();
            }
        }
        #undef REROLL
        // tree-based milestone logic checking
        bool reroll = false;
        #define REROLL() { reroll = true; break; }
        for (int milestone=0; milestone<2; milestone++)
        {
            enum progression goal = milestone==0 ? P_ROCKET : P_ENERGY_CORE;
            bool allow_rockskip=!fixsequence && !glitchless;
            bool allow_saturnskip=!fixsequence && !glitchless;
            check_tree_item checks[ARRAY_SIZE(blank_check_tree)];
            memcpy(checks, blank_check_tree, sizeof(blank_check_tree));
            int progress[P_END]; memset(progress, 0, sizeof(progress));
            if (allow_rockskip) progress[P_ROCK_SKIP]++;
            bool complete=false;
            while (!complete) {
                complete = true;
                for (size_t i=0; i<ARRAY_SIZE(checks); i++) {
                    if (checks[i].reached) continue;
                    if (check_requires(checks+i, goal)) continue; // don't iterate past goal
                    if (check_reached(checks+i, progress)) {
                        // NOTE: alchemy[a] = b moves a to vanilla b location
                        uint8_t idx = checks[i].type==CHECK_ALCHEMY ? alchemy_lookup(alchemy,checks[i].index) :
                                      checks[i].type==CHECK_BOSS ? boss_drops[checks[i].index] : 0;
                        const drop_tree_item* drop = get_drop(checks[i].type, idx);
                        check_progress(checks+i, progress);
                        drop_progress(drop, progress);
                        #ifdef DEBUG_CHECK_TREE
                        printf("Reached %s\n", check2str(checks+i));
                        if (drop) printf("Got %s\n", drop2str(drop));
                        #endif
                        complete=false;
                    }
                }
            }
            if (allow_saturnskip && goal==P_ENERGY_CORE) { /* goal optional */ }
            else if (progress[goal]<1) REROLL();
            // make sure atlas is reachable if it should be
            if (difficulty==milestone && progress[P_ATLAS]<1) REROLL();
            // FIXME: add ingredients to check-table, so we don't have to do this?
            if (milestone==0) {
                if (progress[P_LEVITATE] && !allow_rockskip && !can_buy_in_act3(levitate_formula)) REROLL();
                if (progress[P_REVEALER] && !can_buy_in_act3(revealer_formula)) REROLL();
            }
        }
        #undef REROLL
        if (reroll) continue;
        break;
    } while (true);
    if (randomized) printf("\n");
    #endif
    
    // apply patches
    #define APPLY(n) APPLY_PATCH(buf, PATCH##n, rom_off + PATCH_LOC##n)
    
    if (fixsequence && !openworld)
        die("Cannot fix glitches without applying open world patch-set!\n");
    
    #ifndef NO_RANDO
    if (bossdropamizer && !openworld)
        die("Cannot randomize boss drops without open world patch-set (yet)!\n");
    #endif
    
    if (openworld) {
        printf("Applying open world patch-set...\n");
        APPLY(1);  APPLY(2);  APPLY(3);  APPLY(4);  APPLY(5);  APPLY(6);
        APPLY(7);  APPLY(8);  APPLY(9);  APPLY(10); APPLY(11); APPLY(12);
        APPLY(13); APPLY(14); APPLY(15); APPLY(16); APPLY(17); APPLY(18);
        APPLY(19); APPLY(20); APPLY(21); APPLY(22); APPLY(23); APPLY(24);
        APPLY(25); APPLY(26); APPLY(27); APPLY(28); APPLY(29); APPLY(30);
        APPLY(31); /* -32- */ APPLY(33); APPLY(34); APPLY(35); APPLY(36);
        APPLY(37); APPLY(38);
        // v005:
        APPLY(39); APPLY(40); APPLY(41); APPLY(42); APPLY(43); APPLY(44);
        APPLY(45); APPLY(46); APPLY(47); /*48-49: see below*/  APPLY(50);
        APPLY(51); APPLY(52); APPLY(53); APPLY(54); APPLY(55); APPLY(56);
        APPLY(57); APPLY(58); APPLY(59);
        // v006:
        APPLY(60); APPLY(61); APPLY(62); APPLY(63); APPLY(64); APPLY(65);
        // v008:
        APPLY(66); /*67-68: see below*/  APPLY(69); APPLY(70); APPLY(71);
        // v009:
        APPLY(72); /*73,74: see below*/  APPLY(75); APPLY(76);
        // v015:
        APPLY(139); APPLY(140); /* supersedes 32 */
        // v017:
        APPLY(147); APPLY(149a); APPLY(149b);
        // v018:
        APPLY(FE_VILLAGE_WW);  APPLY(FE_VILLAGE_WW2); APPLY(FE_VILLAGE_WW3);
        APPLY(FE_VILLAGE_WW4); APPLY(FE_VILLAGE_WW5);
        APPLY(MARKET_REWORK);
        APPLY(ACT3_OW);  APPLY(ACT3_OW2);
    }
    
    // General bug fixes
    printf("Fixing vanilla softlocks...\n");
    // v009:
    APPLY(73);
    // v015:
    APPLY(141); APPLY(142);
    // v017:
    APPLY(146);
    APPLY(148);
    
    printf("Fixing some missables...\n");
    // v018:
    APPLY(ESCAPE_AFTER_DES); // would only be required for alchemizer
    APPLY(PALACE_REWORK); // would only be required for alchemizer
    
    #ifndef NO_RANDO
    if (gourdomizer || bossdropamizer) {
        // v0015:
        APPLY(126); APPLY(127); APPLY(128); APPLY(129); APPLY(130); APPLY(131);
        APPLY(132); APPLY(133); APPLY(134); APPLY(135); APPLY(136); APPLY(137);
        APPLY(138); 
    }
    #endif
    // General features
    printf("Improving quality of life...\n");
    // v014:
    APPLY(106);
    
    if (fixsequence) {
        printf("Applying desolarify patch-set...\n");
        APPLY(48);
        APPLY(49);
        printf("Applying desaturnate patch...\n");
        APPLY(68);
        printf("Disabling double gauge...\n");
        APPLY(145);
    }
    
    if (fixcheats) { // excluding atlas
        printf("Removing infinite call bead glitch...\n");
        APPLY(67);
    }
    
    #ifdef NO_RANDO // ger rid of unused warnings
    UNUSED(74);
    UNUSED(77);
    #else
    if (alchemizer) {
        printf("Applying alchemizer...\n");
        // Alchemy preselection relocation
        grow = true;
        APPLY(107);
        APPLY(108); APPLY(109); APPLY(110); APPLY(111); APPLY(112); APPLY(113);
        APPLY(114); APPLY(115); APPLY(116); APPLY(117); APPLY(118); APPLY(119);
        APPLY(120); APPLY(121); APPLY(122); APPLY(123); APPLY(124); APPLY(125);
        // Write randomized values
        for (uint8_t i=0; i<ALCHEMY_COUNT; i++) {
            // write value from i to locateions of alchemy[i]
            uint16_t id = alchemy_locations[i].id;
            size_t to = alchemy[i];
            //printf("%s @ %s\n", alchemy_locations[i].name, alchemy_locations[to].name);
            for (size_t j=0; alchemy_locations[to].locations[j] != LOC_END; j++) {
                size_t loc = alchemy_locations[to].locations[j];
                assert(loc & 0x8000);
                buf[rom_off + loc + 0] = id & 0xff;
                buf[rom_off + loc + 1] = id >> 8;
                //printf("  writing 0x%02x to 0x%06x\n", (unsigned)id, (unsigned)loc);
            }
            for (size_t j=0; j<ARRAY_SIZE(alchemy_locations[to].preselects); j++) {
                size_t loc = alchemy_locations[to].preselects[j];
                if (loc == LOC_END) continue; // or break
                assert(loc & 0x8000);
                buf[rom_off + loc] = id<<1;
            }
        }
    }
    if (ingredienizer) {
        _Static_assert(sizeof(*ingredients)==4, "Bad padding"); // required for memcpy
        printf("Applying ingredienizer...\n");
        for (uint8_t i=0; i<ALCHEMY_COUNT; i++) {
            uint16_t id = alchemy_locations[i].id;
            memcpy(buf + rom_off + 0x4601F + id*4, &(ingredients[i]), 4);
        }
    }
    if (bossdropamizer) {
        printf("Applying fixes for randomized boss drops...\n");
        APPLY(74); APPLY(78); APPLY(79); APPLY(80);
        // v015:
        APPLY(78a); APPLY(78b); APPLY(78c); APPLY(78d); APPLY(78e);
        APPLY(143); APPLY(144);
        printf("Applying boss dropamizer...\n");
        APPLY(77);
        APPLY(81);  APPLY(82);  APPLY(83);  APPLY(84);  APPLY(85);  APPLY(86);
        APPLY(87);  APPLY(88);  APPLY(89);  APPLY(90);  APPLY(91);  APPLY(92);
        APPLY(93);  APPLY(94);  APPLY(95);  APPLY(96);  APPLY(97);  APPLY(98);
        APPLY(99);  APPLY(100); APPLY(101); APPLY(102); APPLY(103); APPLY(104);
        APPLY(105);
        // actually apply boss drop randomization
        for (size_t i=0; i<ARRAY_SIZE(boss_drops); i++) {
            uint32_t tgt = boss_drop_jump_targets[boss_drops[i]];
            buf[rom_off + boss_drop_jumps[i] + 0] = (uint8_t)(tgt>>0)&0xff;
            buf[rom_off + boss_drop_jumps[i] + 1] = (uint8_t)(tgt>>8)&0xff;
            buf[rom_off + boss_drop_jumps[i] + 2] = (uint8_t)(tgt>>16)&0xff;
        }
    }
    if (musicmizer) {
        printf("Applying musicmizer...\n");
        // NOTE: this is actually for jukebox, not musicmizer
        grow = true;
        APPLY(JUKEBOX);
        APPLY(JUKEBOX_SJUNGLE);
        APPLY(JUKEBOX_RAPTORS_1);
        APPLY(JUKEBOX_RATPROS_3);
        APPLY(JUKEBOX_NJUNGLE);
        APPLY(JUKEBOX_EJUNGLE);
        APPLY(JUKEBOX_DEFEND);
        APPLY(JUKEBOX_ECRUSTACIA);
        APPLY(JUKEBOX_ECRUSTACIAR);
        APPLY(JUKEBOX_CRUSTACIAFP);
        APPLY(JUKEBOX_NOBILIAF);
        APPLY(JUKEBOX_PALACE);
        APPLY(JUKEBOX_NOBILIAFP);
        APPLY(JUKEBOX_STRONGHHE);
        APPLY(JUKEBOK_SWAMPPEPPER);
        APPLY(JUKEBOK_SWAMPSLEEP);
    }
    
    // if check value differs, the generated ROMs are different.
    uint32_t seedcheck = (uint16_t)(rand64()&0x3ff); // 10bits=2 b32 chars
    if (openworld)      seedcheck |= 0x00000400;
    if (fixsequence)    seedcheck |= 0x00000800;
    if (fixcheats)      seedcheck |= 0x00001000; // excluding atlas
    if (glitchless)     seedcheck |= 0x00002000;
    if (bossdropamizer) seedcheck |= 0x00004000;
    if (alchemizer)     seedcheck |= 0x00008000;
    if (ingredienizer)  seedcheck |= 0x00010000;
    if (gourdomizer)    seedcheck |= 0x00020000;
  //if (sniffamizer)    seedcheck |= 0x00040000;
  //if (doggomizer)     seedcheck |= 0x00080000;
  //if (enemizer)       seedcheck |= 0x00100000;
    if (chaos)          seedcheck |= 0x01000000; // 25bits in use -> 5 b32 chars
    seedcheck |= ((uint32_t)difficulty<<22);
    printf("\nCheck: %c%c%c%c%c (Please compare before racing)\n",
           b32(seedcheck>>20), b32(seedcheck>>15),
           b32(seedcheck>>10), b32(seedcheck>>5),  b32(seedcheck>>0));
    #endif
    
    
    char shortsettings[sizeof(settings)];
    {
        memset(shortsettings, 0, sizeof(shortsettings));
        char* a = shortsettings; char* b = settings;
        while (*b) if (*b!='r' && *b!='l') *a++=*b++; else b++;
        if (!shortsettings[0]) shortsettings[0]='r';
    }
#ifdef NO_RANDO
    char dsttitle[strlen("SoE-OpenWorld_")+strlen(VERSION)+1+sizeof(shortsettings)-1+1]; // SoE-OpenWorld_vXXX_e0123
#else
    char dsttitle[strlen("Evermizer_")+strlen(VERSION)+1+sizeof(shortsettings)-1+1+16+1]; // Evermizer_vXXX_e0123caibgsdm_XXXXXXXXXXXXXXXX
    assert(snprintf(dsttitle, sizeof(dsttitle), "Evermizer_%s_%s_%" PRIx64, VERSION, shortsettings, seed)<sizeof(dsttitle));
    if (!randomized)
#endif
        assert(snprintf(dsttitle, sizeof(dsttitle), "SoE-OpenWorld_%s_%s", VERSION, shortsettings)<sizeof(dsttitle));
    char* pSlash = strrchr(src, DIRSEP);
    if (!pSlash && DIRSEP!='/') pSlash = strrchr(src, '/'); // wine support
    const char* ext = strrchr(src, '.');
    if (!ext || ext<pSlash) ext = ".sfc";
    size_t baselen = pSlash ? (pSlash-src+1) : 0;
    char dstbuf[dstdir? (strlen(dstdir)+1+strlen(dsttitle)+strlen(ext)) : (baselen+strlen(dsttitle)+strlen(ext))+1];
    if (dstdir) {
        size_t p = strlen(dstdir);
        memcpy(dstbuf, dstdir, p);
        if (p>0 && dstbuf[p-1]!='/' && dstbuf[p-1]!='\\')
            dstbuf[p++]=DIRSEP;
        memcpy(dstbuf+p, dsttitle, strlen(dsttitle));
        memcpy(dstbuf+p+strlen(dsttitle), ext, strlen(ext)+1);
    } else {
        if (baselen) memcpy(dstbuf, src, baselen);
        char* p = dstbuf + (pSlash ? (pSlash-src+1) : 0);
        memcpy(p, dsttitle, strlen(dsttitle));
        memcpy(p+strlen(dsttitle), ext, strlen(ext)+1);
    }
    
    const char* dst = (ofn && *ofn) ? ofn : dstbuf;
    
    FILE* fdst = fopen(dst,"wb");
    if (!fdst) { fclose(fsrc); free(buf); die("Could not open output file!\n"); }
    if (grow) sz+=GROW_BY;
    // TODO: recalculate checksum
    len = fwrite(buf, 1, sz, fdst);
    fclose(fdst); fdst=NULL;
    if (len<sz) die("Could not write output file!\n");
    printf("Rom saved as %s!\n", dst);
    
    // write spoiler log
#ifndef NO_RANDO
    if (spoilerlog) {
    char logdstbuf[strlen(dst)+strlen("_SPOILER.log")+1];
    pSlash = strrchr(dst, DIRSEP);
    ext = strrchr(dst, '.');
    if (!ext || ext<pSlash) ext = dst+strlen(dst);
    memcpy(logdstbuf, dst, ext-dst);
    memcpy(logdstbuf+(ext-dst), "_SPOILER.log", strlen("_SPOILER.log")+1);
    
    FILE* flog = fopen(logdstbuf,"wb");
    if (!flog) { fclose(fsrc); free(buf); die("Could not open spoiler log file!\n"); }
    #define ENDL "\r\n"
    fprintf(flog,"Spoiler log for evermizer %s settings %s seed %" PRIx64 "%s", VERSION, shortsettings, seed, ENDL);
    fprintf(flog,"%s", ENDL);
    fprintf(flog,"     %-15s  %-15s  %-15s   %s" ENDL, "Spell", "Location", "Ingredient 1", "Ingredient 2"); 
    fprintf(flog,"------------------------------------------------------------------------" ENDL);
    for (size_t i=0; i<ALCHEMY_COUNT; i++) {
        struct formula* f = &(ingredients[i]);
        size_t to = alchemy[i];
        
        fprintf(flog,"(%02d) %-15s  %-15s  %dx %-12s + %dx %s" ENDL,
            alchemy_locations[i].id, 
            alchemy_locations[i].name,
            alchemy_locations[to].name,
            f->amount1, ingredient_names[f->type1],
            f->amount2, ingredient_names[f->type2]);
    }
    fprintf(flog,"------------------------------------------------------------------------" ENDL);
    fprintf(flog,"%s", ENDL);
    fprintf(flog,"     %-13s  %s" ENDL, "Boss", "Drop");
    fprintf(flog,"------------------------------------------------------------------------" ENDL);
    for (size_t i=0; i<ARRAY_SIZE(boss_drops); i++) {
        fprintf(flog,"(%02d) %-13s  %s" ENDL, (int)i, boss_names[i], boss_drop_names[boss_drops[i]]);
    }
    fprintf(flog,"------------------------------------------------------------------------" ENDL);
    #undef ENDL
    fclose(flog); flog=NULL;
    printf("Spoiler log saved as %s!\n", logdstbuf);
    }
#endif

    free(buf);
    fclose(fsrc);
#if (defined(WIN32) || defined(_WIN32)) && !defined(NO_UI)
    if (!batch) system("pause");
#endif
}
