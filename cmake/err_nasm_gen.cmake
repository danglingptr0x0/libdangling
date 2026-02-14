file(STRINGS "${INPUT}" LINES REGEX "^#define LDG_[A-Z_]+ [0-9]+$")

set(CONTENT "; err.inc: auto-gen from err.h (do not edit)\n\n")

foreach(LINE ${LINES})
    string(REGEX REPLACE "^#define " "%define " NASM_LINE "${LINE}")
    string(APPEND CONTENT "${NASM_LINE}\n")
endforeach()

file(WRITE "${OUTPUT}" "${CONTENT}")
