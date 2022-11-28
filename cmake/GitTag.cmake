#!/usr/bin/cmake -P

#
# Headless script to generate git versioning with only cmake installed
#

# the output version filename relative to the PWD of the script
set(VERSION_FILENAME "Version.h")

# execute all the things we need
execute_process(COMMAND git describe --tags --always HEAD OUTPUT_VARIABLE GIT_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_COMMIT OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)

file(WRITE ${VERSION_FILENAME} "")
file(APPEND ${VERSION_FILENAME} "#ifndef COLLAB3_VERSION_H\n")
file(APPEND ${VERSION_FILENAME} "#define COLLAB3_VERSION_H\n")
file(APPEND ${VERSION_FILENAME} "//\n")
file(APPEND ${VERSION_FILENAME} "// Do not edit!\n")
file(APPEND ${VERSION_FILENAME} "// This file is auto generated by GitTag.cmake.\n")
file(APPEND ${VERSION_FILENAME} "//\n")
file(APPEND ${VERSION_FILENAME} "\n")
file(APPEND ${VERSION_FILENAME} "#ifndef RC_INVOKED /* Win32 RC cannot understand C++17 :( */\n")
file(APPEND ${VERSION_FILENAME} "namespace collab3::version {\n")

# TODO: Fix this logic so just-tag goes on the v3.x.x branches instead

if("${GIT_BRANCH}" STREQUAL "master")
	# master/stable releases
	file(APPEND ${VERSION_FILENAME} "\tconstexpr static char tag[] = \"${GIT_TAG}\";\n")
else()
	if("${GIT_TAG}" STREQUAL "${GIT_COMMIT}")
		# git describe --always will fallback to shorthand commit if no tags can describe this version
		file(APPEND ${VERSION_FILENAME} "\tconstexpr static char tag[] = \"${GIT_TAG}-${GIT_BRANCH}\";\n")
	else()
		# we actually have a tag, so do this instead
		file(APPEND ${VERSION_FILENAME} "\tconstexpr static char tag[] = \"${GIT_TAG}-${GIT_COMMIT}-${GIT_BRANCH}\";\n")
	endif()
endif()

file(APPEND ${VERSION_FILENAME} "\tconstexpr static char branch[] = \"${GIT_BRANCH}\";")
file(APPEND ${VERSION_FILENAME} "\n}\n")
file(APPEND ${VERSION_FILENAME} "#endif\n")

file(APPEND ${VERSION_FILENAME} "// Legacy macros for stamping Windows binaries\n")
file(APPEND ${VERSION_FILENAME} "// with the Git tag/commit/branch in their version manifest, or basic string literal concatenation.\n")

if("${GIT_BRANCH}" STREQUAL "master")
	file(APPEND ${VERSION_FILENAME} "\t#define COLLAB3_VERSION_TAG \"${GIT_TAG}\"\n")
else()
	if("${GIT_TAG}" STREQUAL "${GIT_COMMIT}")
		# git describe --tags will fallback to shorthand commit if no tags can describe this version
		file(APPEND ${VERSION_FILENAME} "\t#define COLLAB3_VERSION_TAG \"${GIT_TAG}-${GIT_BRANCH}\"\n")
	else()
		# we actually have a tag, so do this instead
		file(APPEND ${VERSION_FILENAME} "\t#define COLLAB3_VERSION_TAG \"${GIT_TAG}-${GIT_COMMIT}-${GIT_BRANCH}\"\n")
	endif()
endif()
file(APPEND ${VERSION_FILENAME} "\t#define COLLAB3_VERSION_BRANCH \"${GIT_BRANCH}\"\n")
file(APPEND ${VERSION_FILENAME} "#endif // COLLAB3_VERSION_H\n")

if("${GIT_TAG}" STREQUAL "${GIT_COMMIT}")
	message(STATUS "Generated ${VERSION_FILENAME} for commit ${GIT_COMMIT} on branch ${GIT_BRANCH}.")
else()
	message(STATUS "Generated ${VERSION_FILENAME} for commit ${GIT_COMMIT} on tag ${GIT_TAG} on branch ${GIT_BRANCH}.")
endif()