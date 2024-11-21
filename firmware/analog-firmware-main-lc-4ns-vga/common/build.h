#define BUILDDATE 0x20241004
#define BUILDID 0x0187
#define BUILDSTR " 5 Oct 2024 Build 0187"

#ifdef ANALOG_GS
#define HWSTRING "     V2 Analog GS Rev1"
#define HWBYTE 'G'
#define HWREV '1'
#elif defined(ANALOG_WIFI)
#define HWSTRING "   V2 Analog WiFi Rev1"
#define HWBYTE 'W'
#define HWREV '1'
#else
#define HWSTRING "     V2 Analog LC Rev1"
#define HWBYTE 'L'
#define HWREV '1'
#endif
