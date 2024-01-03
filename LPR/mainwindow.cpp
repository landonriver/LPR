#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include<opencv2/core.hpp>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    CntTime = new QTimer(this);
    CntTime->setInterval(30);
    connect(CntTime, &QTimer::timeout, this, &MainWindow::readFrame);
    canSegChar=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event){
    int result = QMessageBox::warning(this,"Exit",
    "确定关闭窗口?",
    QMessageBox::Yes,
    QMessageBox::No);
    if(result==QMessageBox::Yes)
        event->accept();
    else event->ignore();
}

QImage MainWindow::MatToQimg(const Mat& mat)
{

    if(mat.type() == CV_8UC1)
    {
        QImage qimg(mat.cols, mat.rows, QImage::Format_Indexed8);
        qimg.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            qimg.setColor(i, qRgb(i, i, i));
        }
        uchar *src = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = qimg.scanLine(row);
            memcpy(pDest, src, mat.cols);
            src += mat.step;
        }
        return qimg;
    }
    else if(mat.type() == CV_8UC3)
    {
        const uchar *pSrc = (const uchar*)mat.data;
        QImage img(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return img.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        const uchar *pSrc = (const uchar*)mat.data;
        QImage img(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return img.copy();
    }
    else
    {
        return QImage();
    }
}


QPixmap MainWindow::adaptImg(const QImage& qImg, int h, int w)
{
    // 从QImage创建QPixmap
    QPixmap pixmap = QPixmap::fromImage(qImg);

    // 缩放QPixmap以适应指定的高度和宽度，保持纵横比并进行平滑处理
    QPixmap fitpixmap = pixmap.scaled(w, h, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // 返回适应后的QPixmap
    return fitpixmap;
}




void MainWindow::readFrame()
{
    Mat frame;

       //从cap中读取一帧数据，存到fram中
       *m_pVideo >> frame;
       if ( frame.empty() )
       {
           return;
       }
       //处理关键帧
        cv::Mat hsv;
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        //车牌颜色范围
        cv::Vec3b color1(110, 50, 50);
        cv::Vec3b color2(130, 255, 255);

        // 二值化
        cv::Mat mask;
        cv::inRange(hsv, color1, color2, mask);

        //形态学处理
        dilate(mask, mask, getStructuringElement(MORPH_RECT, Size(1, 3)), Point(-1, -1), 1);
//        erode(mask, mask, getStructuringElement(MORPH_RECT, Size(3, 1)), Point(-1, -1), 1);

        //显示形态学处理结果
        vector<vector<cv::Point>> vpp;
        vector<Vec4i> hierarchy;
        findContours(mask, vpp, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);//找轮廓

        RotatedRect maxRec;//最大矩形
        for (auto& i : vpp)
        {
          RotatedRect r_rect = minAreaRect(i);
          if (r_rect.size.area() > maxRec.size.area())
          {
              maxRec = r_rect;

          }
        }


        if (maxRec.size.area() > 5000) {//找到车牌
          Mat mask_r;
          Rect mask_rect = maxRec.boundingRect();
          Mat lic_r = frame(mask_rect).clone();

          //显示关键帧
          QImage qImg = MatToQimg(frame);
          QPixmap pixmap = QPixmap::fromImage(qImg);
          int width=ui->label_keyframe->width();
          int height=ui->label_keyframe->height();
          QPixmap fitpixmap=pixmap.scaled(width,height,Qt::KeepAspectRatio, Qt::SmoothTransformation);
          ui->label_keyframe->setPixmap(fitpixmap);
          //显示车牌
          cvtColor(lic_r, lic_r, COLOR_BGR2GRAY);
          QImage qLic = MatToQimg(lic_r);
          width=ui->lp_img->width();
          height=ui->lp_img->height();
          ui->lp_img->setPixmap(adaptImg(qLic,height,width));

          lice=new Mat(lic_r);
          QMessageBox::information(this,"提示",
              "发现车牌");
          canSegChar=true;
          CntTime->stop();
        }

        // 获取车牌部分
        cv::Mat final;
        cv::bitwise_and(frame, frame, final, mask);

       //显示图片
        QImage qImg = MatToQimg(frame);
        int w=ui->label_video->width();
        int h=ui->label_video->height();
        ui->label_video->setPixmap(adaptImg(qImg,h,w));
}

void MainWindow::on_pushButton_openVideo_clicked()
{

        QString fileName = QFileDialog::getOpenFileName (
                    this,
                    "Open Input File",
                    QDir::currentPath(),
        "Video(*.avi *.mp4)");

        VideoCapture cap(fileName.toStdString());//读取视频

        if(!cap.isOpened())
        {
            QMessageBox::warning(this,"错误",
                "读取视频失败");
        }
        else
        {
            m_pVideo=new VideoCapture(fileName.toStdString());

        }
        CntTime->start();

}


void MainWindow::on_pushButton_morphy_clicked()
{
    if(canSegChar==false)
    {
        QMessageBox::warning(this,"错误",
                             "未发现车牌，无法分割");
    }
    else
    {

        Mat match_img = (*lice).clone();
        threshold(match_img, match_img, 100, 255, THRESH_BINARY);

        //裁切
        int height = match_img.size().height;
        int width = match_img.size().width;
        Mat clip_bin_lic = match_img(Range(0.05*height,  height*0.95), Range(0, width));//裁切的车牌


        dilate(clip_bin_lic, clip_bin_lic, getStructuringElement(MORPH_RECT,Size(2, 2)), Point(-1, -1),2);
//        erode(clip_bin_lic, clip_bin_lic, getStructuringElement(MORPH_RECT, Size(1, 2)), Point(-1, -1));

        morphology_lice=new Mat(clip_bin_lic);//创建共公有变量
        //显示形态学和裁切后的图片
        QImage qImg_morph_clip=MatToQimg(*morphology_lice);
        ui->label_morph->setPixmap(adaptImg(qImg_morph_clip,height,width));
    }
}


void MainWindow::on_pushButton_charseg_clicked()
{
    Mat final_lic=(*morphology_lice).clone();//此为形态学和裁剪处理后的图片

    //寻找轮廓
           vector<vector<cv::Point>> contours;
           vector<Vec4i> hier;

           findContours(final_lic, contours, hier, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));   //寻找轮廓

          Mat img=final_lic.clone();
          cvtColor(img, img, COLOR_GRAY2RGB);


          //分割各个字符
          int sum=0;
          for (auto& i : contours) {
              Rect rect = boundingRect(i);
              sum += rect.area();
          }

          vector<vector<Point>> Con;//字符轮廓
          vector<Rect> rectInfo;//字符的矩形信息
          for (auto& i : contours) {
              Rect rect = boundingRect(i);
              if (rect.area() > sum / contours.size()-50)
              {
                  Con.push_back(i);
                  rectInfo.push_back(rect);
              }
          }
          drawContours(img, Con, -1, Scalar(0, 0, 255), 2);

          std::sort(rectInfo.begin(), rectInfo.end(), [](const Rect& a, const Rect& b)
              {
                  return a.x < b.x;
              });

          Mat no_morph=(*morphology_lice).clone();

          Mat lic_char[7];
          QImage licImg[7];
          QPixmap finalChar[7];
          int h=ui->lp_char1->height();
          int w=ui->lp_char1->width();
          for(int i=0;i<7;i++)
          {
              lic_char[i]=no_morph(rectInfo[i]);
              liceChar[i]=lic_char[i];
              imwrite(to_string(i)+".jpg",liceChar[i]);//保存字符
              licImg[i]=MatToQimg(lic_char[i]);
              finalChar[i]=adaptImg(licImg[i],h,w);
          }
          Point tl=rectInfo[0].tl();
          Point br=rectInfo[0].br();
          tl.y-=6;
          imwrite("0.jpg",final_lic(Rect(tl,br)));
          licImg[0]=MatToQimg(final_lic(Rect(tl,br)));
          finalChar[0]=adaptImg(licImg[0],h,w);
          ui->lp_seg1->setPixmap(finalChar[0]);
          ui->lp_seg2->setPixmap(finalChar[1]);
          ui->lp_seg3->setPixmap(finalChar[2]);
          ui->lp_seg4->setPixmap(finalChar[3]);
          ui->lp_seg5->setPixmap(finalChar[4]);
          ui->lp_seg6->setPixmap(finalChar[5]);
          ui->lp_seg7->setPixmap(finalChar[6]);




}


void MainWindow::on_pushButton_template_clicked()
{
    // 省份简称
    QString provinces[] = {"京", "津", "冀", "晋", "蒙", "辽", "吉", "黑", "沪", "苏",
                           "浙", "皖", "闽", "赣", "鲁", "豫", "鄂", "湘", "粤", "桂",
                           "琼", "川", "贵", "云", "藏", "陕", "甘", "青", "宁", "新", "渝"};

    // 车牌字符
    string CharName[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                         "A", "B", "C", "D", "E", "F", "G", "H", "J",
                         "K", "L", "M", "N", "P", "Q", "R", "S", "T",
                         "U", "V", "W", "X", "Y", "Z"};

    std::string provinceName[31];
    Mat match_imgs[7];

    // 转换省份简称为string类型
    for (int i = 0; i < 31; i++)
    {
        QString FileName = provinces[i];
        provinceName[i] = FileName.toLocal8Bit().toStdString();
    }

    // 读取并预处理匹配图片
    for (int i = 0; i < 7; i++)
    {
        match_imgs[i] = imread(to_string(i) + ".jpg");
        cvtColor(match_imgs[i], match_imgs[i], COLOR_BGR2GRAY);
        threshold(match_imgs[i], match_imgs[i], 100, 255, THRESH_BINARY);
    }

    Mat match_imgC = imread("0.jpg");
    cvtColor(match_imgC, match_imgC, COLOR_BGR2GRAY);
    threshold(match_imgC, match_imgC, 100, 255, THRESH_BINARY);
    QString qlic_charC;

    vector<float> max_valueC;
    vector<string> lic_char_nameC;

    QString qlic_char[7];
    for (auto i : provinceName)
    {
        string pattern = "province/" + i + "/*.jpg";
        std::vector<cv::String> fn;
        std::vector<cv::Mat> ListImg;
        cv::glob(pattern, fn);

        int count = fn.size();
        for (int j = 0; j < count; j++)
        {
            ListImg.push_back(imread(fn[j]));
        }

        // 模板匹配
        vector<float> result;
        for (auto &j : ListImg)
        {
            Size size = j.size();
            Mat r;
            cvtColor(j, j, COLOR_BGR2GRAY);
            threshold(j, j, 100, 255, THRESH_BINARY);
            cv::resize(match_imgC, match_imgC, size); // 尺寸一致

            matchTemplate(match_imgC, j, r, TM_CCOEFF_NORMED);
            // 找到最佳匹配
            double min_val, max_val;
            Point min_loc, max_loc;
            minMaxLoc(r, &min_val, &max_val, &min_loc, &max_loc);
            result.push_back(max_val);
        }

        auto max_temp = max_element(result.begin(), result.end()); // 获取此文件夹内最大匹配值
        max_valueC.push_back(*max_temp);
        lic_char_nameC.push_back(i); // 存储文件夹名
    }

    for (int i = 0; i < 7; i++)
    {
        Mat match_img = match_imgs[i].clone();
        vector<float> max_value;
        vector<string> lic_char_name;
        for (auto i : CharName)
        {
            string pattern = "num/" + i + "/*.jpg";
            std::vector<cv::String> fn;
            std::vector<cv::Mat> ListImg;
            cv::glob(pattern, fn);

            int count = fn.size(); // 当前文件夹中图片个数
            for (int j = 0; j < count; j++)
            {
                ListImg.push_back(imread(fn[j]));
            }

            // 模板匹配
            vector<float> result;
            for (auto &j : ListImg)
            {
                Size size = j.size();
                Mat r;
                cvtColor(j, j, COLOR_BGR2GRAY);
                threshold(j, j, 100, 255, THRESH_BINARY);
                cv::resize(match_img, match_img, size); // 必须尺寸一致

                matchTemplate(match_img, j, r, TM_CCOEFF_NORMED);
                // 找到最佳匹配
                double min_val, max_val;
                Point min_loc, max_loc;
                minMaxLoc(r, &min_val, &max_val, &min_loc, &max_loc);
                result.push_back(max_val);
            }

            auto max_temp = max_element(result.begin(), result.end()); // 获取此文件夹内最大匹配值
            max_value.push_back(*max_temp);
            lic_char_name.push_back(i); // 存储文件夹名
        }
        auto it = std::max_element(std::begin(max_value), std::end(max_value));
        size_t ind = std::distance(std::begin(max_value), it);
        qlic_char[i] = QString::fromStdString(lic_char_name[ind]);
    }

    auto it = std::max_element(std::begin(max_valueC), std::end(max_valueC));
    size_t ind = std::distance(std::begin(max_valueC), it);
    qlic_charC = provinces[ind];

    ui->lp_char1->setText(qlic_charC);
    ui->lp_char2->setText(qlic_char[1]);
    ui->lp_char3->setText(qlic_char[2]);
    ui->lp_char4->setText(qlic_char[3]);
    ui->lp_char5->setText(qlic_char[4]);
    ui->lp_char6->setText(qlic_char[5]);
    ui->lp_char7->setText(qlic_char[6]);
}


