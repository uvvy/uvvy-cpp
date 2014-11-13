set(SSS_LIBS sss routing krypto opus ${OPENSSL_LIBRARIES})

create_test(file_playback LIBS voicebox ${VOICEBOX_LIBS} arsenal NO_CTEST)
create_test(file_send LIBS voicebox ${VOICEBOX_LIBS} ${SSS_LIBS} arsenal)

create_test(sim_transmit LIBS voicebox ${VOICEBOX_LIBS} ${SSS_LIBS} arsenal)
