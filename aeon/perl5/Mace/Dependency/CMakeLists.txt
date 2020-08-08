EXECUTE_PROCESS(COMMAND ${PERL_EXECUTABLE} ${compilergen} MaceHeaderDependencyRecDescent.pm MaceGrammarDependencyRecDescent.pm
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                )
