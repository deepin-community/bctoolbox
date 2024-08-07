############################################################################
# CMakeLists.txt
# Copyright (C) 2016  Belledonne Communications, Grenoble France
#
############################################################################
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
############################################################################

set(BCTOOLBOX_C_SOURCE_FILES
	containers/list.c
	logging/logging.c
	parser.c
	utils/port.c
	vconnect.c
	vfs/vfs.c
	vfs/vfs_standard.c
	param_string.c
)

set(BCTOOLBOX_CXX_SOURCE_FILES
	containers/map.cc
	conversion/charconv_encoding.cc
	utils/exception.cc
	utils/regex.cc
	utils/utils.cc
)

set(BCTOOLBOX_PRIVATE_HEADER_FILES
	vfs/vfs_encryption_module.hh
	vfs/vfs_encryption_module_dummy.hh
	vfs/vfs_encryption_module_aes256gcm_sha256.hh
)

if(APPLE)
	set(BCTOOLBOX_OBJC_SOURCE_FILES
		utils/ios_utils.mm
	)
	set(BCTOOLBOX_IOS_OBJC_SOURCE_FILES
		utils/ios_utils_app.mm
	)
endif()
if(WIN32)
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES utils/win_utils.cc)
endif()

if(ANDROID)
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES conversion/charconv_android.cc)
elseif(WIN32)
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES conversion/charconv_windows.cc)
else()
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES conversion/charconv.cc)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	list(APPEND STRICT_OPTIONS_CXX "-x c++")
endif()
if(WIN32)
	list(APPEND STRICT_OPTIONS_CXX "/EHa")
	add_definitions(-EHa)
endif()
if(MBEDTLS_FOUND OR POLARSSL_FOUND)
	list(APPEND BCTOOLBOX_C_SOURCE_FILES crypto/crypto.c)
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES crypto/ecc.cc)
endif()
if(MBEDTLS_FOUND)
	list(APPEND BCTOOLBOX_C_SOURCE_FILES crypto/mbedtls.c)
	list(APPEND BCTOOLBOX_CXX_SOURCE_FILES
		crypto/mbedtls.cc
		vfs/vfs_encrypted.cc
		vfs/vfs_encryption_module_dummy.cc
		vfs/vfs_encryption_module_aes256gcm_sha256.cc)
endif()
if(POLARSSL_FOUND)
	list(APPEND BCTOOLBOX_C_SOURCE_FILES crypto/polarssl.c)
endif()
if(ENABLE_TESTS_COMPONENT)
	set(BCTOOLBOX_C_TESTER_SOURCE_FILES tester.c)
endif()

set(BCTOOLBOX_SOURCE_FILES ${BCTOOLBOX_C_SOURCE_FILES} ${BCTOOLBOX_CXX_SOURCE_FILES} ${BCTOOLBOX_OBJC_SOURCE_FILES})
set(BCTOOLBOX_IOS_SOURCE_FILES ${BCTOOLBOX_IOS_OBJC_SOURCE_FILES})
set(BCTOOLBOX_TESTER_SOURCE_FILES ${BCTOOLBOX_C_TESTER_SOURCE_FILES})

bc_apply_compile_flags(BCTOOLBOX_C_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_C)
bc_apply_compile_flags(BCTOOLBOX_C_TESTER_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_C)
bc_apply_compile_flags(BCTOOLBOX_CXX_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_CXX)
bc_apply_compile_flags(BCTOOLBOX_OBJC_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_OBJC)
bc_apply_compile_flags(BCTOOLBOX_IOS_OBJC_SOURCE_FILES STRICT_OPTIONS_CPP STRICT_OPTIONS_OBJC)

if(ENABLE_STATIC)
	add_library(bctoolbox-static STATIC ${BCTOOLBOX_SOURCE_FILES} ${BCTOOLBOX_HEADER_FILES} ${BCTOOLBOX_PRIVATE_HEADER_FILES})
	target_link_libraries(bctoolbox-static INTERFACE ${CMAKE_THREAD_LIBS_INIT})
	if(APPLE)
		target_link_libraries(bctoolbox-static INTERFACE "-framework Foundation" "-framework AVFoundation")
		target_include_directories(bctoolbox-static SYSTEM PRIVATE ${ICONV_INCLUDE_DIRS})
		target_link_libraries(bctoolbox-static INTERFACE ${ICONV_LIBRARIES})
	endif()
	if(WIN32)
		target_link_libraries(bctoolbox-static INTERFACE "Winmm" "Ws2_32")
	endif()
	set_target_properties(bctoolbox-static PROPERTIES LINKER_LANGUAGE "CXX")
	set_target_properties(bctoolbox-static PROPERTIES OUTPUT_NAME bctoolbox)
	if(HAVE_LIBRT)
		target_link_libraries(bctoolbox-static INTERFACE rt)
	endif()
	if(HAVE_LIBDL)
		target_link_libraries(bctoolbox-static INTERFACE dl)
	endif()
	if(ANDROID)
		target_link_libraries(bctoolbox-static INTERFACE log)
	endif()
	if(ENABLE_TESTS_COMPONENT)
		add_library(bctoolbox-tester-static STATIC ${BCTOOLBOX_TESTER_SOURCE_FILES})
		set_target_properties(bctoolbox-tester-static PROPERTIES OUTPUT_NAME bctoolbox-tester)
		target_link_libraries(bctoolbox-tester-static INTERFACE bctoolbox-static)
	endif()
endif()
if(ENABLE_SHARED)
	add_library(bctoolbox SHARED ${BCTOOLBOX_SOURCE_FILES} ${BCTOOLBOX_HEADER_FILES} ${BCTOOLBOX_PRIVATE_HEADER_FILES})
	target_link_libraries(bctoolbox PRIVATE ${CMAKE_THREAD_LIBS_INIT})
	if(APPLE)
		if(IOS)
			set(MIN_OS ${LINPHONE_IOS_DEPLOYMENT_TARGET})
		else()
			set(MIN_OS ${CMAKE_OSX_DEPLOYMENT_TARGET})
		endif()
		set_target_properties(bctoolbox PROPERTIES
			FRAMEWORK TRUE
			MACOSX_FRAMEWORK_IDENTIFIER org.linphone.bctoolbox
			MACOSX_FRAMEWORK_INFO_PLIST "${PROJECT_SOURCE_DIR}/build/osx/Info.plist.in"
			PUBLIC_HEADER "${BCTOOLBOX_HEADER_FILES}"
		)
	endif()
	if(APPLE)
		if (IOS)
			target_link_libraries(bctoolbox PRIVATE "-framework UIKit" "-framework Foundation" "-framework AVFoundation")
		else()
			target_link_libraries(bctoolbox PRIVATE "-framework Foundation" "-framework AVFoundation")
		endif()
		target_include_directories(bctoolbox SYSTEM PRIVATE ${ICONV_INCLUDE_DIRS})
		target_link_libraries(bctoolbox PRIVATE ${ICONV_LIBRARIES})
	endif()
	if(WIN32)
		set_target_properties(bctoolbox PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
		if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
			target_link_libraries(bctoolbox PRIVATE "Winmm" "Ws2_32")
		endif()
		target_compile_definitions(bctoolbox PRIVATE "-DBCTBX_EXPORTS")
		if(CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")#Fix error on 'vccorlib_lib_should_be_specified_before_msvcrt_lib_to_linker''
			get_target_property(BC_LINK_FLAGS bctoolbox LINK_FLAGS)
			if(NOT BC_LINK_FLAGS)
				set(BC_LINK_FLAGS "")
			endif()
			if(CMAKE_BUILD_TYPE STREQUAL "Debug")
				list(APPEND BC_LINK_FLAGS "/nodefaultlib:vccorlibd /nodefaultlib:msvcrtd vccorlibd.lib msvcrtd.lib")
			else()
				list(APPEND BC_LINK_FLAGS "/nodefaultlib:vccorlib /nodefaultlib:msvcrt vccorlib.lib msvcrt.lib")
			endif()
			set_target_properties(bctoolbox PROPERTIES VS_WINRT_EXTENSIONS TRUE)
			add_definitions(-ZW -EHsc)
			set_target_properties(bctoolbox PROPERTIES LINK_FLAGS "${BC_LINK_FLAGS}")
		endif()
	endif()
	set_target_properties(bctoolbox PROPERTIES LINKER_LANGUAGE "CXX")
	if(NOT ANDROID)
		# Do not version shared library on Android
		set_target_properties(bctoolbox PROPERTIES SOVERSION ${BCTOOLBOX_SO_VERSION})
	endif()
	if(MSVC)
		if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
			install(FILES $<TARGET_PDB_FILE:bctoolbox>
				DESTINATION ${CMAKE_INSTALL_BINDIR}
				PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			)
		endif()
	endif()
	if(HAVE_LIBRT)
		target_link_libraries(bctoolbox PRIVATE rt)
	endif()
	if(HAVE_LIBDL)
		target_link_libraries(bctoolbox PRIVATE dl)
	endif()
	if(ANDROID)
		target_link_libraries(bctoolbox PRIVATE log)
	endif()
	if(ENABLE_TESTS_COMPONENT)
		add_library(bctoolbox-tester SHARED ${BCTOOLBOX_TESTER_SOURCE_FILES})
		set_target_properties(bctoolbox-tester PROPERTIES LINKER_LANGUAGE "CXX")
		target_link_libraries(bctoolbox-tester PUBLIC bctoolbox PRIVATE ${BCUNIT_TARGETNAME})
		if(WIN32)
			target_compile_definitions(bctoolbox-tester PRIVATE "-DBCTBX_EXPORTS")
		endif()
		if(NOT ANDROID)
			# Do not version shared library on Android
			set_target_properties(bctoolbox-tester PROPERTIES SOVERSION ${BCTOOLBOXTESTER_SO_VERSION})
		endif()
		if(MSVC)
			if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
				install(FILES $<TARGET_PDB_FILE:bctoolbox-tester>
					DESTINATION ${CMAKE_INSTALL_BINDIR}
					PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
				)
			endif()
		endif()
	endif()
endif()

if(ENABLE_STATIC)
	target_include_directories(bctoolbox-static PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)
endif()

if(ENABLE_SHARED)
	target_include_directories(bctoolbox PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
	)
endif()

if(APPLE)
	if(IOS)
		add_library(bctoolbox-ios SHARED ${BCTOOLBOX_IOS_SOURCE_FILES})
		target_link_libraries(bctoolbox-ios PRIVATE bctoolbox)
		set_target_properties(bctoolbox-ios PROPERTIES
			FRAMEWORK TRUE
			MACOSX_FRAMEWORK_IDENTIFIER org.linphone.bctoolbox-ios
			MACOSX_FRAMEWORK_INFO_PLIST "${PROJECT_SOURCE_DIR}/build/osx/Info.plist.in"
			LINK_FLAGS "-framework UIKit -framework Foundation -framework CoreFoundation"
		)
	endif()
endif()

if(MBEDTLS_FOUND)
	if(ENABLE_STATIC)
		target_link_libraries(bctoolbox-static INTERFACE ${MBEDTLS_TARGETS})
	endif()
	if(ENABLE_SHARED)
		target_link_libraries(bctoolbox PRIVATE ${MBEDTLS_TARGETS})
	endif()
endif()
if(POLARSSL_FOUND)
	if(ENABLE_STATIC)
		target_include_directories(bctoolbox-static SYSTEM PRIVATE ${POLARSSL_INCLUDE_DIRS})
		target_link_libraries(bctoolbox-static INTERFACE ${POLARSSL_LIBRARIES})
	endif()
	if(ENABLE_SHARED)
		target_include_directories(bctoolbox PRIVATE ${POLARSSL_INCLUDE_DIRS})
		target_link_libraries(bctoolbox PRIVATE ${POLARSSL_LIBRARIES})
	endif()
endif()
if(DECAF_FOUND)
	if(ENABLE_STATIC)
		target_include_directories(bctoolbox-static PRIVATE ${DECAF_INCLUDE_DIRS})
		target_link_libraries(bctoolbox-static PUBLIC decaf-static)
	endif()
	if(ENABLE_SHARED)
		target_include_directories(bctoolbox PRIVATE ${DECAF_INCLUDE_DIRS})
		target_link_libraries(bctoolbox PRIVATE ${DECAF_TARGETNAME})
	endif()
endif()

if(ENABLE_TESTS_COMPONENT)
	if(ENABLE_STATIC)
		target_link_libraries(bctoolbox-tester-static PUBLIC bctoolbox-static ${BCUNIT_TARGETNAME})
	endif()
	if(ENABLE_SHARED)
		if(APPLE)
			set_target_properties(bctoolbox-tester PROPERTIES
				FRAMEWORK TRUE
				MACOSX_FRAMEWORK_IDENTIFIER com.belledonne-communications.bctoolbox-tester
				MACOSX_FRAMEWORK_INFO_PLIST "${PROJECT_SOURCE_DIR}/build/osx/Info.plist.in"
				PUBLIC_HEADER "${BCTOOLBOX_HEADER_FILES}"
			)
		endif()
	endif()
endif()


if(ENABLE_STATIC)
	install(TARGETS bctoolbox-static EXPORT ${EXPORT_TARGETS_NAME}Targets
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		COMPONENT core
	)
	if(ENABLE_TESTS_COMPONENT)
		install(TARGETS bctoolbox-tester-static EXPORT ${EXPORT_TARGETS_NAME}Targets
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			COMPONENT tester
		)
	endif()
endif()
if(ENABLE_SHARED)
	install(TARGETS bctoolbox EXPORT ${EXPORT_TARGETS_NAME}Targets
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		FRAMEWORK DESTINATION Frameworks
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
		COMPONENT core
	)
	if(ENABLE_TESTS_COMPONENT)
		install(TARGETS bctoolbox-tester EXPORT ${EXPORT_TARGETS_NAME}Targets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			FRAMEWORK DESTINATION Frameworks
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			COMPONENT tester
		)
	endif()
endif()

if(APPLE)
	if(IOS)
		install(TARGETS bctoolbox-ios EXPORT ${EXPORT_TARGETS_NAME}Targets
			RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
			LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
			ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
			FRAMEWORK DESTINATION Frameworks
			PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
			COMPONENT core
		)
	endif()
endif()

if(MSVC)
	# Disable "was deprecated" warnings of windows compiler (typically using strcpy_s instead of strcpy and stupid things like this).
	if(ENABLE_SHARED)
		target_compile_options(bctoolbox PRIVATE "/wd4996")
	endif()
	if(ENABLE_STATIC)
		target_compile_options(bctoolbox-static PRIVATE "/wd4996")
	endif()
	if(ENABLE_TESTS_COMPONENT)
		target_compile_options(bctoolbox-tester PRIVATE "/wd4996")
	endif()
endif()
