set(SSU_LIBS ssu routing krypto opus ${OPENSSL_LIBRARIES})

create_test(file_playback LIBS voicebox ${VOICEBOX_LIBS} support NO_CTEST)
create_test(file_send LIBS voicebox ${VOICEBOX_LIBS} ${SSU_LIBS} support)

create_test(sim_transmit LIBS voicebox ${VOICEBOX_LIBS} ${SSU_LIBS} support)
