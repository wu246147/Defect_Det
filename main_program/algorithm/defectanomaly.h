#ifndef DEFECTANOMALY_H
#define DEFECTANOMALY_H

#include "dinomalyonnx.h"
#include "yolodetonnx.h"
#include <vector>

namespace DefectAnomaly
{
/**
 * @brief 按指定裁块大小和滑动步长完整覆盖 ROI，末块贴齐边界。
 * @param size 裁剪后的 ROI 宽高，单位为像素，必须均为正数；短边不足 tileSize 时保持原尺寸，不补边。
 * @param tileSize 正整数裁块边长上限，单位为像素，默认 640；两轴使用相同上限。
 * @param stride 正整数滑动步长，单位为像素，默认 480，必须不大于 tileSize，防止覆盖间隙。
 * @return 相对于 ROI 的切块矩形，按行排列，末块不重复。
 * @throws std::invalid_argument 尺寸、裁块大小或步长不满足上述约束时抛出。
 * @note 仅分配返回的矩形列表，不修改图像或共享状态，可独立并发调用。
 */
std::vector<cv::Rect> tileRegions(cv::Size size, int tileSize = 640, int stride = 480);

/**
 * @brief 将切块分数按逐像素最大值合并到 ROI 分数图，消除重叠切块的重复结果。
 * @param mergedMap 已分配的单通道浮点 ROI 分数图，调用后就地更新。
 * @param region 切块在 ROI 中的位置，必须完全位于合并图内。
 * @param tileMap 已还原为切块尺寸的归一化分数图；无效输入抛出异常。
 */
void mergeTileMap(cv::Mat& mergedMap, const cv::Rect& region, const cv::Mat& tileMap);

/**
 * @brief 对合并分数图按阈值提取八连通缺陷，输出原有框和最低像素分数。
 * @param anomalyMap 归一化的单通道浮点 ROI 分数图。
 * @param offset ROI 左上角在整图中的坐标，用于恢复显示位置。
 * @param threshold 异常像素分数下限，范围为 [0,1]，仅保留严格大于阈值的像素。
 * @param minimumBoxArea 最小外接框面积，沿用现有面积参数的含义。
 * @return 每个独立缺陷的轴对齐最小外接框、区域最低分数和异常类别 1；无效参数抛出异常。
 */
std::vector<DetectDetResult> extractDefects(const cv::Mat& anomalyMap, cv::Point offset,
                                          double threshold, int minimumBoxArea);

/**
 * @brief 仅对选区内图片分段推理，合并分数并转换为原有缺陷结果。
 * @param model 已成功初始化的 Dinomaly 模型，按顺序执行各切块推理。
 * @param image 原始八位图像，不修改其内容；兼容原有灰度、BGR、BGRA 和双通道输入。
 * @param selection 整图中的检测选区，越界部分裁掉；完全越界或空选区抛出异常。
 * @param threshold 本次检测的像素阈值，调整后立即生效，无需重新加载模型。
 * @param minimumBoxArea 最小缺陷外接框面积。
 * @param tileSize 正整数裁块边长上限，单位为像素，默认 640；短轴不足时保持原尺寸，不改变 ONNX 网络尺寸。
 * @param stride 正整数滑动步长，单位为像素，默认 480，必须不大于 tileSize。
 * @return 所有有效缺陷；推理失败时抛出带模型错误原因的异常，不返回部分结果。
 * @throws std::invalid_argument 图片、选区、阈值或分块参数无效时抛出。
 * @note 分块参数每次调用时生效；调用方应串行使用同一模型，不得并发修改输入像素。
 * 临时图像和结果由局部对象管理，输入图像及模型所有权仍归调用方。
 */
std::vector<DetectDetResult> detect(DinomalyOnnx& model, const cv::Mat& image,
                                  const cv::Rect& selection, double threshold, int minimumBoxArea,
                                  int tileSize = 640, int stride = 480);
}

#endif
