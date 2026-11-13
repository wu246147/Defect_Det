#ifndef DIALOGTOOLPARA_H
#define DIALOGTOOLPARA_H

#include <QDialog>
#include "jobmanager.h"
#include "jobworker.h"
#include <QGridLayout>
#include <QSplashScreen>
#include "qwaiting.h"
#include <QMessageBox>
#include "mainwindow.h"

#pragma execution_character_set("utf-8")

namespace Ui
{
    class DialogToolPara;
}




void draw_roi(int event, int x, int y, int flags, void *param);


class DialogToolPara : public QDialog
{
    Q_OBJECT

public:
    explicit DialogToolPara(QWidget *parent = nullptr);
    DialogToolPara(JobManager *j, QWidget *parent = nullptr);

    ~DialogToolPara();

    ///
    /// \brief set_display
    ///
    void set_display();

    ///
    /// \brief init
    ///
    void init();

    ///
    /// \brief update_train_roi 更新训练roi
    ///
    void update_train_roi();

    ///
    /// \brief update_run_roi 更新运行roi
    ///
    void update_run_roi();

public:
    JobManager *jobmanager = nullptr;

    bool is_loading = true;

    MyGraphicRectItem *myRectItem_select = nullptr ;

    MyGraphicRectItem *myRectItem_location = nullptr ;

    MyGraphicRectItem *myRectItem_center = nullptr ;

    MyGraphicRectItem *myRectItem_corner1 = nullptr ;
    MyGraphicRectItem *myRectItem_corner2 = nullptr ;
    MyGraphicRectItem *myRectItem_corner3 = nullptr ;
    MyGraphicRectItem *myRectItem_corner4 = nullptr ;


    MyGraphicRectItem *myRectItem_caliperTool1 = nullptr ;
    MyGraphicRectItem *myRectItem_caliperTool2 = nullptr ;

    MyGraphicLineItem *myRectItem_caliperLine = nullptr ;


    QGridLayout *display_grid_layout = nullptr;

    int cam_id = 0;

    int work_mode = 0;//工作模式0为运行，1为训练

    bool exist_show_img = false;

    cv::Mat show_img;

    int select_simulation_img_id = -1; //仿真图片id


    void showCenterLine(int work_id);

    ///
    /// \brief show_display
    ///
    void show_display();

    ///
    /// \brief get_show_img
    ///
    void get_show_img();


    /**
     * @brief 根据当前相机和工作模式刷新参数控件及面板可见性。
     * @details 运行模式显示运行参数和仿真面板，训练模式显示训练参数并隐藏仿真面板。
     * 回填当前相机的定位、缺陷检测参数和仿真路径时，通过 is_loading 阻止槽函数反写参数，
     * 最后刷新仿真图片列表并重置其选中状态。
     * 缺陷模型下拉框随当前作业回填，阈值名称及提示按模型类型切换，回填不重复初始化模型。
     * 同时回填异常检测裁块大小和步长，按裁块大小更新步长上限；仅选择异常检测时显示这两个参数。
     * @note 仅在界面线程调用；jobmanager 必须有效，cam_id 必须指向已有作业。
     * 本函数会修改控件状态，但不会自动保存作业或重新训练定位模型。
     */
    void updata_control_value();

    ///
    /// \brief clear_simulation_table
    ///
    void clear_simulation_table();

    ///
    /// \brief show_simulation_table
    ///
    void show_simulation_table();

private slots:

    /**
     * @brief 响应用户选择缺陷模型，立即初始化所选类型并刷新阈值提示。
     * @param index 下拉框索引，0 为目标检测、1 为异常检测；其他索引及控件回填期间不执行操作。
     * @note 仅在界面线程调用，jobmanager 和 cam_id 必须有效；模型初始化与该作业推理串行执行。
     * 初始化失败弹出提示并保留所选类型，后续检测按不合格处理；点击保存后才持久化选择。
     */
    void on_comboBox_defect_model_currentIndexChanged(int index);

    /**
     * @brief 修改异常检测裁块边长，并在必要时将滑动步长同步缩小到新的边长。
     * @param arg1 控件提供的裁块边长上限，单位为像素，范围为 1～2147483647。
     * @note 仅在界面线程且 jobmanager、cam_id 有效时调用，回填期间不反写。
     * 成对更新作业参数并限制步长控件上限；使用信号阻断避免联动时重复写入，不重载模型或自动保存。
     */
    void on_spinBox_anomaly_tile_size_valueChanged(int arg1);

    /**
     * @brief 将异常检测滑动步长与当前控件裁块边长成对写入作业。
     * @param arg1 控件提供的步长，单位为像素，范围由控件保证为 1～当前裁块大小。
     * @note 仅在界面线程且 jobmanager、cam_id 有效时调用，回填期间不反写。
     * 新参数在下次异常检测生效，与正在执行的缺陷推理串行更新，不重载模型或自动保存。
     */
    void on_spinBox_anomaly_tile_stride_valueChanged(int arg1);

    void on_comboBox_cam_currentIndexChanged(int index);

    void on_comboBox_type_currentIndexChanged(int index);

    void on_pushButton_set_train_image_clicked();

    void on_pushButton_train_clicked();

    void on_pushButton_run_clicked();

    void change_roi_even(QGraphicsItem *item);


    void on_comboBox_location_caliper1_transition_currentIndexChanged(int index);

    void on_comboBox_location_caliper1_select_currentIndexChanged(int index);

    void on_spinBox_location_caliper1_thre_valueChanged(int arg1);

    void on_comboBox_location_caliper2_transition_currentIndexChanged(int index);

    void on_comboBox_location_caliper2_select_currentIndexChanged(int index);

    void on_spinBox_location_caliper2_thre_valueChanged(int arg1);

    void on_pushButton_save_clicked();

    void on_spinBox_canny_thre_min_valueChanged(int arg1);

    void on_spinBox_canny_thre_max_valueChanged(int arg1);

    void on_spinBox_chip_area_min_valueChanged(int arg1);

    void on_spinBox_chip_area_max_valueChanged(int arg1);

    void on_pushButton_getStandardPoint_clicked();

    void on_comboBox_resultType_currentIndexChanged(int index);

    void on_doubleSpinBox_xb_valueChanged(double arg1);

    void on_doubleSpinBox_yb_valueChanged(double arg1);

    void on_doubleSpinBox_rb_valueChanged(double arg1);

    void on_doubleSpinBox_xa_valueChanged(double arg1);

    void on_doubleSpinBox_ya_valueChanged(double arg1);

    void on_doubleSpinBox_ra_valueChanged(double arg1);

    void on_spinBox_centerLineD_valueChanged(int arg1);

    void on_comboBox_location_Politary_currentIndexChanged(int index);

    void on_doubleSpinBox_distance_thre_min_valueChanged(double arg1);

    void on_doubleSpinBox_distance_thre_max_valueChanged(double arg1);

    void on_lineEdit_simulation_path_textChanged(const QString &arg1);

    void on_pushButton_open_dir_clicked();

    void on_tableWidget_simualtion_filePath_itemSelectionChanged();

    void on_pushButton_run_last_clicked();

    void on_pushButton_run_next_clicked();

    void on_spinBox_location_caliper_roi_count_valueChanged(int arg1);

    void on_spinBox_location_caliper_roi_h_valueChanged(int arg1);

    void on_spinBox_location_caliper_roi_w_valueChanged(int arg1);

    void on_doubleSpinBox_object_detect_thre_valueChanged(double arg1);

    void on_spinBox_defect_area_thre_valueChanged(int arg1);

    /**
     * @brief 将训练参数框中的定位低阈值写入当前相机作业。
     * @details 回填控件期间直接返回；用户调整时只更新 LowThreshold，供下次定位训练使用。
     * @param arg1 控件提供的低阈值，无量纲整数，界面允许范围为 0～255。
     * @note 仅在界面线程调用；jobmanager 和 cam_id 必须有效，不自动训练或保存作业。
     */
    void on_spinBox_location_low_threshold_valueChanged(int arg1);

    /**
     * @brief 将训练参数框中的定位高阈值写入当前相机作业。
     * @details 回填控件期间直接返回；用户调整时只更新 HighThreshold，供下次定位训练使用。
     * @param arg1 控件提供的高阈值，无量纲整数，界面允许范围为 0～255。
     * @note 仅在界面线程调用；jobmanager 和 cam_id 必须有效，不自动训练或保存作业。
     */
    void on_spinBox_location_high_threshold_valueChanged(int arg1);

    /**
     * @brief 将训练参数框中的定位最低匹配分数写入当前相机作业。
     * @details 回填控件期间直接返回；用户调整时将数值转换为作业使用的 float，供后续定位匹配使用。
     * @param arg1 控件提供的最低匹配分数，无量纲，界面允许范围为 0～1，保留三位小数。
     * @note 仅在界面线程调用；jobmanager 和 cam_id 必须有效，不自动运行匹配或保存作业。
     */
    void on_doubleSpinBox_location_min_score_valueChanged(double arg1);

    void on_pushButton_update_trainROI_clicked();

signals:
    void save_file_signal();


private:
    /**
     * @brief 根据当前作业的缺陷模型类型刷新阈值提示和异常分块控件可见性。
     * @details 目标检测显示置信度及库内 0.5 下限，异常检测显示严格大于阈值的异常像素分数语义。
     * 裁块大小、步长及其标签仅在异常检测模式下显示并启用，切回目标检测仍保留已设置的分块参数。
     * @note 仅在界面线程且 jobmanager、cam_id 有效时调用；只修改文字，不改变参数或初始化模型。
     */
    void updateDefectModelControls();

    void closeEvent(QCloseEvent *event);

private:
    Ui::DialogToolPara *ui;
};

#endif // DIALOGTOOLPARA_H
