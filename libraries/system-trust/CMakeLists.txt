cmake_minimum_required(VERSION 3.15)
project(system-trust Java)
find_package(Java 1.8 REQUIRED COMPONENTS Development Runtime)

include(UseJava)
# Java 8 bytecode, so the agent runs on every Java that Minecraft uses (8, 17, 21, 25).
set(CMAKE_JAVA_COMPILE_FLAGS --release 8 -Xlint:-options)

set(SRC
    org/pineconemc/systemtrust/Agent.java
    org/pineconemc/systemtrust/EitherTrustManager.java
    org/pineconemc/systemtrust/SelfTest.java
)

add_jar(pinecone-system-trust SOURCES ${SRC} MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.MF)
install_jar(pinecone-system-trust "${JARS_DEST_DIR}")

if(BUILD_TESTING)
    get_target_property(_system_trust_jar pinecone-system-trust JAR_FILE)
    add_test(NAME SystemTrust COMMAND ${Java_JAVA_EXECUTABLE} -cp "${_system_trust_jar}" org.pineconemc.systemtrust.SelfTest)
endif()
