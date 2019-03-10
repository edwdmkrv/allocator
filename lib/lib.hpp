#ifndef __LIB_INCLUDED
#define __LIB_INCLUDED

#include <string>

#include <version.hpp>

/* Version stuff
 */
static inline std::string ver() {
	return PROJECT_VERSION;
}

static inline unsigned major() {
	return PROJECT_VERSION_MAJOR;
}

static inline unsigned minor() {
	return PROJECT_VERSION_MINOR;
}

static inline unsigned patch() {
	return PROJECT_VERSION_PATCH;
}


#endif
