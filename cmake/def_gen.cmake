file(STRINGS "${MAP}" MAP_LINES REGEX "^[ \t]+[A-Za-z_][A-Za-z0-9_]*;[ \t]*$")

set(MAP_SYMS "")
foreach(MAP_LINE ${MAP_LINES})
    string(REGEX REPLACE "^[ \t]+([A-Za-z_][A-Za-z0-9_]*);[ \t]*$" "\\1" MAP_SYM "${MAP_LINE}")
    list(APPEND MAP_SYMS "${MAP_SYM}")
endforeach()

execute_process(
    COMMAND "${NM}" -g --defined-only "${ARCHIVE}"
    OUTPUT_VARIABLE NM_OUT
    RESULT_VARIABLE NM_RES
)
if(NOT NM_RES EQUAL 0)
    message(FATAL_ERROR "def_gen: nm failed; archive: ${ARCHIVE}; res: ${NM_RES}")
endif()

string(REPLACE "\n" ";" NM_LINES "${NM_OUT}")

set(DEF_SYMS "")
set(CONTENT "EXPORTS\n")
foreach(NM_LINE ${NM_LINES})
    if(NOT NM_LINE MATCHES "^[0-9A-Fa-f]+ ([A-Za-z]) ([A-Za-z_][A-Za-z0-9_]*)$")
        continue()
    endif()
    set(NM_TYPE "${CMAKE_MATCH_1}")
    set(NM_SYM "${CMAKE_MATCH_2}")
    if(NOT NM_SYM IN_LIST MAP_SYMS OR NM_SYM IN_LIST DEF_SYMS)
        continue()
    endif()
    if(NM_TYPE STREQUAL "T")
        list(APPEND DEF_SYMS "${NM_SYM}")
        string(APPEND CONTENT "${NM_SYM}\n")
    elseif(NM_TYPE MATCHES "^[DBRS]$")
        list(APPEND DEF_SYMS "${NM_SYM}")
        string(APPEND CONTENT "${NM_SYM} DATA\n")
    endif()
endforeach()

file(WRITE "${OUTPUT}" "${CONTENT}")
