#include "defectanomaly.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace
{
/**
 * @brief 生成单轴切块起点，覆盖尾部且不重复添加已贴边的末块。
 * @param length 正数轴长。
 * @return 从零开始的递增起点列表，短轴只生成一个起点。
 */
std::vector<int> axisStarts(int length)
{
    constexpr int tileSize = 640;
    constexpr int stride = 480;
    std::vector<int> starts{0};
    const int last = (std::max)(0, length - tileSize);
    while(starts.back() < last)
    {
        // 通过剩余距离限制增量，避免大尺寸下起点相加溢出。
        starts.push_back(starts.back() + (std::min)(stride, last - starts.back()));
    }
    return starts;
}

/**
 * @brief 检查像素阈值及外接框面积参数；无效配置抛出异常，防止误判为合格。
 * @param threshold 有限且位于 [0,1] 的像素阈值。
 * @param minimumBoxArea 非负外接框面积下限。
 */
void validateThresholds(double threshold, int minimumBoxArea)
{
    if(!std::isfinite(threshold) || threshold < 0 || threshold > 1 || minimumBoxArea < 0)
        throw std::invalid_argument("异常像素阈值必须在 [0,1] 内，缺陷框面积阈值必须非负。");
}
}

namespace DefectAnomaly
{
std::vector<cv::Rect> tileRegions(cv::Size size)
{
    if(size.width <= 0 || size.height <= 0)
        throw std::invalid_argument("异常检测 ROI 尺寸必须为正数。");
    const auto xs = axisStarts(size.width);
    const auto ys = axisStarts(size.height);
    std::vector<cv::Rect> regions;
    regions.reserve(xs.size() * ys.size());
    for(int y : ys)
        for(int x : xs)
            regions.emplace_back(x, y, (std::min)(640, size.width), (std::min)(640, size.height));
    return regions;
}

void mergeTileMap(cv::Mat& mergedMap, const cv::Rect& region, const cv::Mat& tileMap)
{
    if(mergedMap.empty() || mergedMap.dims != 2 || mergedMap.type() != CV_32FC1 ||
       region.x < 0 || region.y < 0 || region.width <= 0 || region.height <= 0 ||
       static_cast<int64_t>(region.x) + region.width > mergedMap.cols ||
       static_cast<int64_t>(region.y) + region.height > mergedMap.rows ||
       tileMap.type() != CV_32FC1 || tileMap.size() != region.size() ||
       !cv::checkRange(tileMap, true, nullptr, 0.0, 1.000001))
        throw std::invalid_argument("异常检测切块分数图尺寸、类型或数值无效。");
    cv::Mat target = mergedMap(region);
    (cv::max)(target, tileMap, target);
}

std::vector<DetectDetResult> extractDefects(const cv::Mat& anomalyMap, cv::Point offset,
                                          double threshold, int minimumBoxArea)
{
    validateThresholds(threshold, minimumBoxArea);
    if(anomalyMap.empty() || anomalyMap.dims != 2 || anomalyMap.type() != CV_32FC1 ||
       !cv::checkRange(anomalyMap, true, nullptr, 0.0, 1.000001))
        throw std::invalid_argument("异常检测合并分数图必须为有效的归一化单通道浮点图像。");

    // 在还原后的浮点图上统一筛选，确保边界插值后的输出分数也严格高于当前阈值。
    const cv::Mat mask = anomalyMap > threshold;
    cv::Mat labels, stats, centroids;
    const int count = cv::connectedComponentsWithStats(mask, labels, stats, centroids, 8, CV_32S);
    std::vector<float> minimumScores(count, (std::numeric_limits<float>::max)());
    for(int y = 0; y < labels.rows; ++y)
    {
        const int* labelRow = labels.ptr<int>(y);
        const float* scoreRow = anomalyMap.ptr<float>(y);
        for(int x = 0; x < labels.cols; ++x)
        {
            const int label = labelRow[x];
            // 只统计连通区域本身，外接框中的背景及内部孔洞不参与最低分数计算。
            if(label > 0)
                minimumScores[label] = (std::min)(minimumScores[label], scoreRow[x]);
        }
    }

    std::vector<DetectDetResult> results;
    for(int label = 1; label < count; ++label)
    {
        const int* stat = stats.ptr<int>(label);
        const cv::Rect box(stat[cv::CC_STAT_LEFT], stat[cv::CC_STAT_TOP],
                           stat[cv::CC_STAT_WIDTH], stat[cv::CC_STAT_HEIGHT]);
        // 沿用旧检测的框面积筛选；使用宽整数计算，避免面积乘法溢出。
        if(static_cast<int64_t>(box.width) * box.height < minimumBoxArea)
            continue;
        DetectDetResult result{};
        result.classId = 1;
        result.score = minimumScores[label];
        result.box = cv::Rect2d(static_cast<double>(offset.x) + box.x,
                                static_cast<double>(offset.y) + box.y, box.width, box.height);
        results.push_back(result);
    }
    return results;
}

std::vector<DetectDetResult> detect(DinomalyOnnx& model, const cv::Mat& image,
                                  const cv::Rect& selection, double threshold, int minimumBoxArea)
{
    validateThresholds(threshold, minimumBoxArea);
    if(image.empty() || image.dims != 2 || image.depth() != CV_8U)
        throw std::invalid_argument("异常检测输入必须是非空的八位图像。");
    if(selection.width <= 0 || selection.height <= 0)
        throw std::invalid_argument("异常检测选区宽高必须为正数。");

    // 用宽整数裁剪选区，避免 x+width 或 y+height 溢出；检测绝不扩展到选区之外。
    const int64_t left = (std::max)(int64_t(0), int64_t(selection.x));
    const int64_t top = (std::max)(int64_t(0), int64_t(selection.y));
    const int64_t right = (std::min)(int64_t(image.cols), int64_t(selection.x) + selection.width);
    const int64_t bottom = (std::min)(int64_t(image.rows), int64_t(selection.y) + selection.height);
    if(right <= left || bottom <= top)
        throw std::invalid_argument("异常检测选区与图像没有有效交集。");
    const cv::Rect roi(static_cast<int>(left), static_cast<int>(top),
                       static_cast<int>(right - left), static_cast<int>(bottom - top));
    cv::Mat roiImage = image(roi);
    if(roiImage.channels() == 2)
        cv::cvtColor(roiImage, roiImage, cv::COLOR_BGR5652BGR);
    else if(roiImage.channels() != 1 && roiImage.channels() != 3 && roiImage.channels() != 4)
    {
        // 保留旧实现对特殊多通道图像取首通道的兼容行为。
        cv::extractChannel(roiImage, roiImage, 0);
    }

    cv::Mat mergedMap = cv::Mat::zeros(roi.size(), CV_32FC1);
    for(const cv::Rect& region : tileRegions(roi.size()))
    {
        AnomalyResult prediction;
        const int status = model.detect(roiImage(region), prediction);
        if(status != 0)
            throw std::runtime_error("Dinomaly 分段推理失败：" + model.lastError());
        mergeTileMap(mergedMap, region, prediction.anomalyMap);
    }
    // 与训练验证保持一致，重叠区域取最大分数；合并后再提框，跨切块缺陷只输出一次。
    return extractDefects(mergedMap, roi.tl(), threshold, minimumBoxArea);
}
}
