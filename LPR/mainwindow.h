    #ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include<QFileDialog>
#include<QMessageBox>
#include<QEvent>
#include<QCloseEvent>
#include<QPixmap>
#include<QTimer>
#include<vector>
#include<algorithm>
#include<windows.h>
#include<iostream>
#include<opencv2/highgui/highgui.hpp>
#include<opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
    QImage MatToQimg(const Mat& mat);
    Mat frame;//处理帧
    QPixmap adaptImg(const QImage& qImg,int h,int w);
    bool canSegChar;
    int width;//车牌显示宽度
    int height;
private slots:
    void readFrame();

    void on_pushButton_openVideo_clicked();

    void on_pushButton_morphy_clicked();

    void on_pushButton_charseg_clicked();

    void on_pushButton_template_clicked();


    private:
    Ui::MainWindow *ui;
    QTimer *CntTime;
    VideoCapture *m_pVideo;
    Mat *lice;
    Mat *morphology_lice;
    Mat liceChar[7];
};
#endif // MAINWINDOW_H
