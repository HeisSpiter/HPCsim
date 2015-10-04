add_library(PiReduce SHARED ../Pi/pi.c reduce.c)
add_definitions(-DBUILD_WITH_REDUCE)
