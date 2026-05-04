#ifndef __VERSION_H
#define __VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

#define SW_MAJOR    1
#define SW_MINOR    2
#define SW_PATCH    0

#define STRINGIFY(x)  #x
#define TOSTRING(x)   STRINGIFY(x)

#define SW_VERSION_STR   TOSTRING(SW_MAJOR) "." TOSTRING(SW_MINOR) "." TOSTRING(SW_PATCH)

#define BUILD_DATE    __DATE__
#define BUILD_TIME    __TIME__

#define FW_STRING     "STM32-ENR v" SW_VERSION_STR " (" BUILD_DATE " " BUILD_TIME ")"

#ifdef __cplusplus
}
#endif

#endif
