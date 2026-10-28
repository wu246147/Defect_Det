# Dinomaly 异常检测接入

## 模型和依赖

软件使用 `E:/code/study/qt/opencv_onnxruntime` 中的异常检测库，CMake 从该目录编译
`anomalyonnx.cpp` 和 `anomalymodels.cpp`，链接本软件已有的 OpenCV 库和 ONNX Runtime 1.23。
通过 `ANOMALY_ONNX_ROOT` 可切换库源码目录，不需要编译其 Qt 测试界面。

`DINOMALY_MODEL_FILE` 默认指向库内的 `data/anomaly/dinomaly.onnx`，构建软件时自动复制到
可执行文件所在目录的 `model/dinomaly.onnx`。程序按自身目录定位模型，使用 CPU 推理。
输入尺寸和模型预处理从 ONNX 读取，当前模型的网络输入为 476×476；不在软件中重复归一化。
模型必须是该库支持的、含 `anomalib.*` 元数据及内置预处理和校准后处理的 Dinomaly ONNX。
仅改名的 YOLO 模型或普通第三方 ONNX 不能使用。

发布程序时包含 `opencv_onnx_anomaly_lib.dll`、`onnxruntime.dll`、`model/dinomaly.onnx`
以及原有 Qt、相机等依赖。现有工程同时链接 OpenCV 4.5.5 和 4.8，发布时需同时包含
`opencv_world455.dll` 和 `opencv_world480.dll`。旧目标检测模型不再用于缺陷检测。
构建系统会部署 ONNX Runtime 和 OpenCV DLL；同一构建目录内生成异常检测 DLL。
更换电脑时先配置库路径及模型路径，再构建和部署。

## 检测过程

1. 读取 `defectSelectBox` 与原图的有效交集，仅对此选区裁剪图推理。
2. 按训练数据准备脚本 `toyopuc_prepare.py` 的规则，两轴使用 640 像素块、480 像素步长。
   末块贴齐远端边界，短轴保持原尺寸，不补黑边。403×1908 的 ROI 对应
   `(0,0,403,640)`、`(0,480,403,640)`、`(0,960,403,640)`、`(0,1268,403,640)` 四块。
3. 每块经模型内置预处理缩放到网络尺寸，库将异常浮点分数图还原到切块尺寸。
4. 与训练验证脚本 `test_toyopuc.py` 一致，重叠区域按逐像素最大分数合并。
5. 在合并后的 ROI 浮点图上保留 `score > defectDetThre` 的像素，再提取八连通区域。
   这一步统一处理还原分辨率后的浮点图，不使用库内仅供可视化的二值掩膜，也不按整块分类标签过滤。
6. 每个区域只输出一个轴对齐最小外接矩形，加入 ROI 偏移恢复到原图坐标。
   `DetectDetResult.score` 为该区域所有异常像素的最低分数，不含框内背景或孔洞；
   `classId=1` 表示异常，不再区分原目标检测的缺陷类别。
7. 沿用原来的外接框面积过滤：框面积小于 `defectAreaThre` 时丢弃，等于时保留。
   合并后再提框，避免同一跨块缺陷被重复显示或提前按面积丢弃。

参数界面将“置信度阈值”改为“异常像素阈值”，范围 0～1，越大筛选越严格。
其数值是校准后的异常分数，不是概率。阈值为 1 时不会保留异常像素。
修改阈值在下一次检测立即生效，无需重载模型。原有参数名称和作业文件格式保持不变，
旧作业的数值可继续读取，但目标检测置信度与异常像素分数含义不同，需要用现场样本重新确认阈值。

`defectDetResults` 继续提供原有框、面积和分数显示；有有效缺陷时结果为不合格。
空图、空选区、完全越界选区、未初始化或推理失败同样按不合格处理并记录原因。
不返回失败前的部分缺陷，也不保留上一次结果。模型使用独立智能指针管理，重新初始化及作业释放时释放资源。

## 构建和验证

在已加载 MSVC 2019 开发环境的终端中执行：

```powershell
cmake -S main_program -B main_program/build/Desktop_Qt_5_15_2_MSVC2019_64bit-Release `
  -DDEFECT_ANOMALY_BUILD_TESTS=ON `
  -DANOMALY_ONNX_ROOT=E:/code/study/qt/opencv_onnxruntime
cmake --build main_program/build/Desktop_Qt_5_15_2_MSVC2019_64bit-Release --parallel 4
ctest --test-dir main_program/build/Desktop_Qt_5_15_2_MSVC2019_64bit-Release --output-on-failure
```

测试包括切块覆盖、跨块连通缺陷融合、阈值边界、最低分数、框内孔洞、面积边界、坐标偏移和非法输入。
存在默认模型时额外执行真实模型测试，验证四块推理、输入不变、选区外像素隔离、阈值即时生效及选区裁剪。
也可直接执行 `defect_anomaly_tests.exe <dinomaly.onnx> <测试图片>`，使用现场 ROI 图像验证。
这些验证检查部署与结果转换流程，不代表对新的现场样本重新评估过检出率。
