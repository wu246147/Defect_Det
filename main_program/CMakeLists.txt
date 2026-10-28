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

# 使用新库的源码编译，保证与主程序使用相同的 OpenCV、ONNX Runtime 和 C++ 运行库。
set(ANOMALY_ONNX_ROOT "E:/code/study/qt/opencv_onnxruntime" CACHE PATH "异常检测库源码目录")
if(NOT EXISTS "${ANOMALY_ONNX_ROOT}/dinomalyonnx.h")
    message(FATAL_ERROR "未找到异常检测库，请设置 ANOMALY_ONNX_ROOT 为 opencv_onnxruntime 目录。")
endif()
add_library(opencv_onnx_anomaly_lib SHARED
    "${ANOMALY_ONNX_ROOT}/anomalyonnx.cpp"
    "${ANOMALY_ONNX_ROOT}/anomalymodels.cpp"
)
target_include_directories(opencv_onnx_anomaly_lib PUBLIC "${ANOMALY_ONNX_ROOT}")
target_compile_definitions(opencv_onnx_anomaly_lib PRIVATE ANOMALYONNX_EXPORTS NOMINMAX)
set_target_properties(opencv_onnx_anomaly_lib PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_link_libraries(opencv_onnx_anomaly_lib PRIVATE ${OPENCV_LIBS} ${ORT_LIB})

add_library(defect_anomaly STATIC algorithm/defectanomaly.h algorithm/defectanomaly.cpp)
set_target_properties(defect_anomaly PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(defect_anomaly PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/algorithm")
target_link_libraries(defect_anomaly PUBLIC opencv_onnx_anomaly_lib ${OPENCV_LIBS} ${ORT_LIB})
if(MSVC)
    target_compile_options(opencv_onnx_anomaly_lib PRIVATE /utf-8)
    target_compile_options(defect_anomaly PRIVATE /utf-8)
endif()

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
target_link_libraries(Defect_Det defect_anomaly)

# 模型与运行库随程序部署，运行时使用程序目录下的固定模型路径。
set(DINOMALY_MODEL_FILE "${ANOMALY_ONNX_ROOT}/data/anomaly/dinomaly.onnx" CACHE FILEPATH "已导出的 Dinomaly ONNX 模型")
if(EXISTS "${DINOMALY_MODEL_FILE}")
    add_custom_command(TARGET Defect_Det POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:Defect_Det>/model"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${DINOMALY_MODEL_FILE}"
            "$<TARGET_FILE_DIR:Defect_Det>/model/dinomaly.onnx" VERBATIM)
else()
    message(WARNING "未找到 Dinomaly 模型，请将已导出的模型放到程序目录 model/dinomaly.onnx。")
endif()
if(WIN32)
    # 工程现有链接库包含两个 OpenCV 版本，新库用到的 DNN 符号还依赖 4.8 运行库。
    file(GLOB OPENCV_RUNTIME_DLLS "${LIB_DIR}/opencv/*.dll")
    add_custom_command(TARGET opencv_onnx_anomaly_lib POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${OPENCV_RUNTIME_DLLS}
            "$<TARGET_FILE_DIR:opencv_onnx_anomaly_lib>" COMMAND_EXPAND_LISTS VERBATIM)
    add_custom_command(TARGET Defect_Det POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ORT_ROOT}/lib/onnxruntime.dll"
            "$<TARGET_FILE_DIR:Defect_Det>" VERBATIM)
endif()

option(DEFECT_ANOMALY_BUILD_TESTS "构建异常检测分段、筛选及真实模型验证" OFF)
if(DEFECT_ANOMALY_BUILD_TESTS)
    enable_testing()
    add_executable(defect_anomaly_tests tests/defect_anomaly_tests.cpp)
    set_target_properties(defect_anomaly_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
    target_link_libraries(defect_anomaly_tests PRIVATE defect_anomaly)
    if(MSVC)
        target_compile_options(defect_anomaly_tests PRIVATE /utf-8)
    endif()
    add_test(NAME defect_anomaly_regions COMMAND defect_anomaly_tests)
    if(EXISTS "${DINOMALY_MODEL_FILE}")
        add_test(NAME defect_anomaly_dinomaly COMMAND defect_anomaly_tests "${DINOMALY_MODEL_FILE}"
            "${ANOMALY_ONNX_ROOT}/data/anomaly/sample_anomaly.png")
    endif()
    if(WIN32)
        add_custom_command(TARGET defect_anomaly_tests POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${ORT_ROOT}/lib/onnxruntime.dll"
                "$<TARGET_FILE_DIR:defect_anomaly_tests>" VERBATIM)
    endif()
endif()

# 文件打包
file(COPY ${PROJECT_SOURCE_DIR}/qss DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/img DESTINATION ${PROJECT_BINARY_DIR})
file(COPY ${PROJECT_SOURCE_DIR}/algorithm DESTINATION ${PROJECT_BINARY_DIR})
