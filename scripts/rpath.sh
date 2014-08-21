install_name_tool -change /usr/local/opt/openssl/lib/libssl.1.0.0.dylib ./libssl.1.0.0.dylib opus-streaming 
install_name_tool -change /usr/local/opt/openssl/lib/libcrypto.1.0.0.dylib ./libcrypto.1.0.0.dylib opus-streaming 
install_name_tool -change /Users/berkus/Hobby/mettanode/_build_/3rdparty/rtaudio/librtaudio.dylib ./librtaudio.dylib opus-streaming 
install_name_tool -change /usr/local/Cellar/openssl/1.0.1h/lib/libcrypto.1.0.0.dylib ./libcrypto.1.0.0.dylib libssl.1.0.0.dylib 
