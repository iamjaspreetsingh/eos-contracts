add_contract(crowdsale crowdsale ${CMAKE_CURRENT_SOURCE_DIR}/src/crowdsale.cpp)
target_include_directories(crowdsale.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)
   

set_target_properties(crowdsale.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
