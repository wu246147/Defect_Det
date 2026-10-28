#include "defectanomaly.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace
{
/** @brief 检查验证条件；condition 为假时抛出带 message 的异常。 */
void check(bool condition, const std::string& message)
{
    if(!condition)
        throw std::runtime_error(message);
}

/** @brief 执行 action 并确认其拒绝无效输入；name 用于标识失败的验证项。 */
template<typename Action>
void expectFailure(Action action, const char* name)
{
    bool failed = false;
    try
    {
        action();
    }
    catch(const std::exception&)
    {
        failed = true;
    }
    check(failed, name);
}

/** @brief 验证训练数据对应的切块起点、边界覆盖、小图与横向宽图的切分规则。 */
void testTiles()
{
    const auto regions = DefectAnomaly::tileRegions({403, 1908});
    check(regions.size() == 4, "403x1908 应生成四块");
    const int ys[] = {0, 480, 960, 1268};
    for(int i = 0; i < 4; ++i)
        check(regions[i] == cv::Rect(0, ys[i], 403, 640), "切块位置须与训练一致");
    for(const cv::Size size : {cv::Size(20, 30), cv::Size(640, 640), cv::Size(641, 641),
                              cv::Size(1120, 640), cv::Size(1501, 1703)})
    {
        cv::Mat coverage = cv::Mat::zeros(size, CV_8UC1);
        const auto tiles = DefectAnomaly::tileRegions(size);
        for(size_t index = 0; index < tiles.size(); ++index)
        {
            const auto& tile = tiles[index];
            check((tile & cv::Rect(0, 0, size.width, size.height)) == tile, "切块不能越界");
            check(index == 0 || tile != tiles[index - 1], "末块不能重复");
            coverage(tile).setTo(255);
        }
        check(cv::countNonZero(coverage) == size.area(), "所有 ROI 像素都必须被覆盖");
    }
    expectFailure([] { DefectAnomaly::tileRegions({0, 20}); }, "拒绝空 ROI");
    std::cout << "切块及边界覆盖验证通过\n";
}

/** @brief 用跨切块的合成异常验证最大值融合、最低分数、孔洞排除及整图坐标还原。 */
void testExtraction()
{
    cv::Mat merged = cv::Mat::zeros(1908, 403, CV_32FC1);
    cv::Mat first = cv::Mat::zeros(640, 403, CV_32FC1);
    first(cv::Rect(10, 620, 8, 20)).setTo(0.8f);
    cv::Mat second = cv::Mat::zeros(640, 403, CV_32FC1);
    second(cv::Rect(10, 140, 8, 50)).setTo(0.9f);
    second.at<float>(189, 17) = 0.65f;
    DefectAnomaly::mergeTileMap(merged, {0, 0, 403, 640}, first);
    DefectAnomaly::mergeTileMap(merged, {0, 480, 403, 640}, second);
    check(merged.at<float>(625, 11) == 0.9f, "重叠区保留最大分数");
    auto results = DefectAnomaly::extractDefects(merged, {100, 200}, 0.5, 1);
    check(results.size() == 1, "跨切块缺陷应合并为一个连通区域");
    check(results[0].box == cv::Rect2d(110, 820, 8, 50), "外接框必须恢复整图坐标");
    check(results[0].classId == 1 && std::abs(results[0].score - 0.65f) < 1e-6,
          "分数必须是异常区域最低像素分数");
    check(DefectAnomaly::extractDefects(merged, {}, 0.5, 401).empty(), "保留旧外接框面积过滤语义");
    check(DefectAnomaly::extractDefects(merged, {}, 0.5, 400).size() == 1, "面积等于阈值时保留");
    check(DefectAnomaly::extractDefects(merged, {}, 1.0, 0).empty(), "阈值为一时无异常");

    cv::Mat ring = cv::Mat::zeros(12, 14, CV_32FC1);
    ring(cv::Rect(2, 2, 5, 5)).setTo(0.75f);
    ring(cv::Rect(3, 3, 3, 3)).setTo(0.1f);
    ring.at<float>(8, 10) = 0.95f;
    results = DefectAnomaly::extractDefects(ring, {}, 0.5, 1);
    check(results.size() == 2, "独立异常区域分别提框");
    check(results[0].box == cv::Rect2d(2, 2, 5, 5) && results[0].score == 0.75f,
          "外接框内部背景与孔洞不能拉低分数");
    check(DefectAnomaly::extractDefects(ring, {}, 0.75, 1).size() == 1, "等于阈值的像素被排除");
    expectFailure([&] { DefectAnomaly::extractDefects(ring, {}, -0.1, 0); }, "拒绝负阈值");
    expectFailure([&] { DefectAnomaly::extractDefects(ring, {}, 1.1, 0); }, "拒绝超范围阈值");
    expectFailure([&] { DefectAnomaly::extractDefects(ring, {}, std::numeric_limits<double>::quiet_NaN(), 0); },
                  "拒绝 NaN 阈值");
    expectFailure([&] { DefectAnomaly::extractDefects(ring, {}, 0.5, -1); }, "拒绝负面积");
    ring.at<float>(0, 0) = std::numeric_limits<float>::infinity();
    expectFailure([&] { DefectAnomaly::extractDefects(ring, {}, 0.5, 0); }, "拒绝无穷分数");
    expectFailure([&] { DefectAnomaly::mergeTileMap(merged, {-1, 0, 403, 640}, first); }, "拒绝越界切块");
    expectFailure([&] { DefectAnomaly::mergeTileMap(merged, {0, 0, 403, 600}, first); }, "拒绝错误分数图尺寸");
    std::cout << "融合、连通区域、阈值、最低分数和坐标验证通过\n";
}

/** @brief 验证未初始化模型、空图、越界选区及整数边界不会被当作无缺陷结果返回。 */
void testFailures()
{
    DinomalyOnnx model;
    cv::Mat image(20, 30, CV_8UC3, cv::Scalar::all(255));
    expectFailure([&] { DefectAnomaly::detect(model, image, {0, 0, 30, 20}, 0.5, 0); }, "拒绝未初始化模型");
    expectFailure([&] { DefectAnomaly::detect(model, {}, {0, 0, 30, 20}, 0.5, 0); }, "拒绝空图");
    expectFailure([&] { DefectAnomaly::detect(model, image, {40, 50, 10, 10}, 0.5, 0); }, "拒绝无交集选区");
    expectFailure([&] { DefectAnomaly::detect(model, image, {0, 0, -1, 10}, 0.5, 0); }, "拒绝负尺寸选区");
    expectFailure([&] { DefectAnomaly::detect(model, image,
                      {(std::numeric_limits<int>::max)(), 0, 100, 10}, 0.5, 0); }, "越界选区加法不能溢出");
    std::cout << "错误处理验证通过\n";
}

/**
 * @brief 使用真实 Dinomaly 模型验证四块推理、选区隔离和运行时阈值修改。
 * @param modelPath 模型路径，须为新库支持的已校准 ONNX。
 * @param imagePath 正常或异常测试切片路径，拼成 403×1908 的 ROI 以验证实际部署流程。
 */
void testModel(const char* modelPath, const char* imagePath)
{
    DinomalyOnnx model;
    check(model.init(modelPath, 0.5f, false) == 0, "模型初始化失败：" + model.lastError());
    const cv::Mat sample = cv::imread(imagePath);
    check(!sample.empty(), "未读取到测试切片");
    cv::Mat roiImage;
    cv::resize(sample, roiImage, {403, 1908});
    const cv::Rect selection(23, 31, roiImage.cols, roiImage.rows);
    cv::Mat image(2000, 460, CV_8UC3, cv::Scalar::all(0));
    roiImage.copyTo(image(selection));
    const auto before = image.clone();
    const auto initial = DefectAnomaly::detect(model, image, selection, 0.0, 1);
    check(!initial.empty(), "真实模型应产生非零异常分数区域");
    for(const auto& result : initial)
    {
        const cv::Rect2d bounds(selection);
        check((result.box & bounds) == result.box && result.score > 0 && result.score <= 1,
              "所有框及有效分数应来自选区内部");
    }
    check(cv::norm(image, before, cv::NORM_INF) == 0, "检测不能修改原图");

    // 只改变选区外的像素；若仍对整图推理，输出就不能保持一致。
    image.setTo(cv::Scalar::all(255));
    roiImage.copyTo(image(selection));
    const auto changedOutside = DefectAnomaly::detect(model, image, selection, 0.0, 1);
    check(initial.size() == changedOutside.size(), "选区外内容不能改变缺陷数量");
    for(size_t index = 0; index < initial.size(); ++index)
        check(initial[index].box == changedOutside[index].box &&
              std::abs(initial[index].score - changedOutside[index].score) < 1e-6,
              "选区外内容不能影响框和分数");
    check(DefectAnomaly::detect(model, image, selection, 1.0, 1).empty(), "修改阈值无需重新初始化");

    // 负起点选区裁剪后只剩下 sample 图像，不应补边或访问越界内存。
    const auto clipped = DefectAnomaly::detect(model, sample, {-9, -7, sample.cols + 9, sample.rows + 7}, 0.0, 1);
    const auto full = DefectAnomaly::detect(model, sample, {0, 0, sample.cols, sample.rows}, 0.0, 1);
    check(clipped.size() == full.size(), "越界选区应按有效交集检测");
    for(size_t index = 0; index < full.size(); ++index)
        check(clipped[index].box == full[index].box &&
              std::abs(clipped[index].score - full[index].score) < 1e-6, "裁剪后坐标和分数应相同");

    std::vector<cv::Mat> variants(4);
    cv::cvtColor(sample, variants[0], cv::COLOR_BGR2GRAY);
    cv::cvtColor(sample, variants[1], cv::COLOR_BGR2BGR565);
    cv::cvtColor(sample, variants[2], cv::COLOR_BGR2BGRA);
    const std::vector<cv::Mat> channels(5, variants[0]);
    cv::merge(channels, variants[3]);
    for(const auto& variant : variants)
    {
        const auto snapshot = variant.clone();
        check(DefectAnomaly::detect(model, variant, {0, 0, variant.cols, variant.rows}, 1.0, 1).empty(),
              "兼容灰度、双通道、BGRA 和特殊多通道图像");
        check(cv::norm(variant, snapshot, cv::NORM_INF) == 0, "通道转换不能修改输入像素");
    }
    std::cout << "真实模型四块推理、选区隔离、阈值即时生效及裁剪验证通过\n";
}
}

/** @brief 执行合成验证；可选传入模型和测试图路径继续执行真实模型集成验证，失败返回 1。 */
int main(int argc, char** argv)
{
    try
    {
        testTiles();
        testExtraction();
        testFailures();
        if(argc == 3)
            testModel(argv[1], argv[2]);
        else
            check(argc == 1, "用法：defect_anomaly_tests [dinomaly.onnx sample.png]");
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
