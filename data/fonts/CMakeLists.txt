cmake_minimum_required(VERSION 3.5...3.31)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/LICENSE"
	"${CMAKE_CURRENT_BINARY_DIR}/DejaVu/DejaVu.LICENSE.txt"
	COPYONLY
)

set(wz2100_fonts_FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans-Bold.ttf"
	"${CMAKE_CURRENT_SOURCE_DIR}/DejaVu/DejaVuSans.ttf"
	"${CMAKE_CURRENT_BINARY_DIR}/DejaVu/DejaVu.LICENSE.txt"
)

if(NOT WZ_SKIP_ADDITIONAL_FONTS)
	configure_file(
		"${CMAKE_CURRENT_SOURCE_DIR}/Noto/LICENSE"
		"${CMAKE_CURRENT_BINARY_DIR}/Noto/Noto.LICENSE.txt"
		COPYONLY
	)

	list(APPEND wz2100_fonts_FILES
		"${CMAKE_CURRENT_SOURCE_DIR}/Noto/NotoSansCJK-VF.otf.ttc"
		"${CMAKE_CURRENT_BINARY_DIR}/Noto/Noto.LICENSE.txt"
	)
endif()

set(wz2100_fonts_FILES ${wz2100_fonts_FILES} PARENT_SCOPE)
