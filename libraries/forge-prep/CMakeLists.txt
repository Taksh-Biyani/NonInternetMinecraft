cmake_minimum_required(VERSION 3.15)
project(forge-prep Java)
find_package(Java 1.8 REQUIRED COMPONENTS Development Runtime)

include(UseJava)
# Java 8 bytecode, so it runs on the Java of any Forge version (8, 17, 21, 25).
set(CMAKE_JAVA_COMPILE_FLAGS --release 8 -Xlint:-options)

add_jar(pinecone-forge-prep SOURCES org/pineconemc/forgeprep/Main.java ENTRY_POINT org/pineconemc/forgeprep/Main)
install_jar(pinecone-forge-prep "${JARS_DEST_DIR}")

if(BUILD_TESTING)
    get_target_property(_forge_prep_jar pinecone-forge-prep JAR_FILE)
    add_test(NAME ForgePrepUsage COMMAND ${Java_JAVA_EXECUTABLE} -jar "${_forge_prep_jar}")
    set_tests_properties(ForgePrepUsage PROPERTIES PASS_REGULAR_EXPRESSION "Usage: java -jar pinecone-forge-prep.jar")
endif()
