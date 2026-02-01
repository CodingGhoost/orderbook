cmake_minimum_required(VERSION 3.25)

project(orderbook LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

# ---- Proto files ----
set(PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(MARKET_DATA_PROTO "${PROTO_DIR}/MarketData.proto")
set(ORDER_ENTRY_PROTO "${PROTO_DIR}/OrderEntry.proto")

# ---- Proto output (generated C++ files) ----
set(GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GEN_DIR}")

# Market Data proto files
set(MD_GEN_PB_CC   "${GEN_DIR}/MarketData.pb.cc")
set(MD_GEN_PB_H    "${GEN_DIR}/MarketData.pb.h")
set(MD_GEN_GRPC_CC "${GEN_DIR}/MarketData.grpc.pb.cc")
set(MD_GEN_GRPC_H  "${GEN_DIR}/MarketData.grpc.pb.h")

# Order Entry proto files
set(OE_GEN_PB_CC   "${GEN_DIR}/OrderEntry.pb.cc")
set(OE_GEN_PB_H    "${GEN_DIR}/OrderEntry.pb.h")
set(OE_GEN_GRPC_CC "${GEN_DIR}/OrderEntry.grpc.pb.cc")
set(OE_GEN_GRPC_H  "${GEN_DIR}/OrderEntry.grpc.pb.h")

# ---- Run protoc for MarketData.proto ----
add_custom_command(
  OUTPUT
    "${MD_GEN_PB_CC}" "${MD_GEN_PB_H}"
    "${MD_GEN_GRPC_CC}" "${MD_GEN_GRPC_H}"
  COMMAND protobuf::protoc
    -I ${PROTO_DIR}
    --cpp_out=${GEN_DIR}
    --grpc_out=${GEN_DIR}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    ${MARKET_DATA_PROTO}
  DEPENDS "${MARKET_DATA_PROTO}"
  COMMENT "Generating C++ sources from MarketData.proto"
)

# ---- Run protoc for OrderEntry.proto ----
add_custom_command(
  OUTPUT
    "${OE_GEN_PB_CC}" "${OE_GEN_PB_H}"
    "${OE_GEN_GRPC_CC}" "${OE_GEN_GRPC_H}"
  COMMAND protobuf::protoc
    -I ${PROTO_DIR}
    --cpp_out=${GEN_DIR}
    --grpc_out=${GEN_DIR}
    --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
    ${ORDER_ENTRY_PROTO}
  DEPENDS "${ORDER_ENTRY_PROTO}"
  COMMENT "Generating C++ sources from OrderEntry.proto"
)

# ---- Proto library ----
add_library(orderbook_proto
    "${MD_GEN_PB_CC}"
    "${MD_GEN_GRPC_CC}"
    "${OE_GEN_PB_CC}"
    "${OE_GEN_GRPC_CC}"
)

target_include_directories(orderbook_proto PUBLIC "${GEN_DIR}")
target_link_libraries(orderbook_proto PUBLIC protobuf::libprotobuf gRPC::grpc++)

# ---- Common library ----
add_library(orderbook_common INTERFACE)
target_include_directories(orderbook_common INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/common")

# ---- Find nlohmann_json ----
find_package(nlohmann_json CONFIG REQUIRED)

# ---- Server executable ----
add_executable(server 
    server/server_main.cpp
    server/MatchingEngine.cpp
    server/Config.cpp
    server/Simulator.cpp
    server/OrderEntryService.cpp
    server/MarketDataService.cpp
    common/OrderBook.cpp
)
target_link_libraries(server PRIVATE 
    orderbook_proto 
    orderbook_common
    nlohmann_json::nlohmann_json
)

# ---- Client executable ----
add_executable(client client/client_main.cpp)
target_link_libraries(client PRIVATE orderbook_proto orderbook_common)
