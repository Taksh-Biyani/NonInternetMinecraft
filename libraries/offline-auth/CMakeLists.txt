cmake_minimum_required(VERSION 3.15)
project(offline-auth Java)
find_package(Java 1.8 REQUIRED COMPONENTS Development Runtime)

include(UseJava)
# Java 8 bytecode, so the agent runs on every Java that Minecraft uses (8, 17, 21, 25).
set(CMAKE_JAVA_COMPILE_FLAGS --release 8 -Xlint:-options)

set(SRC
    org/pineconemc/offlineauth/Agent.java
    org/pineconemc/offlineauth/AuthServer.java
    org/pineconemc/offlineauth/Json.java
    org/pineconemc/offlineauth/Log.java
    org/pineconemc/offlineauth/OfflineProfiles.java
    org/pineconemc/offlineauth/Router.java
    org/pineconemc/offlineauth/SelfTest.java
)

add_jar(pinecone-offline-auth SOURCES ${SRC} MANIFEST ${CMAKE_CURRENT_SOURCE_DIR}/MANIFEST.MF)
install_jar(pinecone-offline-auth "${JARS_DEST_DIR}")

if(BUILD_TESTING)
    get_target_property(_offline_auth_jar pinecone-offline-auth JAR_FILE)
    add_test(NAME OfflineAuthStub COMMAND ${Java_JAVA_EXECUTABLE} -cp "${_offline_auth_jar}" org.pineconemc.offlineauth.SelfTest)
endif()
