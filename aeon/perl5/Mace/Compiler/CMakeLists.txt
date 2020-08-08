EXECUTE_PROCESS(COMMAND ${PERL_EXECUTABLE} ${variantgen} VariantRecDescent.pm
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                )

SET(PARSERS MaceHeaderRecDescent.pm MaceRecDescent.pm ClassParserRecDescent.pm VariantRecDescent.pm)
SET(MaceHeaderRecDescent.pm_dep ${Mace_Compiler_MHGrammar_dep})
SET(MaceRecDescent.pm_dep ${Mace_Compiler_MaceGrammar_dep})
SET(ClassParserRecDescent.pm_dep ${Mace_Compiler_ClassGrammar_dep})
SET(VariantRecDescent.pm_dep ${Mace_Compiler_VariantGrammar_dep})

FOREACH(PARSER ${PARSERS})
  ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${PARSER}
                      COMMAND ${PERL_EXECUTABLE} ${compilergen} ${PARSER}
                      DEPENDS ${${PARSER}_dep} ${compilergen_dep}
                      )
  SET(PARSERPATHS ${PARSERPATHS} ${CMAKE_CURRENT_BINARY_DIR}/${PARSER})
ENDFOREACH(PARSER)

ADD_CUSTOM_TARGET(parsers ALL DEPENDS ${PARSERPATHS})
