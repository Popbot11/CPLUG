/* Released into the public domain by Tré Dudman - 2024
 * For licensing and more info see https://github.com/Tremus/CPLUG */

#ifndef CPLUG_H
#define CPLUG_H
#ifdef __cplusplus
extern "C" {
#endif

/*  ██████╗██████╗ ██╗     ██╗   ██╗ ██████╗
   ██╔════╝██╔══██╗██║     ██║   ██║██╔════╝
   ██║     ██████╔╝██║     ██║   ██║██║  ███╗
   ██║     ██╔═══╝ ██║     ██║   ██║██║   ██║
   ╚██████╗██║     ███████╗╚██████╔╝╚██████╔╝
    ╚═════╝╚═╝     ╚══════╝ ╚═════╝  ╚═════╝ */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef CPLUG_SHARED
#ifdef _WIN32
#define CPLUG_API __declspec(dllexport)
#else
#define CPLUG_API __attribute__((visibility("default")))
#endif
#else
#define CPLUG_API
#endif // CPLUG_SHARED

#ifndef CPLUG_EVENT_QUEUE_SIZE
#define CPLUG_EVENT_QUEUE_SIZE 256
#endif

typedef union CplugEvent           CplugEvent;
typedef struct CplugHostContext    CplugHostContext;
typedef struct CplugProcessContext CplugProcessContext;

CPLUG_API void cplug_libraryLoad();
CPLUG_API void cplug_libraryUnload();

struct CplugHostContext
{
    // VST3 & AUv2 only, UI thread only.
    void (*sendParamEvent)(CplugHostContext* ctx, const CplugEvent*);
};

CPLUG_API void* cplug_createPlugin(CplugHostContext* ctx);
CPLUG_API void  cplug_destroyPlugin(void*);

CPLUG_API uint32_t cplug_getNumInputBusses(void*);
CPLUG_API uint32_t cplug_getNumOutputBusses(void*);
CPLUG_API uint32_t cplug_getInputBusChannelCount(void*, uint32_t bus_idx);
CPLUG_API uint32_t cplug_getOutputBusChannelCount(void*, uint32_t bus_idx);

// Copy UTF8 name to this buffer, including null terminating byte.
// NOTE: VST3 uses UTF16 strings with a max length of 128 characters. Cplug the handles UTF16<->UTF8 conversion
//       CLAP has a max length of 256 bytes, AUv2 no limit (mandatory CFString), both are UTF8
CPLUG_API void cplug_getInputBusName(void*, uint32_t idx, char* buf, size_t buflen);
CPLUG_API void cplug_getOutputBusName(void*, uint32_t idx, char* buf, size_t buflen);

CPLUG_API uint32_t cplug_getLatencyInSamples(void*);
CPLUG_API uint32_t cplug_getTailInSamples(void*);

CPLUG_API void cplug_setSampleRateAndBlockSize(void*, double sampleRate, uint32_t maxBlockSize);

enum
{
    CPLUG_EVENT_UNHANDLED_EVENT, // Empty event for unimplemented features. Do nothing
    CPLUG_EVENT_PROCESS_AUDIO,
    CPLUG_EVENT_PARAM_CHANGE_BEGIN,
    CPLUG_EVENT_PARAM_CHANGE_UPDATE,
    CPLUG_EVENT_PARAM_CHANGE_END,
    CPLUG_EVENT_MIDI,
    // CPLUG_EVENT_NOTE_EXPRESSION_VOLUME,
    // CPLUG_EVENT_NOTE_EXPRESSION_PAN,
    CPLUG_EVENT_NOTE_EXPRESSION_TUNING,
    // CPLUG_EVENT_NOTE_EXPRESSION_VIBRATO,
    // CPLUG_EVENT_NOTE_EXPRESSION_EXPRESSION, // similar to an expression pedal (MIDI CC 11), except polyphonic
    // CPLUG_EVENT_NOTE_EXPRESSION_BRIGHTNESS,
};

union CplugEvent
{
    uint32_t type; // CPLUG_EVENT_XXX

    struct
    {
        uint32_t type;
        uint32_t endFrame;
    } processAudio;

    struct
    {
        uint32_t type;
        uint32_t id;
        double   value;
    } parameter;

    struct
    {
        uint32_t type;
        uint32_t frame;
        union
        {
            struct
            {
                uint8_t status;
                uint8_t data1;
                uint8_t data2;
            };
            uint8_t  bytes[4];
            uint32_t bytesAsInt;
        };
    } midi;

    struct
    {
        uint32_t type;
        uint32_t key; // 0-127
        double   value;
    } note_expression;
};

enum
{
    CPLUG_FLAG_TRANSPORT_IS_PLAYING         = 1 << 0,
    CPLUG_FLAG_TRANSPORT_IS_LOOPING         = 1 << 1,
    CPLUG_FLAG_TRANSPORT_IS_RECORDING       = 1 << 2,
    CPLUG_FLAG_TRANSPORT_HAS_BPM            = 1 << 3,
    CPLUG_FLAG_TRANSPORT_HAS_TIME_SIGNATURE = 1 << 4,
    CPLUG_FLAG_TRANSPORT_HAS_PLAYHEAD_BEATS = 1 << 5,
};

struct CplugProcessContext
{
    uint32_t numFrames;

    uint32_t flags; // CPLUG_FLAG_TRANSPORT_XXX
    double   bpm;
    double   playheadBeats;
    double   loopStartBeats;
    double   loopEndBeats;
    uint32_t timeSigNumerator;
    uint32_t timeSigDenominator;

    bool (*enqueueEvent)(CplugProcessContext* ctx, const CplugEvent*, uint32_t frameIdx);
    bool (*dequeueEvent)(CplugProcessContext* ctx, CplugEvent*, uint32_t frameIdx);

    float** (*getAudioInput)(const CplugProcessContext* ctx, uint32_t busIdx);
    float** (*getAudioOutput)(const CplugProcessContext* ctx, uint32_t busIdx);
};

CPLUG_API void cplug_process(void* userPlugin, CplugProcessContext* ctx);

enum
{
    // All formats
    CPLUG_FLAG_PARAMETER_IS_READ_ONLY = 1 << 0,
    CPLUG_FLAG_PARAMETER_IS_BOOL      = 1 << 1,
    // (VST3 | CLAP)
    CPLUG_FLAG_PARAMETER_IS_INTEGER     = 1 << 2,
    CPLUG_FLAG_PARAMETER_IS_HIDDEN      = 1 << 3, // Internal param, hidden from the DAWs GUI
    CPLUG_FLAG_PARAMETER_IS_AUTOMATABLE = 1 << 4, // Parameter is automatable by the host
    CPLUG_FLAG_PARAMETER_IS_BYPASS      = 1 << 5
};

CPLUG_API uint32_t cplug_getNumParameters(void*);
CPLUG_API uint32_t cplug_getParameterID(void*, uint32_t paramIndex);
CPLUG_API uint32_t cplug_getParameterFlags(void*, uint32_t paramId); // CPLUG_FLAG_PARAMETER_XXX

CPLUG_API void cplug_getParameterRange(void*, uint32_t paramId, double* min, double* max);

// NOTE: AUv2 supports a max length of 52 bytes, VST3 128, CLAP 256
CPLUG_API void cplug_getParameterName(void*, uint32_t paramId, char* buf, size_t buflen);

CPLUG_API double cplug_getParameterValue(void*, uint32_t paramId);
CPLUG_API double cplug_getDefaultParameterValue(void*, uint32_t paramId);
// [hopefully audio thread] VST3 & AU only
CPLUG_API void cplug_setParameterValue(void*, uint32_t paramId, double value);
// VST3 only
CPLUG_API double cplug_denormaliseParameterValue(void*, uint32_t paramId, double value);
CPLUG_API double cplug_normaliseParameterValue(void*, uint32_t paramId, double value);

CPLUG_API double cplug_parameterStringToValue(void*, uint32_t paramId, const char*);
CPLUG_API void   cplug_parameterValueToString(void*, uint32_t paramId, char* buf, size_t bufsize, double value);

// Returns -1 on error and 'numBytesToWrite' on success
typedef int64_t (*cplug_writeProc)(const void* stateCtx, void* writePos, size_t numBytesToWrite);
CPLUG_API void cplug_saveState(void* userPlugin, const void* stateCtx, cplug_writeProc writeProc);

// Returns 0 if all bytes are read, -1 on error, and 'maxBytesToRead' when there are remaining bytes to read
typedef int64_t (*cplug_readProc)(const void* stateCtx, void* readPos, size_t maxBytesToRead);
CPLUG_API void cplug_loadState(void* userPlugin, const void* stateCtx, cplug_readProc readProc);

static const uint32_t kAudioUnitProperty_UserPlugin = 'plug';

// NOTE: For AUv2, your pointer MUST be castable to NSView. AUv2 hosts expect an NSView & you simply override methods
// This is the only CPLUG method used in AUv2 builds.
CPLUG_API void* cplug_createGUI(void* userPlugin);
CPLUG_API void  cplug_destroyGUI(void* userGUI);
// If not NULL, set your window/view as a child/subview. If NULL, remove from parent/superview.
// This is a good place to init/deinit your GFX and timer. Be prepared for this to be called multiple times with NULL
CPLUG_API void cplug_setParent(void* userGUI, void* hwnd_or_nsview);
// CLAP only. VST3 simply create/destroy your window.
CPLUG_API void cplug_setVisible(void* userGUI, bool visible);
CPLUG_API void cplug_setScaleFactor(void* userGUI, float scale);
CPLUG_API void cplug_getSize(void* userGUI, uint32_t* width, uint32_t* height);
// Host is trying to resize, but giving you the chance to overwrite their size
CPLUG_API void cplug_checkSize(void* userGUI, uint32_t* width, uint32_t* height);
CPLUG_API bool cplug_setSize(void* userGUI, uint32_t width, uint32_t height);

/*  ██╗   ██╗████████╗██╗██╗     ███████╗
    ██║   ██║╚══██╔══╝██║██║     ██╔════╝
    ██║   ██║   ██║   ██║██║     ███████╗
    ██║   ██║   ██║   ██║██║     ╚════██║
    ╚██████╔╝   ██║   ██║███████╗███████║
     ╚═════╝    ╚═╝   ╚═╝╚══════╝╚══════╝*/

// clang-format off
typedef volatile int cplug_atomic_i32;
#if defined(_MSC_VER) && ! (__clang__)
extern long _InterlockedExchange(long volatile *Target, long Value);
extern long _InterlockedCompareExchange(long volatile *Destination, long ExChange, long Comperand);
extern long _InterlockedExchangeAdd(long volatile *Addend, long Value);
extern long _InterlockedAnd(long volatile *Addend, long Value);
static inline int cplug_atomic_exchange_i32 ( cplug_atomic_i32* ptr, int v) { return _InterlockedExchange       ((volatile long*)ptr, v); }
static inline int cplug_atomic_load_i32(const cplug_atomic_i32* ptr)        { return _InterlockedCompareExchange((volatile long*)ptr, 0, 0); }
static inline int cplug_atomic_fetch_add_i32( cplug_atomic_i32* ptr, int v) { return _InterlockedExchangeAdd    ((volatile long*)ptr, v); }
static inline int cplug_atomic_fetch_and_i32( cplug_atomic_i32* ptr, int v) { return _InterlockedAnd            ((volatile long*)ptr, v); }
#else
static inline int cplug_atomic_exchange_i32 ( cplug_atomic_i32* ptr, int v) { return __atomic_exchange_n(ptr, v, __ATOMIC_SEQ_CST); }
static inline int cplug_atomic_load_i32(const cplug_atomic_i32* ptr)        { return __atomic_load_n    (ptr,    __ATOMIC_SEQ_CST); }
static inline int cplug_atomic_fetch_add_i32( cplug_atomic_i32* ptr, int v) { return __atomic_fetch_add (ptr, v, __ATOMIC_SEQ_CST); }
static inline int cplug_atomic_fetch_and_i32( cplug_atomic_i32* ptr, int v) { return __atomic_fetch_and (ptr, v, __ATOMIC_SEQ_CST); }
#endif

/*  ██████╗ ███████╗██████╗ ██╗   ██╗ ██████╗
    ██╔══██╗██╔════╝██╔══██╗██║   ██║██╔════╝
    ██║  ██║█████╗  ██████╔╝██║   ██║██║  ███╗
    ██║  ██║██╔══╝  ██╔══██╗██║   ██║██║   ██║
    ██████╔╝███████╗██████╔╝╚██████╔╝╚██████╔╝
    ╚═════╝ ╚══════╝╚═════╝  ╚═════╝  ╚═════╝*/

#if !defined(__cplusplus) && !defined(_MSC_VER) && !defined(static_assert)
#define static_assert _Static_assert
#endif

#if defined(__GNUC__) || defined(__clang__)
#define cplug_unlikely(x) __builtin_expect(x, 0)
#else
#define cplug_unlikely(x) x
#endif

#ifndef cplug_log
#ifdef NDEBUG
#define cplug_log(...)
#else
#include <stdarg.h>
#include <stdio.h>
static inline void cplug_printf(const char* const fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    // When debugging in a host, consider adding: freopen(".../Desktop/log.txt", "a", stderr);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");
    va_end(args);
}
#define cplug_log cplug_printf
#endif // NDEBUG
#endif // cplug_log

#ifndef CPLUG_LOG_ASSERT
#ifdef NDEBUG
#define CPLUG_LOG_ASSERT(...)
#else
#define CPLUG_LOG_ASSERT(cond) if (cplug_unlikely(!(cond))) { cplug_log("FAIL ASSERT: " #cond " - %s:%d", __FILE__, __LINE__); }
#endif // NDEBUG
#endif // CPLUG_LOG_ASSERT

#define CPLUG_LOG_ASSERT_RETURN(cond, ret)                                                                             \
    CPLUG_LOG_ASSERT(cond)                                                                                             \
    if (cplug_unlikely(!(cond))) return ret;

// clang-format on

#ifdef __cplusplus
}
#endif

#endif // !CPLUG_H