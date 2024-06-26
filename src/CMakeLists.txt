#
# Synthetico - Pure-past LTL synthesizer based on BLACK
#
# (C) 2023 Nicola Gigante
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

set (
  LIB_SRC
  src/main.cpp 
  src/common.cpp
  src/spec.cpp
  src/automata.cpp
  src/automatabdd.cpp
  src/varmgr.cpp
  src/qbf.cpp
  src/backend.cpp
  src/random.cpp
  src/game/qbf.cpp
  src/game/bdd.cpp
  src/transducer.cpp
  src/quantification.cpp
)

add_executable (synthetico ${LIB_SRC})

target_link_libraries(synthetico PUBLIC black::black ${CUDD_LIBRARIES})

target_include_directories(synthetico PUBLIC  
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${CUDD_INCLUDE_DIRS}
)

set_property(TARGET synthetico PROPERTY OUTPUT_NAME synth)
set_property(TARGET synthetico PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

target_compile_features(synthetico PUBLIC cxx_std_20)
target_enable_warnings(synthetico)