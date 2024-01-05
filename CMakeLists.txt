cmake_minimum_required(VERSION 3.27)
project(Muve)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Muve
        Aplication.cpp
        NoiseMaker.h
        NoteGenarator.h
        SessionEvaluator.h
        SocketServer.cpp
        SocketServer.h
        StateMachine.cpp
        StateMachine.h
        SynthUtils.h)

target_link_libraries(Muve PRIVATE winmm ws2_32)