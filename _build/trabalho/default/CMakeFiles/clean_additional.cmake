# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "/home/renan/programs/teste/out/trabalho/default.cmf"
  "/home/renan/programs/teste/out/trabalho/default.hex"
  "/home/renan/programs/teste/out/trabalho/default.hxl"
  "/home/renan/programs/teste/out/trabalho/default.mum"
  "/home/renan/programs/teste/out/trabalho/default.o"
  "/home/renan/programs/teste/out/trabalho/default.sdb"
  "/home/renan/programs/teste/out/trabalho/default.sym"
  )
endif()
