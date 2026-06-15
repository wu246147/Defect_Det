cmake_minimum_required(VERSION 3.16)

project(Defect_Det VERSION 1.0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


#qt
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
set(Qt5_DIR   "/home/dell/Qt5.14.2/5.14.2/gcc_64/lib/cmake/Qt5")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
endif()
find_package(Qt5 COMPONENTS Widgets Xml Concurrent Network SerialBus SerialPort REQUIRED)


#统一打包
set(LIB_DIR "${PROJECT_SOURCE_DIR}/../lib/x64-win64")

# -------------- ONNX Runtime --------------
# 路径指向你解压的目录
set(ORT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/onnxruntime)
set(ORT_INCLUDE ${ORT_ROOT}/include)
message(ORT_INCLUDE:${ORT_INCLUDE})
include_directories(${ORT_INCLUDE})

if(WIN32)
    set(ORT_LIB ${ORT_ROOT}/lib/onnxruntime.lib)
    # # DLL 需要拷贝到可执行文件同目录
    # add_custom_command(TARGET opencv_onnxruntime POST_BUILD
    #     COMMAND ${CMAKE_COMMAND} -E copy_if_different
    #     ${ORT_ROOT}/lib/onnxruntime.dll
    #     $<TARGET_FILE_DIR:opencv_onnxruntime>)
else()
    set(ORT_LIB ${ORT_ROOT}/lib/libonnxruntime.so)
endif()

#mvs
include_directories(${PROJECT_SOURCE_DIR}/../include/mvs)

link_libraries(${PROJECT_SOURCE_DIR}/../lib/MvCameraControl.lib)


#boost
include_directories(${PROJECT_SOURCE_DIR}/../include/boost-1_60)
# 使用file(GLOB)获取所有的.lib文件
file(GLOB BOOST_LIBS "${LIB_DIR}/boost/*.lib")

#opencv
include_directories(${PROJECT_SOURCE_DIR}/../include)
file(GLOB OPENCV_LIBS "${LIB_DIR}/opencv/*.lib")

#log display alg
include_directories(${PROJECT_SOURCE_DIR}/../include/algorithm)
file(GLOB ALGORITHM_LIBS "${LIB_DIR}/algorithm/*.lib")

#s7
# include_directories(${PROJECT_SOURCE_DIR}/../include/S7)
file(GLOB S7_LIBS "${LIB_DIR}/S7/*.lib")

add_executable(Defect_Det
    main.cpp
    ./algorithm/qtstreambuf.cpp
    ./algorithm/qtstreambuf.h
    dialoglinecam.h
    dialoglinecam.cpp
    dialoglinecam.ui
    dialogsimulationset.h
    dialogsimulationset.cpp
    dialogsimulationset.ui
    dialogoutputset.h
    dialogoutputset.cpp
    dialogoutputset.ui
    dialogtoolpara.h
    dialogtoolpara.cpp
    dialogtoolpara.ui
    mainwindow.h
    mainwindow.cpp
    mainwindow.ui
    jobmanager.h
    jobmanager.cpp
    jobworker.h
    jobworker.cpp
    MvCamera.h
    MvCamera.cpp
    controls/calipertoolpara.h
    controls/calipertoolpara.cpp
    controls/calipertoolpara.ui
    Image.qrc
    Font.qrc
    tool/cam.h
    tool/cam.cpp
    forminterface.h
    forminterface.cpp
    forminterface.ui
    SharePara.h
    SharePara.cpp
    tool/communication.h
    tool/communication.cpp
    dialogcalibration.cpp dialogcalibration.h dialogcalibration.ui
    logo.rc
    tool/communicationmodbus.h
    tool/communicationmodbus.cpp
    dialogrobotacq.ui
    dialogrobotacq.cpp
    dialogrobotacq.h
    tool/projecttool.cpp
    tool/projecttool.h
    tool/communicationcom.cpp
    tool/communicationcom.h
    tool/toyopuc.cpp
    tool/toyopuc.h
    tool/toyopuc_caller.cpp
    tool/toyopuc_caller.h
    tool/IOPara.h

    mytextitem.cpp
    mytextitem.h

    #S7通讯
    tool/plc_siemens.cpp
    tool/plc_siemens.h
    tool/snap7.cpp
    tool/snap7.h
  )

message(ALGORITHM_LIBS:${ALGORITHM_LIBS})
message(OPENCV_LIBS:${OPENCV_LIBS})


target_link_libraries(Defect_Det Qt5::Widgets Qt5::Xml Qt5::Concurrent Qt5::Network Qt5::SerialBus Qt5::SerialPort)
target_link_libraries(Defect_Det ${BOOST_LIBS} ${OPENCV_LIBS} ${ALGORITHM_LIBS} ${S7_LIBS} ${ORT_LIB})

# 文件打包
file(COPY ${PROJECT_SOURCE_DIR}/qss DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/img DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/algorithm DESTINATION ${PROJECT_BINARY_DIR})
