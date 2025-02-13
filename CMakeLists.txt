cmake_minimum_required(VERSION 3.28)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if (TEST)
  add_subdirectory(c-testing-and-mocking)

  project(smart-timed-plug-test C)

  add_library(common INTERFACE)

  target_compile_definitions(common INTERFACE
    -D_FORTIFY_SOURCE=2
  )

  target_include_directories(common INTERFACE
    .
    test
  )

  target_compile_options(common INTERFACE
    -g
    -std=c99
    -Wall
    -Werror
    #-Wconversion
    #-Wsign-conversion
    -Wformat-security
    #-pedantic
    -ftrapv
    -fstack-protector-all
    --param ssp-buffer-size=4
    -fstack-clash-protection
  )

  target_link_libraries(common INTERFACE
    c-testing-and-mocking
  )

  add_executable(button-test
    test/avr_mock.c

    test/button_tc.c

    button.c
  )

  target_link_libraries(button-test PUBLIC common)

  add_custom_command(
    TARGET button-test
    POST_BUILD
    COMMAND button-test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
  )

  add_executable(time-test
    test/avr_mock.c
    test/button_mock.c

    test/time_tc.c

    time.c
  )

  target_link_libraries(time-test PUBLIC common)

  add_custom_command(
    TARGET time-test
    POST_BUILD
    COMMAND time-test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
  )

  add_executable(oled-test
    test/avr_mock.c
    test/time_mock.c

    test/oled_common.c

    test/oled_tc.c

    oled_bitmap.c
    oled.c
  )

  target_link_libraries(oled-test PUBLIC common)

  add_custom_command(
    TARGET oled-test
    POST_BUILD
    COMMAND oled-test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
  )

  add_executable(oled-all-test
    test/avr_mock.c
    test/time_mock.c

    test/oled_common.c

    ${CMAKE_CURRENT_LIST_DIR}/test/oled_progress_gen.c
    ${CMAKE_CURRENT_LIST_DIR}/test/oled_time_gen.c

    test/oled-all-test_tc.c

    oled_bitmap.c
    oled.c
  )

  target_link_libraries(oled-all-test PUBLIC common)

  add_custom_command(
    TARGET oled-all-test
    POST_BUILD
    COMMAND oled-all-test
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM
  )

  add_executable(data-gen
    oled_bitmap.c
    test/data_generator.c
  )

  target_link_libraries(data-gen PUBLIC
    common
  )

  add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/test/oled_progress_gen.c
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/test/oled_time_gen.c
    DEPENDS data-gen
    COMMAND data-gen
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    VERBATIM
  )

else()
  set(F_CPU 8000000UL)

  # CPU, you can find the list here:
  # https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
  set(MCU_GCC attiny84a)

  # Part for avrdude
  # https://avrdudes.github.io/avrdude/8.0/avrdude_45.html#List-of-Parts
  set(MCU_DUDE t84a)

  # ISR names:
  # https://www.nongnu.org/avr-libc/user-manual/group__avr__interrupts.html

  # The programmer to use, read avrdude manual for list
  set(PROG_TYPE avrispmkII)

  # AVR Fuses, must be in concordance with your hardware and F_CPU
  # https://www.engbedded.com/fusecalc/
  set(L_FUSE 0xE2)
  set(H_FUSE 0xDF)

  set(CMAKE_SYSTEM_NAME Generic)
  set(CMAKE_SYSTEM_PROCESSOR avr)
  set(CMAKE_CXX_COMPILER avr-g++)
  set(CMAKE_C_COMPILER avr-gcc)
  set(CMAKE_ASM_COMPILER avr-gcc)

  set(AVR_LIBC_PROJECT_PATH ${CMAKE_SOURCE_DIR}/avr-libc)
  set(AVR_LIBC_PATH ${AVR_LIBC_PROJECT_PATH}/install-dir)

  project(smart-timed-plug C)

  add_executable(smart-timed-plug
    avr.c
    button.c
    time.c
    oled.c
    oled_bitmap.c
    main.c

    ${AVR_LIBC_PATH}/avr/include/stdlib.h
  )

  target_compile_definitions(smart-timed-plug PUBLIC
    -DF_CPU=${F_CPU}
  )

  # building avr-libc: https://avrdudes.github.io/avr-libc/avr-libc-user-manual/install_tools.html
  target_include_directories(smart-timed-plug PUBLIC
    ${AVR_LIBC_PATH}/avr/include
    ${AVR_LIBC_PATH}/include
  )

  target_compile_options(smart-timed-plug PUBLIC
    -mmcu=${MCU_GCC}
    -std=c99
    -Os
    -flto
    -Wall
    -Wundef
    -pedantic
    -Werror
    -Wl,--relax,--gc-sections
    -fpack-struct
    -fshort-enums
    -ffunction-sections
    -fdata-sections
  )

  target_link_options(smart-timed-plug PUBLIC
    -mmcu=${MCU_GCC}
    --sysroot=${AVR_LIBC_PATH}/avr/
  )

  set_target_properties(smart-timed-plug PROPERTIES OUTPUT_NAME smart-timed-plug.elf)

  add_custom_command(TARGET smart-timed-plug POST_BUILD
    DEPENDS $<TARGET_FILE:smart-timed-plug>
    COMMAND avr-size -C $<TARGET_FILE:smart-timed-plug> --mcu=${MCU_GCC}
  )

  add_custom_command(
    OUTPUT ${AVR_LIBC_PATH}/avr/include/stdlib.h
    DEPENDS build-avr-libc
  )

  add_custom_command(
    OUTPUT ${AVR_LIBC_PATH}/bin/avr-man # this is the last built file of the avr-libc building process
    COMMAND ./bootstrap && ./configure --prefix=${AVR_LIBC_PATH} --build=x86_64-pc-linux-gnu --host=avr && make -j4 && make install
    WORKING_DIRECTORY ${AVR_LIBC_PROJECT_PATH}
  )
  add_custom_target(build-avr-libc DEPENDS ${AVR_LIBC_PATH}/bin/avr-man)

  add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex smart-timed-plug.elf smart-timed-plug.hex DEPENDS smart-timed-plug)

  add_custom_target(upload avrdude -c ${PROG_TYPE} -p ${MCU_DUDE} -U flash:w:smart-timed-plug.hex DEPENDS hex)

  add_custom_target(upload-eeprom avrdude -c ${PROG_TYPE} -p ${MCU}  -U eeprom:w:eeprom.hex:i)
  add_custom_target(download-eeprom avrdude -c ${PROG_TYPE} -p ${MCU}  -U eeprom:r:readback.hex:i)

  add_custom_target(fuses avrdude -c ${PROG_TYPE} -p ${MCU_DUDE} -U lfuse:w:${L_FUSE}:m -U hfuse:w:${H_FUSE}:m)

endif()
