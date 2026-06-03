add_executable(ProtocolCodecTest
    protocol/tst_protocol_codec.cpp
)

target_link_libraries(ProtocolCodecTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME ProtocolCodecTest COMMAND ProtocolCodecTest)

add_executable(PacketFramingTest
    network/tst_packet_framing.cpp
)

target_link_libraries(PacketFramingTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME PacketFramingTest COMMAND PacketFramingTest)

add_executable(ConfigStoreTest
    storage/tst_config_store.cpp
)

target_link_libraries(ConfigStoreTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME ConfigStoreTest COMMAND ConfigStoreTest)

add_executable(DeviceManagerTest
    core/tst_device_manager.cpp
)

target_link_libraries(DeviceManagerTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME DeviceManagerTest COMMAND DeviceManagerTest)

add_executable(TransferTaskTest
    core/tst_transfer_task.cpp
)

target_link_libraries(TransferTaskTest PRIVATE
    androp_core
    Qt5::Test
)

add_test(NAME TransferTaskTest COMMAND TransferTaskTest)

add_custom_target(androp_tests
    DEPENDS ProtocolCodecTest PacketFramingTest ConfigStoreTest DeviceManagerTest TransferTaskTest
)
