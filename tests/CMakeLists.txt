add_executable(ProtocolCodecTest
    protocol/tst_protocol_codec.cpp
)

target_link_libraries(ProtocolCodecTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME ProtocolCodecTest COMMAND ProtocolCodecTest)

add_custom_target(androp_tests
    DEPENDS ProtocolCodecTest
)
