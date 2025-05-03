
file(GLOB_RECURSE SRC_LUA
        src/*.h src/*.c src/*.cpp src/*.hpp)

add_library(lua5.3.6 
	${SRC_LUA}
	)

##

file(GLOB_RECURSE SRC_LUA_BIND
        bind/*.h
        bind/*.c
        bind/*.cpp
        bind/*.hpp)

add_executable(luabind
    ${SRC_LUA_BIND}
    )
#target_compile_definitions(luabind PUBLIC BIND11_CPP14)
