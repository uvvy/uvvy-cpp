create_test(link "ssu;support")
create_test(udp_link "ssu;support;${OPENSSL_LIBRARIES}")
create_test(host "ssu;support;${OPENSSL_LIBRARIES}")
create_test(channel "ssu;support")
create_test(stream_user "ssu;support")
create_test(stream_internal "ssu;support")

create_test(run_udp_receiver "ssu;support")

create_test(optional "ssu;support")
create_test(serialization "ssu;support")
