#include "jobworker.h"

#include <QCoreApplication>
#include <sstream>
#include <stdexcept>

namespace
{
/**
 * @brief 校验测试条件，并在失败时终止当前测试流程。
 * @param condition 预期为真的条件；为假时抛出异常。
 * @param message 失败原因，非空字符串，由调用方在调用期间持有。
 * @throws std::runtime_error 条件不成立时携带失败原因抛出。
 */
void check(bool condition, const char* message)
{
    if(!condition)
    {
        throw std::runtime_error(message);
    }
}

/**
 * @brief 验证默认类型、两种类型的序列化往返以及配置复制行为。
 * @details 使用内存二进制归档，不加载模型或访问硬件；验证每次读取均停在作业结束标记。
 * 测试对象和归档均由局部变量自动释放，校验失败时异常交由入口处理。
 */
void testSerialization()
{
    int id = 0;
    JobWorker source(id);
    check(source.defectModelType == DefectModelType::ObjectDetection, "新作业默认使用目标检测");
    for(const auto type : {DefectModelType::ObjectDetection, DefectModelType::AnomalyDetection})
    {
        source.defectModelType = type;
        source.defectDetThre = 0.73;
        source.defectAreaThre = 432;
        std::stringstream stream;
        {
            boost::archive::binary_oarchive archive(stream);
            archive &source;
            int sentinel = 12345;
            archive &sentinel;
        }
        JobWorker loaded(id);
        {
            boost::archive::binary_iarchive archive(stream);
            archive &loaded;
            int sentinel = 0;
            archive &sentinel;
            check(sentinel == 12345, "读取作业不能破坏后续归档数据");
        }
        check(loaded.defectModelType == type, "缺陷模型类型序列化往返");
        check(loaded.defectDetThre == 0.73 && loaded.defectAreaThre == 432, "原有缺陷阈值继续保存读取");
        JobWorker copied(source);
        JobWorker pointerCopied(&source);
        JobWorker assigned(id);
        assigned = source;
        assigned = assigned;
        check(copied.defectModelType == type && pointerCopied.defectModelType == type &&
              assigned.defectModelType == type, "两种构造复制及赋值均保留模型选择");
    }
}

/**
 * @brief 验证旧作业缺字段和非法模型类型均使用目标检测，且可读取后续其他参数。
 * @details 手工生成既有字段协议的最小归档；先将接收对象设为异常检测，防止默认构造掩盖兼容问题。
 * 不操作模型、文件和硬件，局部归档自动释放；校验失败时异常交由入口处理。
 */
void testLegacySerialization()
{
    int id = 0;
    for(const bool invalidType : {false, true})
    {
        std::stringstream stream;
        {
            boost::archive::binary_oarchive archive(stream);
            if(invalidType)
            {
                std::string name = "defectModelType";
                int value = 99;
                archive &name &value;
            }
            std::string name = "defectAreaThre";
            int area = 678;
            archive &name &area;
            name = "JobWorkerEnd";
            archive &name;
        }
        JobWorker loaded(id);
        loaded.defectModelType = DefectModelType::AnomalyDetection;
        boost::archive::binary_iarchive archive(stream);
        loaded.serialize(archive, 0);
        check(loaded.defectModelType == DefectModelType::ObjectDetection, "旧作业或非法类型恢复目标检测默认值");
        check(loaded.defectAreaThre == 678, "兼容读取不影响后续参数");
    }
}

/**
 * @brief 验证模型未初始化和输入异常时清除旧结果并判定为不合格。
 * @details 直接执行检测入口，不加载模型或硬件；人为放入旧结果，检查两种模型类型均不返回残留缺陷。
 * 校验失败时异常交由入口处理，临时图像与作业自动释放。
 */
void testUninitializedModels()
{
    int id = 0;
    JobWorker worker(id);
    for(const auto type : {DefectModelType::ObjectDetection, DefectModelType::AnomalyDetection})
    {
        worker.defectModelType = type;
        worker.defectDetResult = true;
        worker.defectDetResults.push_back(DetectDetResult{});
        worker.runDefectDetTool(cv::Mat::zeros(32, 32, CV_8UC3));
        check(!worker.defectDetResult && worker.defectDetResults.empty(), "未初始化模型不能返回合格或残留缺陷");
        worker.runDefectDetTool({});
        check(!worker.defectDetResult, "空图必须判定为不合格");
    }
    check(!worker.setDefectModelType(static_cast<DefectModelType>(99)), "拒绝无效模型选择");
    check(worker.defectModelType == DefectModelType::AnomalyDetection, "无效选择不改变当前类型");
}

/**
 * @brief 使用程序目录中的真实模型验证目标、异常、目标的连续切换与推理。
 * @details 按生产路径初始化模型，采用阈值 1 验证推理完成；检查输入图像不变和失败后结果清除。
 * 此测试不评估现场检出率，也不启动相机或通讯；模型随作业析构释放，失败时异常交由入口处理。
 */
void testRealModels()
{
    int id = 0;
    JobWorker worker(id);
    worker.defectSelectBox = {0, 0, 80, 64};
    worker.defectDetThre = 1.0;
    const cv::Mat image = cv::Mat::zeros(64, 80, CV_8UC3);
    const cv::Mat original = image.clone();
    for(const auto type : {DefectModelType::ObjectDetection, DefectModelType::AnomalyDetection,
                          DefectModelType::ObjectDetection})
    {
        check(worker.setDefectModelType(type), "真实模型初始化及连续切换");
        check(!worker.defectDetResult && worker.defectDetResults.empty(), "切换清除旧结果并等待下次推理");
        worker.runDefectDetTool(image);
        check(worker.defectDetResult && worker.defectDetResults.empty(), "所选真实模型完成推理并应用阈值");
        check(cv::norm(image, original, cv::NORM_INF) == 0, "推理不修改输入图像");
        worker.defectSelectBox = {1000, 1000, 20, 20};
        worker.runDefectDetTool(image);
        check(!worker.defectDetResult && worker.defectDetResults.empty(), "越界选区按不合格处理");
        worker.defectSelectBox = {0, 0, 80, 64};
    }
}
}

/**
 * @brief 运行缺陷模型选择的兼容性验证，并可选运行真实模型切换测试。
 * @param argc 命令行参数数量；1 表示仅运行参数测试，2 时第二项必须为 --models。
 * @param argv 命令行参数数组，由运行时持有；--models 使用可执行文件目录的两种模型。
 * @return 所有校验通过返回 0；参数错误或校验失败输出原因并返回 1。
 * @note QCoreApplication 仅用于解析生产模型路径；不创建界面、不启动相机或通讯。
 */
int main(int argc, char* argv[])
{
    QCoreApplication application(argc, argv);
    try
    {
        check(argc == 1 || (argc == 2 && std::string(argv[1]) == "--models"),
              "用法：defect_model_tests [--models]");
        testSerialization();
        testLegacySerialization();
        testUninitializedModels();
        if(argc == 2)
        {
            testRealModels();
        }
        std::cout << "缺陷模型选择验证通过。" << std::endl;
        return 0;
    }
    catch(const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
