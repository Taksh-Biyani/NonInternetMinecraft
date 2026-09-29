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

# authlib-injector (https://github.com/yushijinhun/authlib-injector), shipped unmodified under the
# AGPL-3.0 "authlib-injector exception". It redirects Minecraft's auth calls to pinecone-offline-auth.
set(AUTHLIB_INJECTOR_VERSION 1.2.8)
set(AUTHLIB_INJECTOR_SHA256 9c7f4343e6c82034958ffb48c14a2cb0c85928be7283103ce17da00c6d5a7b10)
set(AUTHLIB_INJECTOR_JAR "${CMAKE_CURRENT_BINARY_DIR}/authlib-injector.jar")
if(NOT EXISTS "${AUTHLIB_INJECTOR_JAR}")
    file(DOWNLOAD
        "https://github.com/yushijinhun/authlib-injector/releases/download/v${AUTHLIB_INJECTOR_VERSION}/authlib-injector-${AUTHLIB_INJECTOR_VERSION}.jar"
        "${AUTHLIB_INJECTOR_JAR}"
        EXPECTED_HASH SHA256=${AUTHLIB_INJECTOR_SHA256}
        TLS_VERIFY ON
        STATUS _authlib_injector_status)
    list(GET _authlib_injector_status 0 _authlib_injector_code)
    if(NOT _authlib_injector_code EQUAL 0)
        file(REMOVE "${AUTHLIB_INJECTOR_JAR}")
        message(FATAL_ERROR "Downloading authlib-injector failed: ${_authlib_injector_status}")
    endif()
endif()
install(FILES "${AUTHLIB_INJECTOR_JAR}" "${CMAKE_CURRENT_SOURCE_DIR}/authlib-injector-LICENSE.txt" DESTINATION "${JARS_DEST_DIR}")

if(BUILD_TESTING)
    get_target_property(_offline_auth_jar pinecone-offline-auth JAR_FILE)
    add_test(NAME OfflineAuthStub COMMAND ${Java_JAVA_EXECUTABLE} -cp "${_offline_auth_jar}" org.pineconemc.offlineauth.SelfTest)
endif()
