cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS on)

if (TEST)
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

  add_executable(button-test
    test/framework.c
    test/mock.c

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
    test/framework.c
    test/mock.c

    test/avr_mock.c
    test/button_mock.c

    #test/clock_common.c

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
    test/framework.c
    test/mock.c

    test/avr_mock.c
    test/time_mock.c

    test/oled_common.c

    test/oled_tc.c

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
    test/framework.c
    test/mock.c

    test/avr_mock.c
    test/time_mock.c

    test/oled_common.c

    ${CMAKE_CURRENT_LIST_DIR}/test/oled_progress_gen.c
    ${CMAKE_CURRENT_LIST_DIR}/test/oled_time_gen.c

    test/oled-all-test_tc.c

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
  # ATmega88PA
  # ATmega328P
  # ATmega8
  set(MCU_GCC atmega8)
  set(MCU_DUDE m8)
  # The programmer to use, read avrdude manual for list
  set(PROG_TYPE avrispmkII)

  # AVR Fuses, must be in concordance with your hardware and F_CPU
  # https://www.engbedded.com/fusecalc/
  set(L_FUSE 0x62)
  set(H_FUSE 0xDF)

  set(CMAKE_SYSTEM_NAME Generic)
  set(CMAKE_SYSTEM_PROCESSOR avr)
  set(CMAKE_CXX_COMPILER avr-g++)
  set(CMAKE_C_COMPILER avr-gcc)
  set(CMAKE_ASM_COMPILER avr-gcc)

  project(smart-timed-plug C)

  add_executable(smart-timed-plug
    avr.c
    button.c
    time.c
    oled.c
    oled_bitmap.c
    main.c
  )

  target_include_directories(smart-timed-plug PUBLIC
    /usr/lib/avr/include
  )

  target_compile_options(smart-timed-plug PUBLIC
    -mmcu=${MCU_GCC}
    -std=c99
    -O1
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

  target_compile_definitions(smart-timed-plug PUBLIC
    -DF_CPU=${F_CPU}
  )

  target_link_options(smart-timed-plug PUBLIC -mmcu=${MCU_GCC})

  # TODO: is this needed?
  #target_link_libraries(smart-timed-plug PUBLIC -lm)

  # Rename the output to .elf as we will create multiple files
  set_target_properties(smart-timed-plug PROPERTIES OUTPUT_NAME smart-timed-plug.elf)

  add_custom_command(TARGET smart-timed-plug POST_BUILD
    DEPENDS $<TARGET_FILE:smart-timed-plug>
    COMMAND avr-size -C $<TARGET_FILE:smart-timed-plug> --mcu=${MCU_GCC}
  )

  # Transform binary into hex file, we ignore the eeprom segments in the step
  add_custom_target(hex ALL avr-objcopy -R .eeprom -O ihex smart-timed-plug.elf smart-timed-plug.hex DEPENDS smart-timed-plug)
  # Transform binary into hex file, this is the eeprom part (empty if you don't
  # use eeprom static variables)
  add_custom_target(eeprom avr-objcopy -j .eeprom  --set-section-flags=.eeprom="alloc,load"  --change-section-lma .eeprom=0 -O ihex ${PRODUCT_NAME}.elf ${PRODUCT_NAME}.eep DEPENDS strip)

  # Upload the firmware with avrdude
  add_custom_target(upload avrdude -c ${PROG_TYPE} -p ${MCU_DUDE} -U flash:w:smart-timed-plug.hex DEPENDS hex)

  # Upload the eeprom with avrdude
  add_custom_target(upload_eeprom avrdude -c ${PROG_TYPE} -p ${MCU}  -U eeprom:w:${PRODUCT_NAME}.eep DEPENDS eeprom)

  # Burn fuses
  # -U lfuse:w:0xfd:m -U hfuse:w:0xd9:m
  add_custom_target(fuses avrdude -c ${PROG_TYPE} -p ${MCU_DUDE} -U lfuse:w:${L_FUSE}:m -U hfuse:w:${H_FUSE}:m)

  # Clean extra files
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${PRODUCT_NAME}.hex;${PRODUCT_NAME}.eeprom;${PRODUCT_NAME}.lst")
endif()
