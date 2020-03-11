#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <imgviewer.h>
#include <ui_pixelTForm.h>
#include <ui_lFilterForm.h>
#include <ui_operOrderForm.h>

#include <QtWidgets/QFileDialog>




using namespace cv;

namespace Ui {
    class MainWindow;
}

class PixelTDialog : public QDialog, public Ui::PixelTForm
{
    Q_OBJECT

public:
    PixelTDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};

class LFilterDialog : public QDialog, public Ui::LFilterForm
{
    Q_OBJECT

public:
    LFilterDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};



class OperOrderDialog : public QDialog, public Ui::OperOrderForm
{
    Q_OBJECT

public:
    OperOrderDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
};




class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    //Interfaz principal de usuario
    Ui::MainWindow *ui;
    //Dialogos
    PixelTDialog pixelTDialog;
    LFilterDialog lFilterDialog;
    OperOrderDialog operOrderDialog;
    //Timer
    QTimer timer;

    VideoCapture *cap;
    ImgViewer *visorS, *visorD, *visorHistoS, *visorHistoD;
    Mat colorImage, grayImage, destColorImage, destGrayImage;
    Mat destColorImageAux, destGrayImageAux;
    bool winSelected;
    Rect imageWindow;



    std::vector<uchar> tablaLUT;
    Mat kernel;

    void updateHistograms(Mat image, ImgViewer * visor);

public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow();
    void loadFromFile();
    void saveToFile();
    void transformPixel(Mat image, Mat destImage);
    void thresholding(Mat image, Mat destImage);
    void equalize(Mat image, Mat destImage);
    void applyGaussianBlur(Mat image, Mat destImage);
    void applyMedianBlur(Mat image, Mat destImage);
    void linearFilter(Mat image, Mat destImage);
    void dilate(Mat image, Mat destImage);
    void erode(Mat image, Mat destImage);


    void setPixelTransformation();
    void setKernel();
    void setOperationOrder();

    void selectOperation(Mat image, Mat destImage, int option);
};


#endif // MAINWINDOW_H
