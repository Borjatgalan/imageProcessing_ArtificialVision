#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    //Todo hacer kernel

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destGrayImage.create(240,320,CV_8UC1);
    //Auxiliares
    destColorImageAux.create(240,320,CV_8UC3);
    destGrayImageAux.create(240,320,CV_8UC1);

    visorHistoS = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260,150, (QImage *) NULL, ui->histoFrameD);


    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    //Inicializacion de la matriz de kernels
    kernel.create(3,3,CV_32F);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(windowSelected(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(pressEvent()),this,SLOT(deselectWindow()));
    //Connect Load and save from file
    connect(ui->loadButton,SIGNAL(pressed()),this,SLOT(loadFromFile()));
    connect(ui->saveButton,SIGNAL(pressed()),this,SLOT(saveToFile()));


    connect(ui->pixelTButton,SIGNAL(clicked()),this,SLOT(setPixelTransformation()));
    connect(ui->kernelButton,SIGNAL(clicked()),this,SLOT(setKernel()));
    connect(ui->operOrderButton,SIGNAL(clicked()),this,SLOT(setOperationOrder()));


    timer.start(30);


}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
}

void MainWindow::compute()
{
    //Captura de imagen

    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;

        cv::resize(colorImage, colorImage, Size(320, 240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }


    //Procesamiento
    //QComboBox box;
    //box.currentIndex()

    //Actualización de los visores
    if(!ui->colorButton->isChecked())
    {
        selectOperation(grayImage, destGrayImage);
        updateHistograms(grayImage, visorHistoS);
        updateHistograms(destGrayImage, visorHistoD);
    }else{
        selectOperation(colorImage, destColorImage);
        updateHistograms(colorImage, visorHistoS);
        updateHistograms(destColorImage, visorHistoD);
    }

    if(winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x+imageWindow.width/2, imageWindow.y+imageWindow.height/2), imageWindow.width,imageWindow.height, Qt::green );
    }
    visorS->update();
    visorD->update();
    visorHistoS->update();
    visorHistoD->update();

}

void MainWindow::updateHistograms(Mat image, ImgViewer * visor)
{
    if(image.type() != CV_8UC1) return;

    Mat histogram;
    int channels[] = {0,0};
    int histoSize = 256;
    float grange[] = {0, 256};
    const float * ranges[] = {grange};
    double minH, maxH;

    calcHist( &image, 1, channels, Mat(), histogram, 1, &histoSize, ranges, true, false );
    minMaxLoc(histogram, &minH, &maxH);

    float maxY = visor->getHeight();

    for(int i = 0; i<256; i++)
    {
        float hVal = histogram.at<float>(i);
        float minY = maxY-hVal*maxY/maxH;

        visor->drawLine(QLineF(i+2, minY, i+2, maxY), Qt::red);
    }

}


void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        visorD->setImage(&destColorImage);

    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);

    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x;
        imageWindow.height = pEnd.y()-imageWindow.y;

        winSelected = true;
    }
}

void MainWindow::deselectWindow()
{
    winSelected = false;
}

void MainWindow::loadFromFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    Mat image;
    QString fileName = QFileDialog::getOpenFileName(this,tr("Open"), "/home",tr("Images (*.jpg *.png "
                                                                                "*.jpeg *.gif);;All Files(*)"));
    image = cv::imread(fileName.toStdString());

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            QMessageBox::information(this, tr("Unable to open file"), file.errorString());
            return;
        }
        ui->captureButton->setChecked(false);
        ui->captureButton->setText("Start capture");
        cv::resize(image, colorImage, Size(320, 240));
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
        cvtColor(colorImage, grayImage, COLOR_RGB2GRAY);

        connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));

    }
}

void MainWindow::saveToFile()
{
    disconnect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    Mat save_image;
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image File"),
                                                    QString(),
                                                    tr("JPG (*.JPG) ; jpg (*.jpg); png (*.png); jpeg(*.jpeg); gif(*.gif); All Files (*)"));
    if(ui->colorButton->isChecked())
        cvtColor(destColorImage, save_image, COLOR_RGB2BGR);

    else
        cvtColor(destGrayImage, save_image, COLOR_GRAY2BGR);

    if (fileName.isEmpty())
        return;
    else {
        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::information(this, tr("Unable to open file"),
                                     file.errorString());
            return;
        }
    }
    cv::imwrite(fileName.toStdString(), save_image);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
}

void MainWindow::transformPixel()
{


}

void MainWindow::thresholding(Mat image, Mat destImage)
{
    cv::threshold(image, destImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    destImage.copyTo(destGrayImageAux);
}

void MainWindow::equalize(Mat image, Mat destImage)
{
    cv::equalizeHist(image, destImage);
    destImage.copyTo(destGrayImageAux);
}

void MainWindow::applyGaussianBlur(Mat image, Mat destImage)
{
    double gaussValue = ui->gaussWidthBox->value()/5.0;
    cv::GaussianBlur(image, destImage, Size(ui->gaussWidthBox->value(), ui->gaussWidthBox->value()),gaussValue);
    destGrayImage.copyTo(destGrayImageAux);

}

void MainWindow::applyMedianBlur(Mat image, Mat destImage)
{
    cv::medianBlur(image, destImage, 3);
    destGrayImage.copyTo(destGrayImageAux);
}

void MainWindow::setKernel()
{
    printf("set kernel \n");
    kernel.at<float>(0,0)=lFilterDialog.kernelBox11->value();
    kernel.at<float>(0,1)=lFilterDialog.kernelBox12->value();
    kernel.at<float>(0,2)=lFilterDialog.kernelBox13->value();
    kernel.at<float>(1,0)=lFilterDialog.kernelBox21->value();
    kernel.at<float>(1,1)=lFilterDialog.kernelBox22->value();
    kernel.at<float>(1,2)=lFilterDialog.kernelBox23->value();
    kernel.at<float>(2,0)=lFilterDialog.kernelBox31->value();
    kernel.at<float>(2,1)=lFilterDialog.kernelBox32->value();
    kernel.at<float>(2,2)=lFilterDialog.kernelBox33->value();
}

void MainWindow::linearFilter(Mat image, Mat destImage)
{
    setKernel();
    cv::filter2D(image, destImage, CV_8U, kernel, Point(-1,-1), lFilterDialog.addedVBox->value());
}

void MainWindow::dilate(Mat image, Mat destImage)
{

}

void MainWindow::erode(Mat image, Mat destImage)
{

}


void MainWindow::applySeveral(Mat image, Mat destImage)
{

}

void MainWindow::setPixelTransformation()
{

}


void MainWindow::setOperationOrder()
{

}

void MainWindow::selectOperation(Mat image, Mat destImage)
{

    switch(ui->selectOperation->currentIndex()){
    case 0:
        transformPixel();
        break;
    case 1:
        thresholding(image, destImage);
        break;
    case 2:
        equalize(image, destImage);
        break;
    case 3:
        applyGaussianBlur(image, destImage);
        break;
    case 4:
        applyMedianBlur(image, destImage);
        break;
    case 5:
        linearFilter(image, destImage);
        break;
    case 6:
        dilate(image, destImage);
        break;
    case 7:
        erode(image, destImage);
        break;
    case 8:
        applySeveral(image, destImage);
        break;


    }
}



