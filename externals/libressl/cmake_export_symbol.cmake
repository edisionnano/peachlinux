macro(export_symbol TARGET SYMBOLS_PATH)

	set(FLAG "")
	get_filename_component(FILENAME ${SYMBOLS_PATH} NAME)

	if(WIN32)
		string(REPLACE ".sym" ".def" DEF_FILENAME ${FILENAME})
		file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${DEF_FILENAME}" DEF_PATH)
		file(WRITE ${DEF_PATH} "EXPORTS\n")
		file(READ ${SYMBOLS_PATH} SYMBOLS)
		file(APPEND ${DEF_PATH} "${SYMBOLS}")
		target_sources(${TARGET} PRIVATE ${DEF_PATH})

	elseif(APPLE)
		file(READ ${SYMBOLS_PATH} SYMBOLS)
		string(REGEX REPLACE "\n$" "" SYMBOLS ${SYMBOLS})
		string(REPLACE "\n" "\n_" SYMBOLS ${SYMBOLS})
		string(REGEX REPLACE "(.)$" "\\1\\n" SYMBOLS ${SYMBOLS})
		string(REPLACE ".sym" ".exp" EXP_FILENAME ${FILENAME})
		file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${EXP_FILENAME}" EXP_PATH)
		file(WRITE ${EXP_PATH} "_${SYMBOLS}")
		set(FLAG "-exported_symbols_list ${EXP_PATH}")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS ${FLAG})

	elseif(CMAKE_SYSTEM_NAME MATCHES "HP-UX")
		file(READ ${SYMBOLS_PATH} SYMBOLS)
		string(REGEX REPLACE "\n$" "" SYMBOLS ${SYMBOLS})
		string(REPLACE "\n" "\n+e " SYMBOLS ${SYMBOLS})
		string(REPLACE ".sym" ".opt" OPT_FILENAME ${FILENAME})
		file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${OPT_FILENAME}" OPT_PATH)
		file(WRITE ${OPT_PATH} "+e ${SYMBOLS}")
		set(FLAG "-Wl,-c,${OPT_PATH}")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS ${FLAG})

	elseif(CMAKE_SYSTEM_NAME MATCHES "SunOS")
		file(READ ${SYMBOLS_PATH} SYMBOLS)
		string(REPLACE "\n" ";\n" SYMBOLS ${SYMBOLS})
		string(REPLACE ".sym" ".ver" VER_FILENAME ${FILENAME})
		file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${VER_FILENAME}" VER_PATH)
		file(WRITE ${VER_PATH}
			"{\nglobal:\n${SYMBOLS}\nlocal:\n*;\n};\n")
		set(FLAG "-Wl,-M${VER_PATH}")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS ${FLAG})

	elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
		file(READ ${SYMBOLS_PATH} SYMBOLS)
		string(REPLACE "\n" ";\n" SYMBOLS ${SYMBOLS})
		string(REPLACE ".sym" ".ver" VER_FILENAME ${FILENAME})
		file(TO_CMAKE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${VER_FILENAME}" VER_PATH)
		file(WRITE ${VER_PATH}
			"{\nglobal:\n${SYMBOLS}\nlocal:\n*;\n};\n")
		set(FLAG "-Wl,--version-script,\"${VER_PATH}\"")
		set_target_properties(${TARGET} PROPERTIES LINK_FLAGS ${FLAG})
	endif()

endmacro()
